#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief  emu user data
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 2

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <am_debug.h>
#include "../am_userdata_internal.h"
#include <am_dvr.h>
#include <am_dmx.h>
#include "../../am_adp_internal.h"
#include <limits.h>
#include <stddef.h>


/****************************************************************************
 * Type definitions
 ***************************************************************************/

#define likely(expr) (expr)
#define unlikely(expr) (expr)


#define N_ELEMENTS(array) (sizeof (array) / sizeof ((array)[0]))
#define CLEAR(var) memset (&(var), 0, sizeof (var))

/* FIXME __typeof__ is a GCC extension. */
#undef SWAP
#define SWAP(x, y)							\
do {									\
	__typeof__ (x) _x = x;						\
	x = y;								\
	y = _x;								\
} while (0)

#undef MIN
#define MIN(x, y) ({							\
	__typeof__ (x) _x = (x);					\
	__typeof__ (y) _y = (y);					\
	(void)(&_x == &_y); /* warn if types do not match */		\
	/* return */ (_x < _y) ? _x : _y;				\
})

#undef MAX
#define MAX(x, y) ({							\
	__typeof__ (x) _x = (x);					\
	__typeof__ (y) _y = (y);					\
	(void)(&_x == &_y); /* warn if types do not match */		\
	/* return */ (_x > _y) ? _x : _y;				\
})

#undef PARENT
#define PARENT(_ptr, _type, _member) ({					\
	__typeof__ (&((_type *) 0)->_member) _p = (_ptr);		\
	(_p != 0) ? (_type *)(((char *) _p) - offsetof (_type,		\
	  _member)) : (_type *) 0;					\
})

/****************************************************************************
 * Static data definitions
 ***************************************************************************/

static AM_ErrorCode_t emu_open(AM_USERDATA_Device_t *dev, const AM_USERDATA_OpenPara_t *para);
static AM_ErrorCode_t emu_close(AM_USERDATA_Device_t *dev);

const AM_USERDATA_Driver_t emu_ud_drv = {
.open  			= emu_open,
.close 			= emu_close,
};



enum start_code {
	PICTURE_START_CODE = 0x00,
	/* 0x01 ... 0xAF slice_start_code */
	/* 0xB0 reserved */
	/* 0xB1 reserved */
	USER_DATA_START_CODE = 0xB2,
	SEQUENCE_HEADER_CODE = 0xB3,
	SEQUENCE_ERROR_CODE = 0xB4,
	EXTENSION_START_CODE = 0xB5,
	/* 0xB6 reserved */
	SEQUENCE_END_CODE = 0xB7,
	GROUP_START_CODE = 0xB8,
	/* 0xB9 ... 0xFF system start codes */
	PRIVATE_STREAM_1 = 0xBD,
	PADDING_STREAM = 0xBE,
	PRIVATE_STREAM_2 = 0xBF,
	AUDIO_STREAM_0 = 0xC0,
	AUDIO_STREAM_31 = 0xDF,
	VIDEO_STREAM_0 = 0xE0,
	VIDEO_STREAM_15 = 0xEF,
};

enum extension_start_code_identifier {
	/* 0x0 reserved */
	SEQUENCE_EXTENSION_ID = 0x1,
	SEQUENCE_DISPLAY_EXTENSION_ID = 0x2,
	QUANT_MATRIX_EXTENSION_ID = 0x3,
	COPYRIGHT_EXTENSION_ID = 0x4,
	SEQUENCE_SCALABLE_EXTENSION_ID = 0x5,
	/* 0x6 reserved */
	PICTURE_DISPLAY_EXTENSION_ID = 0x7,
	PICTURE_CODING_EXTENSION_ID = 0x8,
	PICTURE_SPATIAL_SCALABLE_EXTENSION_ID = 0x9,
	PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID = 0xA,
	/* 0xB ... 0xF reserved */
};

enum picture_coding_type {
	/* 0 forbidden */
	I_TYPE = 1,
	P_TYPE = 2,
	B_TYPE = 3,
	D_TYPE = 4,
	/* 5 ... 7 reserved */
};

enum picture_structure {
	/* 0 reserved */
	TOP_FIELD = 1,
	BOTTOM_FIELD = 2,
	FRAME_PICTURE = 3
};

/* PTSs and DTSs are 33 bits wide. */
#define TIMESTAMP_MASK (((int64_t) 1 << 33) - 1)

struct packet {
	/* Offset in bytes from the buffer start. */
	unsigned int			offset;

	/* Packet and payload size in bytes. */
	unsigned int			size;
	unsigned int			payload;

	/* Decoding and presentation time stamp and display duration
	   of this packet. */
	int64_t				dts;
	int64_t				pts;
	int64_t				duration;

	/* Cut the stream at this packet. */
	AM_Bool_t			splice;

