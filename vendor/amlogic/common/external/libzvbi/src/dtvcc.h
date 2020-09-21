#ifndef _DTVCC_H_
#define _DTVCC_H_

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "libzvbi.h"

enum field_num {
	FIELD_1 = 0,
	FIELD_2,
	MAX_FIELDS
};

enum cc_mode {
	CC_MODE_UNKNOWN,
	CC_MODE_ROLL_UP,
	CC_MODE_POP_ON,
	CC_MODE_PAINT_ON,
	CC_MODE_TEXT
};

enum cc_effect_status {
	CC_EFFECT_NONE,
	CC_EFFECT_DISPLAY,
	CC_EFFECT_HIDE
};

/* EIA 608-B Section 4.1. */
#define VBI_CAPTION_CC1 1 /* primary synchronous caption service (F1) */
#define VBI_CAPTION_CC2 2 /* special non-synchronous use captions (F1) */
#define VBI_CAPTION_CC3 3 /* secondary synchronous caption service (F2) */
#define VBI_CAPTION_CC4 4 /* special non-synchronous use captions (F2) */

#define VBI_CAPTION_T1 5 /* first text service (F1) */
#define VBI_CAPTION_T2 6 /* second text service (F1) */
#define VBI_CAPTION_T3 7 /* third text service (F2) */
#define VBI_CAPTION_T4 8 /* fourth text service (F2) */

#define UNKNOWN_CC_CHANNEL 0
#define MAX_CC_CHANNELS 8

/* 47 CFR 15.119 (d) Screen format. */
#define CC_FIRST_ROW            0
#define CC_LAST_ROW             14
#define CC_MAX_ROWS             15

#define CC_FIRST_COLUMN         1
#define CC_LAST_COLUMN          32
#define CC_MAX_COLUMNS          32

#define CC_ALL_ROWS_MASK ((1 << CC_MAX_ROWS) - 1)

#define VBI_TRANSLUCENT VBI_SEMI_TRANSPARENT

struct cc_timestamp {
	/* System time when the event occured, zero if no event occured yet. */
	struct timeval          sys;

	/* Presentation time stamp of the event. Only the 33 least
	   significant bits are valid. < 0 if no event occured yet. */
	int64_t                 pts;
};

struct cc_channel {
	/**
	 * [0] and [1] are the displayed and non-displayed buffer as
	 * defined in 47 CFR 15.119, and selected by displayed_buffer
	 * below. [2] is a snapshot of the displayed buffer at the
	 * last stream event.
	 *
	 * XXX Text channels don't need buffer[2] and buffer[3], we're
	 * wasting memory.
	 */
	uint16_t		buffer[3][CC_MAX_ROWS][1 + CC_MAX_COLUMNS];

	/**
	 * For buffer[0 ... 2], if bit 1 << row is set this row
	 * contains displayable characters, spacing or non-spacing
	 * attributes. (Special character 0x1139 "transparent space"
	 * is not a displayable character.) This information is
	 * intended to speed up copying, erasing and formatting.
	 */
	unsigned int		dirty[3];

	/** Index of displayed buffer, 0 or 1. */
	unsigned int		displayed_buffer;

	/**
	 * Cursor position: FIRST_ROW ... LAST_ROW and
	 * FIRST_COLUMN ... LAST_COLUMN.
	 */
	unsigned int		curr_row;
	unsigned int		curr_column;

	/**
	 * Text window height in CC_MODE_ROLL_UP. The first row of the
	 * window is curr_row - window_rows + 1, the last row is
	 * curr_row.
	 *
	 * Note: curr_row - window_rows + 1 may be < FIRST_ROW, this
	 * must be clipped before using window_rows:
	 *
	 * actual_rows = MIN (curr_row - FIRST_ROW + 1, window_rows);
	 *
	 * We won't do that at the RUx command because usually a PAC
	 * follows which may change curr_row.
	 */
	unsigned int		window_rows;

	/* Most recently received PAC command. */
	unsigned int		last_pac;