	/* Data is missing before this packet but the producer will
	   correct that later. */
	AM_Bool_t			data_lost;
};

struct buffer {
	uint8_t *			base;

	/* Capacity of the buffer in bytes. */
	unsigned int			capacity;

	/* Offset in bytes from base where the next data will be
	   stored. */
	unsigned int			in;

	/* Offset in bytes from base where the next data will be
	   removed. */ 
	unsigned int			out;
};

#define MAX_PACKETS 64

struct pes_buffer {
	uint8_t *			base;

	/* Capacity of the buffer in bytes. */
	unsigned int			capacity;

	/* Offset in bytes from base where the next data will be stored. */
	unsigned int			in;

	/* Information about the packets in the buffer.
	   packet[0].offset is the offset in bytes from base
	   where the next data will be removed. */
	struct packet			packet[MAX_PACKETS];

	/* Number of packets in the packet[] array. packet[0] must not
	   be removed unless n_packets >= 2. */
	unsigned int			n_packets;
};

struct pes_multiplexer {
	AM_Bool_t			new_file;
	unsigned int			b_state;

	time_t				minicut_end;

	FILE *				minicut_fp;
};

struct audio_es_packetizer {
	/* Test tap. */
	const char *			option_audio_es_tap_file_name;
	FILE *				audio_es_tap_fp;

	struct buffer			ac3_buffer;
	struct pes_buffer		pes_buffer;

	/* Number of bytes we want to examine at pes_buffer.in. */
	unsigned int			need;

	/* Estimated PTS of the next/current AC3 frame (pts or
	   pes_packet_pts plus previous frame duration), -1 if no PTS
	   was received yet. */
	int64_t				pts;

	/* Next/current AC3 frame is the first received frame. */
	AM_Bool_t			first_frame;

	/* Data may have been lost between the previous and
	   next/current AC3 frame. */
	AM_Bool_t			data_lost;

	uint64_t			pes_audio_bit_rate;
};

struct video_es_packetizer {
	struct pes_buffer		pes_buffer;
	int				sequence_header_offset;
	unsigned int			packet_filled;
	uint8_t				pes_packet_header_6;
	uint64_t			pes_video_bit_rate;
	AM_Bool_t			aligned;
};

struct audio_pes_decoder {
	struct buffer			buffer;

	unsigned int			need;
	unsigned int			look_ahead;
};

struct video_recorder {
	struct audio_pes_decoder	apesd;

	struct video_es_packetizer	vesp;

	struct audio_es_packetizer	aesp;

	struct pes_multiplexer		pm;

	/* TS recorder */

	unsigned int			pat_cc;
	unsigned int			pmt_cc;

	time_t				minicut_end;
	FILE *				minicut_fp;
};

enum received_blocks {
	RECEIVED_PES_PACKET		= (1 << 0),
	RECEIVED_PICTURE		= (1 << 1),
	RECEIVED_PICTURE_EXT		= (1 << 2),
	RECEIVED_MPEG_CC_DATA		= (1 << 3)
};

struct video_es_decoder {
	/* Test tap. */
	const char *			option_video_es_all_tap_file_name;
	const char *			option_video_es_tap_file_name;

	FILE *				video_es_tap_fp;

	/* Video elementary stream buffer. */
	struct buffer			buffer;

	unsigned int			min_bytes_valid;

	/* Number of bytes after buffer.out which have already
	   been scanned for a start code prefix. */
	unsigned int			skip;

	/* Last received start code, < 0 if none. If valid
	   buffer.out points at the first start code prefix
	   byte. */
	enum start_code			last_start_code;

	/* The decoding and presentation time stamp of the
	   current picture. Only the lowest 33 bits are
	   valid. < 0 if no PTS or DTS was received or the PES
	   packet header was malformed. */
	int64_t				pts;
	int64_t				dts;

	/* For debugging. */
	uint64_t			n_pictures_received;

	/* Parameters of the current picture. */
	enum picture_coding_type	picture_coding_type;
	enum picture_structure		picture_structure;
	unsigned int			picture_temporal_reference;

	/* Set of the data blocks we received so far. */
	enum received_blocks		received_blocks;

	/* Describes the contents of the reorder_buffer[]:
	   Bit 0 - a top field in reorder_buffer[0],
	   Bit 1 - a bottom field in reorder_buffer[1],
	   Bit 2 - a frame in reorder_buffer[0]. Only the
	   combinations 0, 1, 2, 3, 4 are valid. */
	unsigned int			reorder_pictures;

	/* The PTS (as above) of the data in the reorder_buffer. */
	int64_t				reorder_pts[2];

	unsigned int			reorder_n_bytes[2];

	/* Buffer to convert picture user data from coded
	   order to display order, for the top and bottom
	   field. Maximum size required: 11 + cc_count * 3,
	   where cc_count = 0 ... 31. */
	uint8_t				reorder_buffer[2][128];
};

struct ts_decoder {
	/* TS PID of the video [0] and audio [1] stream. */
	unsigned int			pid[2];

	/* Next expected video and audio TS packet continuity
	   counter. Only the lowest four bits are valid. < 0
	   if no continuity counter has been received yet. */
	int				next_ts_cc[2];

	/* One or more TS packets were lost. */
	AM_Bool_t			data_lost;
}					tsd;


typedef struct
{
	int fd;
	int vpid;
	AM_Bool_t running;
	pthread_t thread;
	struct ts_decoder tsd;
	struct video_es_decoder vesd;

	AM_USERDATA_Device_t *dev;
}emu_ud_drv_data_t;


/****************************************************************************
 * Static functions
 ***************************************************************************/

static int get_vid_pid(emu_ud_drv_data_t *drv_data)
{
#if 1
	if (drv_data->vpid == 0x1fff){
		int handle;
	    int size;
	    char s[1024];
		char *str;

	    handle = open("/sys/class/amstream/ports", O_RDONLY);
	    if (handle < 0) {
	        return -1;
	    }
	    size = read(handle, s, sizeof(s));
	    if (size > 0) {
	        str = strstr(s, "amstream_mpts");
			if (str != NULL){
				str = strstr(str, "Vid:");
				if (str != NULL){
					drv_data->vpid = atoi(str+4);
				}
			}
	    }
	    close(handle);
	}
#else
	//drv_data->vpid = 65;
	drv_data->vpid = 33;
#endif
	return drv_data->vpid;
}


static void
init_buffer			(struct buffer *	b,
				 unsigned int		capacity)
{
	b->capacity = capacity;
	b->base = malloc (capacity);
	b->in = 0;
	b->out = 0;
}

static AM_Bool_t
decode_time_stamp		(int64_t *		ts,
				 const uint8_t *	buf,
				 unsigned int		marker)
{
	/* ISO 13818-1 Section 2.4.3.6 */

	if (0 != ((marker ^ buf[0]) & 0xF1))
		return AM_FALSE;

	if (NULL != ts) {
		unsigned int a, b, c;

		/* marker [4], TS [32..30], marker_bit,
		   TS [29..15], marker_bit,
		   TS [14..0], marker_bit */
		a = (buf[0] >> 1) & 0x7;
		b = (buf[1] * 256 + buf[2]) >> 1;
		c = (buf[3] * 256 + buf[4]) >> 1;

		*ts = ((int64_t) a << 30) + (b << 15) + (c << 0);
	}

	return AM_TRUE;
}


static unsigned int
mpeg2_crc			(const uint8_t *	buf,
				 unsigned int		n_bytes)
{
	static uint32_t crc_table[256];
	unsigned int crc;
	unsigned int i;

	/* ISO 13818-1 Annex B. */

	if (unlikely (0 == crc_table[255])) {
		const unsigned int poly = 
			((1 << 26) | (1 << 23) | (1 << 22) | (1 << 16) |
			 (1 << 12) | (1 << 11) | (1 << 10) | (1 << 8) |
			 (1 << 7) | (1 << 5) | (1 << 4) | (1 << 2) |
			 (1 << 1) | 1);
		unsigned int c, j;

		for (i = 0; i < 256; ++i) {
			c = i << 24;
			for (j = 0; j < 8; ++j) {
				if (c & (1 << 31))
					c = (c << 1) ^ poly;
				else
					c <<= 1;
			}
			crc_table[i] = c;
		}
		assert (0 != crc_table[255]);
	}

	crc = -1;
	for (i = 0; i < n_bytes; ++i)
		crc = crc_table[(buf[i] ^ (crc >> 24)) & 0xFF] ^ (crc << 8);

	return crc & 0xFFFFFFFFUL;
}


/* Video elementary stream decoder. */