	/**
	 * This variable counts successive transmissions of the
	 * letters A to Z. It is reset to zero on reception of any
	 * letter a to z.
	 *
	 * Some stations do not transmit EIA 608-B extended characters
	 * and except for N with tilde the standard and special
	 * character sets contain only lower case accented
	 * characters. We force these characters to upper case if this
	 * variable indicates live caption, which is usually all upper
	 * case.
	 */
	unsigned int		uppercase_predictor;

	/** Current caption mode or CC_MODE_UNKNOWN. */
	enum cc_mode		mode;

	/**
	 * The time when we last received data for this
	 * channel. Intended to detect if this caption channel is
	 * active.
	 */
	struct cc_timestamp	timestamp;

	/**
	 * The time when we received the first (but not necessarily
	 * leftmost) character in the current row. Unless the mode is
	 * CC_MODE_POP_ON the next stream event will carry this
	 * timestamp.
	 */
	struct cc_timestamp	timestamp_c0;
};

struct cc_decoder {
	/**
	 * Decoder state. We decode all channels in parallel, this way
	 * clients can switch between channels without data loss, or
	 * capture multiple channels with a single decoder instance.
	 *
	 * Also 47 CFR 15.119 and EIA 608-C require us to remember the
	 * cursor position on each channel.
	 */
	struct cc_channel	channel[MAX_CC_CHANNELS];

	/**
	 * Current channel, switched by caption control codes. Can be
	 * one of @c VBI_CAPTION_CC1 ... @c VBI_CAPTION_CC4 or @c
	 * VBI_CAPTION_T1 ... @c VBI_CAPTION_T4 or @c
	 * UNKNOWN_CC_CHANNEL if no channel number was received yet.
	 */
	vbi_pgno		curr_ch_num[MAX_FIELDS];

	/**
	 * Caption control codes (two bytes) may repeat once for error
	 * correction. -1 if no repeated control code can be expected.
	 */
	int			expect_ctrl[MAX_FIELDS][2];

	/** Receiving XDS data, as opposed to caption / ITV data. */
	vbi_bool		in_xds[MAX_FIELDS];

	/**
	 * Pointer into the channel[] array if a display update event
	 * shall be sent at the end of this iteration, %c NULL
	 * otherwise. Purpose is to suppress an event for the first of
	 * two displayable characters in a caption byte pair.
	 */
	struct cc_channel *	event_pending;

	/**
	 * Remembers past parity errors: One bit for each call of
	 * cc_feed(), most recent result in lsb. The idea is to
	 * disable the decoder if we detect too many errors.
	 */
	unsigned int		error_history;

	/**
	 * The time when we last received data, including NUL bytes.
	 * Intended to detect if the station transmits any data on
	 * line 21 or 284 at all.
	 */
	struct cc_timestamp	timestamp;
};

/* CEA 708-C decoder. */

enum justify {
	JUSTIFY_LEFT = 0,
	JUSTIFY_RIGHT,
	JUSTIFY_CENTER,
	JUSTIFY_FULL
};

enum direction {
	DIR_LEFT_RIGHT = 0,
	DIR_RIGHT_LEFT,
	DIR_TOP_BOTTOM,
	DIR_BOTTOM_TOP
};

enum display_effect {
	DISPLAY_EFFECT_SNAP = 0,
	DISPLAY_EFFECT_FADE,
	DISPLAY_EFFECT_WIPE
};

enum opacity {
	OPACITY_SOLID = 0,
	OPACITY_FLASH,
	OPACITY_TRANSLUCENT,
	OPACITY_TRANSPARENT
};

enum edge {
	EDGE_NONE = 0,
	EDGE_RAISED,
	EDGE_DEPRESSED,
	EDGE_UNIFORM,
	EDGE_SHADOW_LEFT,
	EDGE_SHADOW_RIGHT
};

enum pen_size {
	PEN_SIZE_SMALL = 0,
	PEN_SIZE_STANDARD,
	PEN_SIZE_LARGE
};