static void
vesd_reorder_decode_cc_data	(struct video_es_decoder *vd,
				 const uint8_t *	buf,
				 unsigned int		n_bytes)
{
	emu_ud_drv_data_t *drv_data = PARENT (vd, emu_ud_drv_data_t, vesd);
	AM_USERDATA_Device_t *dev = drv_data->dev;
	
	n_bytes = MIN (n_bytes, (unsigned int)
		       sizeof (vd->reorder_buffer[0]));

	switch (vd->picture_structure) {
	case FRAME_PICTURE:
		AM_DEBUG(4, "FRAME_PICTURE, 0x%02x", vd->reorder_pictures);
		if (0 != vd->reorder_pictures) {
			if (vd->reorder_pictures & 5) {
				/* Top field or top and bottom field. */
				dev->write_package(dev, 
						&vd->reorder_buffer[0][4],
						vd->reorder_n_bytes[0]-4);
			}
			if (vd->reorder_pictures & 2) {
				/* Bottom field. */
				dev->write_package(dev, 
						&vd->reorder_buffer[1][4],
						vd->reorder_n_bytes[1]-4);
			}
		}

		memcpy (vd->reorder_buffer[0], buf, n_bytes);
		vd->reorder_n_bytes[0] = n_bytes;
		vd->reorder_pts[0] = vd->pts;

		/* We have a frame. */
		vd->reorder_pictures = 4;

		break;

	case TOP_FIELD:
		AM_DEBUG(4, "TOP_FIELD, 0x%02x", vd->reorder_pictures);
		if (vd->reorder_pictures >= 3) {
			/* Top field or top and bottom field. */
			dev->write_package(dev, &vd->reorder_buffer[0][4], vd->reorder_n_bytes[0]-4);

			vd->reorder_pictures &= 2;
		} else if (1 == vd->reorder_pictures) {
			/* Apparently we missed a bottom field. */
		}

		memcpy (vd->reorder_buffer[0], buf, n_bytes);
		vd->reorder_n_bytes[0] = n_bytes;
		vd->reorder_pts[0] = vd->pts;

		/* We have a top field. */
		vd->reorder_pictures |= 1;

		break;

	case BOTTOM_FIELD:
		AM_DEBUG(4, "BOTTOM_FIELD, 0x%02x", vd->reorder_pictures);
		if (vd->reorder_pictures >= 3) {
			if (vd->reorder_pictures >= 4) {
				/* Top and bottom field. */
				dev->write_package(dev, 
						&vd->reorder_buffer[0][4],
						vd->reorder_n_bytes[0]-4);
			} else {
				/* Bottom field. */
				dev->write_package(dev, 
						&vd->reorder_buffer[1][4],
						vd->reorder_n_bytes[1]-4);
			}

			vd->reorder_pictures &= 1;
		} else if (2 == vd->reorder_pictures) {
			/* Apparently we missed a top field. */
		}

		memcpy (vd->reorder_buffer[1], buf, n_bytes);
		vd->reorder_n_bytes[1] = n_bytes;
		vd->reorder_pts[1] = vd->pts;

		/* We have a bottom field. */
		vd->reorder_pictures |= 2;

		break;

	default: /* invalid */
		break;
	}
}

static void
vesd_user_data			(struct video_es_decoder *vd,
				 const uint8_t *	buf,
				 unsigned int		min_bytes_valid)
{
	unsigned int ATSC_identifier;
	unsigned int user_data_type_code;
	unsigned int cc_count;
	emu_ud_drv_data_t *drv_data = PARENT (vd, emu_ud_drv_data_t, vesd);
	AM_USERDATA_Device_t *dev = drv_data->dev;

	/* ATSC A/53 Part 4:2007 Section 6.2.2 */

	if (0 == (vd->received_blocks & RECEIVED_PICTURE)) {
		/* Either sequence or group user_data, or we missed
		   the picture_header. */
		vd->received_blocks &= ~RECEIVED_PES_PACKET;
		return;
	}

	/* start_code_prefix [24], start_code [8],
	   ATSC_identifier [32], user_data_type_code [8] */
	if (min_bytes_valid < 9)
		return;

	ATSC_identifier = ((buf[4] << 24) | (buf[5] << 16) |
			   (buf[6] << 8) | buf[7]);
	if (0x47413934 != ATSC_identifier)
		return;

	user_data_type_code = buf[8];
	if (0x03 != user_data_type_code)
		return;

	/* ATSC A/53 Part 4:2007 Section 6.2.1: "No more than one
	   user_data() structure using the same user_data_type_code
	   [...] shall be present following any given picture
	   header." */
	if (vd->received_blocks & RECEIVED_MPEG_CC_DATA) {
		/* Too much data lost. */
		return;
	}

	vd->received_blocks |= RECEIVED_MPEG_CC_DATA;

	/* reserved, process_cc_data_flag, zero_bit, cc_count [5],
	   reserved [8] */
	if (min_bytes_valid < 11)
		return;

	/* one_bit, reserved [4], cc_valid, cc_type [2],
	   cc_data_1 [8], cc_data_2 [8] */
	cc_count = buf[9] & 0x1F;

	/* CEA 708-C Section 4.4 permits padding, so we have to see
	   all cc_data elements. */
	if (min_bytes_valid < 11 + cc_count * 3)
		return;


	/* CEA 708-C Section 4.4.1.1 */

	switch (vd->picture_coding_type) {
	case I_TYPE:
	case P_TYPE:
		AM_DEBUG(4, "%s, 0x%02x", vd->picture_coding_type==I_TYPE ? "I_TYPE" : "P_TYPE", vd->reorder_pictures);
		vesd_reorder_decode_cc_data (vd, buf, min_bytes_valid);
		break;

	case B_TYPE:
		AM_DEBUG(4, "B_TYPE, 0x%02x", vd->reorder_pictures);
		/* To prevent a gap in the caption stream we must not
		   decode B pictures until we have buffered both
		   fields of the temporally following I or P picture. */
		if (vd->reorder_pictures < 3) {
			vd->reorder_pictures = 0;
			break;
		}

		/* To do: If a B picture appears to have a higher
		   temporal_reference than the picture it forward
		   references we lost that I or P picture. */
		{
			dev->write_package(dev,  buf+4,
					min_bytes_valid-4);
		}

		break;

	default: /* invalid */
		break;
	}
}

static void
vesd_extension			(struct video_es_decoder *vd,
				 const uint8_t *	buf,
				 unsigned int		min_bytes_valid)
{
	enum extension_start_code_identifier extension_start_code_identifier;

	/* extension_start_code [32],
	   extension_start_code_identifier [4],
	   f_code [4][4], intra_dc_precision [2],
	   picture_structure [2], ... */
	if (min_bytes_valid < 7)
		return;

	extension_start_code_identifier =
		(enum extension_start_code_identifier)(buf[4] >> 4);
	if (PICTURE_CODING_EXTENSION_ID
	    != extension_start_code_identifier)
		return;

	vd->picture_structure = (enum picture_structure)(buf[6] & 3);

	vd->received_blocks |= RECEIVED_PICTURE_EXT;
}

static void
vesd_picture_header		(struct video_es_decoder *vd,
				 const uint8_t *	buf,
				 unsigned int		min_bytes_valid)
{
	unsigned int c;

	/* picture_start_code [32],
	   picture_temporal_reference [10],
	   picture_coding_type [3], ... */
	if (min_bytes_valid < 6
	    || vd->received_blocks != RECEIVED_PES_PACKET) {
		/* Too much data lost. */
		vd->received_blocks = 0;
		return;
	}

	c = buf[4] * 256 + buf[5];
	vd->picture_temporal_reference = (c >> 6) & 0x3FF;
	vd->picture_coding_type = (enum picture_coding_type)((c >> 3) & 7);

	vd->received_blocks = (RECEIVED_PES_PACKET |
			       RECEIVED_PICTURE);

	++vd->n_pictures_received;
}

static void
vesd_pes_packet_header		(struct video_es_decoder *vd,
				 const uint8_t *	buf,
				 unsigned int		min_bytes_valid)
{
	unsigned int PES_packet_length;
	unsigned int PTS_DTS_flags;
	int64_t pts;


	vd->pts = -1;
	vd->dts = -1;

	vd->received_blocks = 0;

	/* packet_start_code_prefix [24], stream_id [8],
	   PES_packet_length [16],

	   '10', PES_scrambling_control [2], PES_priority,
	   data_alignment_indicator, copyright, original_or_copy,

	   PTS_DTS_flags [2], ESCR_flag, ES_rate_flag,
	   DSM_trick_mode_flag, additional_copy_info_flag,
	   PES_CRC_flag, PES_extension_flag,

	   PES_header_data_length [8] */
	if (min_bytes_valid < 9)
		return;

	PES_packet_length = buf[4] * 256 + buf[5];
	PTS_DTS_flags = (buf[7] & 0xC0) >> 6;

	/* ISO 13818-1 Section 2.4.3.7: In transport streams video PES
	   packets do not carry data, they only contain the DTS/PTS of
	   the following picture and PES_packet_length must be
	   zero. */
	if (0 != PES_packet_length)
		return;

	switch (PTS_DTS_flags) {
	case 0: /* no timestamps */
		return;

	case 1: /* forbidden */
		return;

	case 2: /* PTS only */
		if (min_bytes_valid < 14)
			return;
		if (!decode_time_stamp (&vd->pts, &buf[9], 0x21))
			return;
		break;

	case 3: /* PTS and DTS */
		if (min_bytes_valid < 19)
			return;
		if (!decode_time_stamp (&pts, &buf[9], 0x31))
			return;
		if (!decode_time_stamp (&vd->dts, &buf[14], 0x11))
			return;
		vd->pts = pts;
		break;
	}


	vd->received_blocks = RECEIVED_PES_PACKET;
}