enum font_style {
	FONT_STYLE_DEFAULT = 0,
	FONT_STYLE_MONO_SERIF,
	FONT_STYLE_PROP_SERIF,
	FONT_STYLE_MONO_SANS,
	FONT_STYLE_PROP_SANS,
	FONT_STYLE_CASUAL,
	FONT_STYLE_CURSIVE,
	FONT_STYLE_SMALL_CAPS
};

enum text_tag {
	TEXT_TAG_DIALOG = 0,
	TEXT_TAG_SOURCE_ID,
	TEXT_TAG_DEVICE,
	TEXT_TAG_DIALOG_2,
	TEXT_TAG_VOICEOVER,
	TEXT_TAG_AUDIBLE_TRANSL,
	TEXT_TAG_SUBTITLE_TRANSL,
	TEXT_TAG_VOICE_DESCR,
	TEXT_TAG_LYRICS,
	TEXT_TAG_EFFECT_DESCR,
	TEXT_TAG_SCORE_DESCR,
	TEXT_TAG_EXPLETIVE,
	TEXT_TAG_NOT_DISPLAYABLE = 15
};

enum offset {
	OFFSET_SUBSCRIPT = 0,
	OFFSET_NORMAL,
	OFFSET_SUPERSCRIPT
};

/* RGB 2:2:2 (lsb = B). */
typedef uint8_t			dtvcc_color;

/* Lsb = window 0, msb = window 7. */
typedef uint8_t			dtvcc_window_map;

struct dtvcc_pen_style {
	enum pen_size			pen_size;
	enum font_style			font_style;
	enum offset			offset;
	vbi_bool			italics;
	vbi_bool			underline;

	enum edge			edge_type;

	dtvcc_color			fg_color;
	enum opacity			fg_opacity;

	dtvcc_color			bg_color;
	enum opacity			bg_opacity;

	dtvcc_color			edge_color;
	vbi_bool fg_flash;
	vbi_bool bg_flash;
};

struct dtvcc_pen {
	enum text_tag			text_tag;
	struct dtvcc_pen_style		style;
};

struct dtvcc_window_style {
	enum justify			justify;
	enum direction			print_direction;
	enum direction			scroll_direction;
	vbi_bool			wordwrap;

	enum display_effect		display_effect;
	enum direction			effect_direction;
	unsigned int			effect_speed; /* 1/10 sec */

	dtvcc_color			fill_color;
	enum opacity			fill_opacity;
	vbi_bool window_flash;

	enum edge			border_type;
	dtvcc_color			border_color;
};

struct dtvcc_window {
	/* EIA 708-C window state. */

	uint16_t			buffer[16][42];
	struct dtvcc_pen_style          pen[16][42];
	int row_start[16];
	int latest_cmd_cr;

	vbi_bool			visible;

	/* 0 = highest ... 7 = lowest. */
	unsigned int			priority;

	unsigned int			anchor_point;
	unsigned int			anchor_horizontal;
	unsigned int			anchor_vertical;
	vbi_bool			anchor_relative;

	unsigned int			row_count;
	unsigned int			column_count;

	vbi_bool			row_lock;
	vbi_bool			column_lock;
	unsigned int        column_no_lock_length;
	unsigned int        row_no_lock_length;

	unsigned int			curr_row;
	unsigned int			curr_column;

	struct dtvcc_pen		curr_pen;

	struct dtvcc_window_style	style;

	/* Used for fade and swipe */
	struct timespec effect_timer;
	/* 0~100 */
	int effect_percent;
	enum cc_effect_status effect_status;

	/* Our stuff. */

	/**
	 * If bit 1 << row is set we already sent a stream event for
	 * this row.
	 */
	unsigned int			streamed;

	/**
	 * The time when we received the first (but not necessarily
	 * leftmost) character in the current row. Unless a
	 * DisplayWindow or ToggleWindow command completed the line
	 * the next stream event will carry this timestamp.
	 */
	struct cc_timestamp		timestamp_c0;
};

struct dtvcc_service {
	/* Interpretation Layer. */
	int id;

	struct dtvcc_window		window[8];
	struct dtvcc_window             old_window[8];