static void
vesd_decode_block		(struct video_es_decoder *vd,
				 unsigned int		start_code,
				 const uint8_t *	buf,
				 unsigned int		n_bytes,
				 unsigned int		min_bytes_valid,
				 AM_Bool_t		data_lost)
{

	/* The CEA 608-C and 708-C Close Caption data is encoded in
	   picture user data fields. ISO 13818-2 requires the start
	   code sequence 0x00, 0xB5/8, (0xB5?, 0xB2?)*. To properly
	   convert from coded order to display order we also need the
	   picture_coding_type and picture_structure fields. */

	if (likely (start_code <= 0xAF)) {
		if (unlikely (0x00 == start_code) && !data_lost) {
			vesd_picture_header (vd, buf, min_bytes_valid);
		} else {
			/* slice_start_code or data lost in or after
			   the picture_header. */
			/* For all we care the picture data is just
			   useless filler prior to the next PES packet
			   header, and we need an uninterrupted
			   sequence from there to the next picture
			   user_data to ensure the PTS, DTS,
			   picture_temporal_reference,
			   picture_coding_type, picture_structure and
			   cc_data belong together. */
			vd->received_blocks = 0;
		}
	} else if (USER_DATA_START_CODE == start_code) {
		vesd_user_data (vd, buf, min_bytes_valid);
	} else if (data_lost) {
		/* Data lost in or after this block. */
		vd->received_blocks = 0;
	} else if (EXTENSION_START_CODE == start_code) {
		vesd_extension (vd, buf, min_bytes_valid);
	} else if (start_code >= VIDEO_STREAM_0
		   && start_code <= VIDEO_STREAM_15) {
		vesd_pes_packet_header (vd, buf, min_bytes_valid);
	} else {
		/* Should be a sequence_header or
		   group_of_pictures_header. */

		vd->received_blocks &= (1 << 0);
	}
}

static unsigned int
vesd_make_room			(struct video_es_decoder *vd,
				 unsigned int		required)
{
	struct buffer *b;
	unsigned int capacity;
	unsigned int in;

	b = &vd->buffer;
	capacity = b->capacity;
	in = b->in;

	if (unlikely (in + required > capacity)) {
		unsigned int consumed;
		unsigned int unconsumed;

		consumed = b->out;
		unconsumed = in - consumed;
		if (required > capacity - unconsumed) {
			/* XXX make this a recoverable error. */
			AM_DEBUG(1, "Video ES buffer overflow.\n");
			return 0;
		}
		memmove (b->base, b->base + consumed, unconsumed);
		in = unconsumed;
		b->out = 0;
	}

	return in;
}

static void
video_es_decoder		(struct video_es_decoder *vd,
				 const uint8_t *	buf,
				 unsigned int		n_bytes,
				 AM_Bool_t		data_lost)
{
	const uint8_t *s;
	const uint8_t *e;
	const uint8_t *e_max;
	unsigned int in;

	/* This code searches for a start code and then decodes the
	   data between the previous and the current start code. */

	in = vesd_make_room (vd, n_bytes);

	memcpy (vd->buffer.base + in, buf, n_bytes);
	vd->buffer.in = in + n_bytes;

	s = vd->buffer.base + vd->buffer.out + vd->skip;
	e = vd->buffer.base + in + n_bytes - 4;
	e_max = e;

	if (unlikely (data_lost)) {
		if (vd->min_bytes_valid >= UINT_MAX) {
			vd->min_bytes_valid =
				in - vd->buffer.out;
		}

		/* Data is missing after vd->buffer.base + in, so
		   we must ignore apparent start codes crossing that
		   boundary. */
		e -= n_bytes;
	}

	for (;;) {
		const uint8_t *b;
		enum start_code start_code;
		unsigned int n_bytes;
		unsigned int min_bytes_valid;

		for (;;) {
			if (s >= e) {
				/* Need more data. */

				if (unlikely (s < e_max)) {
					/* Skip over the lost data. */
					s = e + 4;
					e = e_max;
					continue;
				}

				/* In the next iteration skip the
				   bytes we already scanned. */
				vd->skip = s - vd->buffer.base
					- vd->buffer.out;

				return;
			}

			if (likely (0 != (s[2] & ~1))) {
				/* Not 000001 or xx0000 or xxxx00. */
				s += 3;
			} else if (0 != (s[0] | s[1]) || 1 != s[2]) {
				++s;
			} else {
				break;
			}
		}

		b = vd->buffer.base + vd->buffer.out;
		n_bytes = s - b;
		min_bytes_valid = n_bytes;
		data_lost = AM_FALSE;

		if (unlikely (vd->min_bytes_valid < UINT_MAX)) {
			if (n_bytes < vd->min_bytes_valid) {
				/* We found a new start code before
				   the missing data. */
				vd->min_bytes_valid -= n_bytes;
			} else {
				min_bytes_valid = vd->min_bytes_valid;
				vd->min_bytes_valid = UINT_MAX;

				/* Need a flag in case we lost data just
				   before the next start code. */
				data_lost = AM_TRUE;
			}
		}

		start_code = vd->last_start_code;
		if (likely ((int) start_code >= 0)) {
			vesd_decode_block (vd, start_code, b,
					   n_bytes, min_bytes_valid,
					   data_lost);
		}

		/* Remove the data we just decoded from the
		   buffer. Remember the position of the new start code
		   we found, skip it and continue the search. */
		vd->buffer.out = s - vd->buffer.base;
		vd->last_start_code = (enum start_code) s[3];
		s += 4;
	}
}

static void
reset_video_es_decoder		(struct video_es_decoder *vd)
{
	vd->buffer.in = 0;
	vd->buffer.out = 0;

	vd->min_bytes_valid = UINT_MAX;
	vd->skip = 0;
	vd->last_start_code = (enum start_code) -1;

	vd->pts = -1;
	vd->dts = -1;
	vd->picture_coding_type = (enum picture_coding_type) -1;
	vd->picture_structure = (enum picture_structure) -1;
	vd->received_blocks = 0;
	vd->reorder_pictures = 0;
}

static void
init_video_es_decoder		(struct video_es_decoder *vd)
{
	CLEAR (*vd);

	init_buffer (&vd->buffer, /* capacity */ 1 << 20);
	reset_video_es_decoder (vd);
}


static void
tsd_program			(emu_ud_drv_data_t *drv_data,
				 const uint8_t		buf[188],
				 unsigned int		pid,
				 unsigned int		es_num)
{
	unsigned int adaptation_field_control;
	unsigned int header_length;
	unsigned int payload_length;
	AM_Bool_t data_lost;

	adaptation_field_control = (buf[3] & 0x30) >> 4;
	if (likely (1 == adaptation_field_control)) {
		header_length = 4;
	} else if (3 == adaptation_field_control) {
		unsigned int adaptation_field_length;

		adaptation_field_length = buf[4];

		/* Zero length is used for stuffing. */
		if (adaptation_field_length > 0) {
			unsigned int discontinuity_indicator;

			/* ISO 13818-1 Section 2.4.3.5. Also the code
			   below would be rather upset if
			   header_length > packet_size. */
			if (adaptation_field_length > 182) {
				AM_DEBUG (2, "Invalid TS header ");
				/* Possibly. */
				drv_data->tsd.data_lost = AM_TRUE;
				return;
			}

			/* ISO 13818-1 Section 2.4.3.5 */
			discontinuity_indicator = buf[5] & 0x80;
			if (discontinuity_indicator)
				drv_data->tsd.next_ts_cc[es_num] = -1;
		}

		header_length = 5 + adaptation_field_length;
	} else {
		/* 0 == adaptation_field_control: invalid;
		   2 == adaptation_field_control: no payload. */
		/* ISO 13818-1 Section 2.4.3.3:
		   continuity_counter shall not increment. */
		return;
	}

	payload_length = 188 - header_length;

	data_lost = drv_data->tsd.data_lost;

	if (unlikely (0 != ((drv_data->tsd.next_ts_cc[es_num]
			     ^ buf[3]) & 0x0F))) {
		/* Continuity counter mismatch. */

		if (drv_data->tsd.next_ts_cc[es_num] < 0) {
			/* First TS packet. */
		} else if (0 == (((drv_data->tsd.next_ts_cc[es_num] - 1)
				  ^ buf[3]) & 0x0F)) {
			/* ISO 13818-1 Section 2.4.3.3: Repeated packet. */
			return;
		} else {
			AM_DEBUG (2, "TS continuity error ");
			data_lost = AM_TRUE;
		}
	}

	drv_data->tsd.next_ts_cc[es_num] = buf[3] + 1;
	drv_data->tsd.data_lost = AM_FALSE;

	if (0 == es_num) {
		video_es_decoder (&drv_data->vesd, buf + header_length,
				  payload_length, data_lost);
	}
}

static void
ts_decoder			(emu_ud_drv_data_t *drv_data, const uint8_t		buf[188])
{
	unsigned int pid;
	unsigned int i;

	if (unlikely (buf[1] & 0x80)) {
		AM_DEBUG(1, "TS transmission error.\n");
		return;
	}

	pid = (buf[1] * 256 + buf[2]) & 0x1FFF;

	if (pid == get_vid_pid(drv_data))
		tsd_program (drv_data, buf, pid, 0);
}

static void
init_ts_decoder			(struct ts_decoder *	td)
{
	CLEAR (*td);

	memset (&td->next_ts_cc, -1, sizeof (td->next_ts_cc));
}

static int read_ts_packet(const uint8_t *tsbuf, int tssize, uint8_t buf[188])
{
	int p = 0;
	
	/*Scan the sync byte*/
	while(tsbuf[p]!=0x47)
	{
		p++;
		if(p>=tssize)
		{
			return tssize;
		}
	}
	
	if(p!=0)
		AM_DEBUG(1, "skip %d bytes", p);
	
	if ((tssize - p) >= 188)
	{
		memcpy(buf, tsbuf+p, 188);
		p += 188;
	}

	return p;
}