	int                             old_win_cnt;
	int                             update;

	struct dtvcc_window *		curr_window;

	dtvcc_window_map		created;

	/* For debugging. */
	unsigned int			error_line;

	/* Service Layer. */

	uint8_t				service_data[128];
	unsigned int			service_data_in;
	/* For 0x8D 0x8E delay command
	*  if delay flag is set, decoder stop decoding
	*/
	struct timespec delay_timer;


	int delay;
	int delay_cancel;
	/** The time when we last received data for this service. */
	struct cc_timestamp		timestamp;
};

struct dtvcc_decoder {
	char lang[12];
	unsigned int decoder_param;
	struct dtvcc_service		service[6];

	/* Packet Layer. */

	uint8_t				packet[128];
	unsigned int			packet_size;

	/* Next expected DTVCC packet sequence_number. Only the two
	   most significant bits are valid. < 0 if no sequence_number
	   has been received yet. */
	int				next_sequence_number;

	/** The time when we last received data. */
	struct cc_timestamp		timestamp;

	int                             flash_state;
	int curr_data_pgno;
	int lang_kor;
};

/* ATSC A/53 Part 4:2007 Closed Caption Data decoder. */

enum cc_type {
	NTSC_F1 = 0,
	NTSC_F2 = 1,
	DTVCC_DATA = 2,
	DTVCC_START = 3,
};

struct cc_data_decoder {
	/* Test tap. */
	const char *			option_cc_data_tap_file_name;

	FILE *				cc_data_tap_fp;

	/* For debugging. */
	int64_t				last_pts;
};

/* Caption recorder. */

enum cc_attr {
	VBI_UNDERLINE	= (1 << 0),
	VBI_ITALIC	= (1 << 2),
	VBI_FLASH	= (1 << 3)
};

struct cc_pen {
	uint8_t				attr;

	uint8_t				fg_color;
	uint8_t				fg_opacity;

	uint8_t				bg_color;
	uint8_t				bg_opacity;

	uint8_t				edge_type;
	uint8_t				edge_color;
	uint8_t				edge_opacity;

	uint8_t				pen_size;
	uint8_t				font_style;

	uint8_t				reserved[6];
};

enum caption_format {
	FORMAT_PLAIN,
	FORMAT_VT100,
	FORMAT_NTSC_CC
};

struct tvcc_decoder {
	pthread_mutex_t mutex;
	struct vbi_decoder *vbi;
	struct cc_decoder cc;
	struct dtvcc_decoder dtvcc;
};
unsigned int dtvcc_unicode (unsigned int c);

extern void     tvcc_init(struct tvcc_decoder *td, char* lang, int lang_len, unsigned int decoder_param);

extern void     tvcc_decode_data(struct tvcc_decoder *td, int64_t pts, const uint8_t *buf, unsigned int n_bytes);

extern void     tvcc_reset(struct tvcc_decoder *td);

extern void     tvcc_destroy(struct tvcc_decoder *td);

extern void     tvcc_fetch_page(struct tvcc_decoder *td, int pgno, int *cnt, struct vbi_page *sub_pages);

static void     dtvcc_init(struct dtvcc_decoder *	dc, char* lang, int lang_len, int decoder_param);

extern void     dtvcc_decode_packet(struct dtvcc_decoder *dc, const struct timeval *tv, int64_t pts);

extern void     dtvcc_reset(struct dtvcc_decoder * dc);

extern void     cc_init(struct cc_decoder *cd);

extern vbi_bool cc_feed(struct cc_decoder *cd, const uint8_t buffer[2], unsigned int line, const struct timeval * tv, int64_t pts);

extern void     cc_reset(struct cc_decoder *cd);

extern void     vbi_decode_caption(vbi_decoder *vbi, int line, const uint8_t *buf);

extern int      get_input_offset();

extern void update_service_status(struct tvcc_decoder *td);
static vbi_bool dtvcc_carriage_return		(struct dtvcc_decoder *dc, struct dtvcc_service * ds);



#endif