#if 1
static void* dvr_data_thread(void *arg)
{
	AM_USERDATA_Device_t *dev = (AM_USERDATA_Device_t*)arg;
	emu_ud_drv_data_t *drv_data = (emu_ud_drv_data_t*)dev->drv_data;
	int cnt, left, rp;
	uint8_t buf[256*1024];
	uint8_t pkt_buf[188];
	uint8_t *p;
	AM_DMX_OpenPara_t dpara;
	AM_DVR_OpenPara_t para;
	AM_DVR_StartRecPara_t spara;

	memset(&dpara, 0, sizeof(dpara));
	AM_DMX_Open(0, &dpara);
	AM_DVR_Open(0, &para);
	AM_DVR_SetSource(0, 0);
	spara.pid_count = 1;
	spara.pids[0] = get_vid_pid(drv_data);
	AM_DVR_StartRecord(0, &spara);
	
	left = 0;
	
	while (drv_data->running)
	{
		cnt = AM_DVR_Read(0, buf+left, sizeof(buf)-left, 1000);
		if (cnt <= 0)
		{
			AM_DEBUG(1, "No data available from DVR0");
			usleep(200*1000);
			continue;
		}
		cnt += left;
		left = cnt;
		p = buf;
		AM_DEBUG(1, "read from DVR return %d bytes", cnt);
		while (left >= 188)
		{
			pkt_buf[0] = 0;
			//AM_DEBUG(1, "read 188 bytes ts packet");
			rp = read_ts_packet(p, left, pkt_buf);
			left -= rp;
			p += rp;
			
			if (pkt_buf[0] == 0x47)
				ts_decoder(drv_data, pkt_buf);
		}
		if (left > 0)
			memmove(buf, buf+cnt-left, left);
	}

	AM_DVR_Close(0);
	AM_DMX_Close(0);

	AM_DEBUG(1, "Data thread for DVR0 now exit");

	return NULL;
}
#else

static void* dvr_data_thread(void *arg)
{
	AM_USERDATA_Device_t *dev = (AM_USERDATA_Device_t*)arg;
	emu_ud_drv_data_t *drv_data = (emu_ud_drv_data_t*)dev->drv_data;
	int cnt, left, rp;
	uint8_t buf[256*1024];
	uint8_t pkt_buf[188];
	uint8_t *p;
	//int fd = open("/mnt/sda1/DTV_CC_English_Spanish.ts", O_RDONLY);
	int fd = open("/mnt/sda1/atsc-480i-TV-14.ts", O_RDONLY);
	uint32_t total = 0;

	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open file");
		return NULL;
	}
	
	left = 0;
	
	while (drv_data->running)
	{
		cnt = read(fd, buf+left, sizeof(buf)-left);
		if (cnt <= 0)
		{
			AM_DEBUG(1, "No data available from file");
			usleep(200*1000);
			lseek(fd, 0, SEEK_SET);
			continue;
		}
		cnt += left;
		left = cnt;
		p = buf;
		AM_DEBUG(1, "read from file return %d bytes", cnt);
		while (left >= 188)
		{
			pkt_buf[0] = 0;
			//AM_DEBUG(1, "read 188 bytes ts packet");
			rp = read_ts_packet(p, left, pkt_buf);
			left -= rp;
			p += rp;
			
			if (pkt_buf[0] == 0x47)
				ts_decoder(drv_data, pkt_buf);

			total += rp;
			if (total >= 125000)
			{
				usleep(20*1000);
				total = 0;
			}
		}
		if (left > 0)
			memmove(buf, buf+cnt-left, left);
	}

	close(fd);

	return NULL;
}
#endif

static AM_ErrorCode_t emu_open(AM_USERDATA_Device_t *dev, const AM_USERDATA_OpenPara_t *para)
{
	emu_ud_drv_data_t *drv_data = malloc(sizeof(emu_ud_drv_data_t));
	
	init_ts_decoder (&drv_data->tsd);
	init_video_es_decoder (&drv_data->vesd);

	drv_data->running = AM_TRUE;
	drv_data->dev = dev;
	drv_data->vpid = 0x1fff;
	dev->drv_data = (void*)drv_data;
	pthread_create(&drv_data->thread, NULL, dvr_data_thread, (void*)dev);

	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_close(AM_USERDATA_Device_t *dev)
{
	emu_ud_drv_data_t *drv_data = (emu_ud_drv_data_t*)dev->drv_data;

	pthread_mutex_unlock(&am_gAdpLock);
	drv_data->running = AM_FALSE;
	pthread_join(drv_data->thread, NULL);
	free(drv_data);
	pthread_mutex_lock(&am_gAdpLock);
	
	return AM_SUCCESS;
}


