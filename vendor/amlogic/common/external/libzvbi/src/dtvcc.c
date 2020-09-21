/*
 *  atsc-cc -- ATSC Closed Caption decoder
 *
 *  Copyright (C) 2008 Michael H. Schimek <mschimek@users.sf.net>
 *
 *  Contains code from zvbi-ntsc-cc closed caption decoder written by
 *  <timecop@japan.co.jp>, Mike Baker <mbm@linux.com>,
 *  Mark K. Kim <dev@cbreak.org>.
 *
 *  Thanks to Karol Zapolski for his support.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define _GNU_SOURCE 1

#include "libzvbi.h"
#include "dtvcc.h"

#ifdef ANDROID
#include "am_debug.h"
#else
//for buildroot begin
#undef AM_DEBUG
#define AM_DEBUG(_level,_fmt...) \
	do {\
		fprintf(stderr, "AM_DEBUG:(\"%s\" %d)", __FILE__, __LINE__);\
		fprintf(stderr, _fmt);\
		fprintf(stderr, "\n");\
	} while (0) \
//for buildroot end
#endif//end ANDROID


#define elements(array) (sizeof(array) / sizeof(array[0]))

#define LOG_TAG    "ZVBI"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#if __GNUC__ < 3
#  define likely(expr) (expr)
#  define unlikely(expr) (expr)
#else
#  define likely(expr) __builtin_expect(expr, 1)
#  define unlikely(expr) __builtin_expect(expr, 0)
#endif

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

#define VBI_RGBA(r, g, b)						\
	((((r) & 0xFF) << 0) | (((g) & 0xFF) << 8)			\
	 | (((b) & 0xFF) << 16) | (0xFF << 24))

/* These should be defined in inttypes.h. */
#ifndef PRId64
#  define PRId64 "lld"
#endif
#ifndef PRIu64
#  define PRIu64 "llu"
#endif
#ifndef PRIx64
#  define PRIx64 "llx"
#endif

extern void
vbi_transp_colormap(vbi_decoder *vbi, vbi_rgba *d, vbi_rgba *s, int entries);
extern void
vbi_send_event(vbi_decoder *vbi, vbi_event *ev);
static void
dtvcc_get_visible_windows(struct dtvcc_service *ds, int *cnt, struct dtvcc_window **windows);

/* EIA 608-B decoder. */

static int
printable			(int			c)
{
	if ((c & 0x7F) < 0x20)
		return '.';
	else
		return c & 0x7F;
}

static void
dump				(FILE *			fp,
				 const uint8_t *	buf,
				 unsigned int		n_bytes)
  _vbi_unused;

static void
dump				(FILE *			fp,
				 const uint8_t *	buf,
				 unsigned int		n_bytes)
{
	const unsigned int width = 16;
	unsigned int i;

	for (i = 0; i < n_bytes; i += width) {
		unsigned int end;
		unsigned int j;

		end = MIN (i + width, n_bytes);
		for (j = i; j < end; ++j)
			fprintf (fp, "%02x ", buf[j]);
		for (; j < i + width; ++j)
			fputs ("   ", fp);
		fputc (' ', fp);
		for (j = i; j < end; ++j) {
			int c = buf[j];
			fputc (printable (c), fp);
		}
		fputc ('\n', fp);
	}
}

#define log(verb, templ, args...) \
	log_message (2, /* print_errno */ FALSE, templ , ##args)

#define log_errno(verb, templ, args...) \
	log_message (2, /* print_errno */ TRUE, templ , ##args)

#define bug(templ, args...) \
	log_message (1, /* print_errno */ FALSE, "BUG: " templ , ##args)

static void
log_message			(unsigned int		verbosity,
				 vbi_bool		print_errno,
				 const char *		templ,
				 ...)
{
	if (1) {
		va_list ap;

		va_start (ap, templ);

		fprintf (stderr, "TVCC: ");
		vfprintf (stderr, templ, ap);

		if (print_errno) {
			fprintf (stderr, ": %s.\n",
				 strerror (errno));
		}

		va_end (ap);
	}
}

static void *
xmalloc				(size_t			size)
{
	void *p;

	p = malloc (size);
	if (NULL == p) {
		log(2, "no memory");
	}

	return p;
}

static char *
xasprintf			(const char *		templ,
				 ...)
{
	va_list ap;
	char *s;
	int r;

	va_start (ap, templ);

	r = vasprintf (&s, templ, ap);
	if (r < 0 || NULL == s) {
		return NULL;
	}

	va_end (ap);

	return s;
}

_vbi_inline void
vbi_char_copy_attr		(struct vbi_char *	cp1,
				 struct vbi_char *	cp2,
				 unsigned int		attr)
{
	if (attr & VBI_UNDERLINE)
		cp1->underline = cp2->underline;
	if (attr & VBI_ITALIC)
		cp1->italic = cp2->italic;
	if (attr & VBI_FLASH)
		cp1->flash = cp2->flash;
}

_vbi_inline void
vbi_char_clear_attr		(struct vbi_char *	cp,
				 unsigned int		attr)
{
	if (attr & VBI_UNDERLINE)
		cp->underline = 0;
	if (attr & VBI_ITALIC)
		cp->italic = 0;
	if (attr & VBI_FLASH)
		cp->flash = 0;
}

_vbi_inline void
vbi_char_set_attr		(struct vbi_char *	cp,
				 unsigned int		attr)
{
	if (attr & VBI_UNDERLINE)
		cp->underline = 1;
	if (attr & VBI_ITALIC)
		cp->italic = 1;
	if (attr & VBI_FLASH)
		cp->flash = 1;
}

_vbi_inline unsigned int
vbi_char_has_attr		(struct vbi_char *	cp,
				 unsigned int		attr)
{
	attr &= (VBI_UNDERLINE | VBI_ITALIC | VBI_FLASH);

	if (0 == cp->underline)
		attr &= ~VBI_UNDERLINE;
	if (0 == cp->italic)
		attr &= ~VBI_ITALIC;
	if (0 == cp->flash)
		attr &= ~VBI_FLASH;

	return attr;
}

_vbi_inline unsigned int
vbi_char_xor_attr		(struct vbi_char *	cp1,
				 struct vbi_char *	cp2,
				 unsigned int		attr)
{
	attr &= (VBI_UNDERLINE | VBI_ITALIC | VBI_FLASH);

	if (0 == (cp1->underline ^ cp2->underline))
		attr &= ~VBI_UNDERLINE;
	if (0 == (cp1->italic ^ cp2->italic))
		attr &= ~VBI_ITALIC;
	if (0 == (cp1->flash ^ cp2->flash))
		attr &= ~VBI_FLASH;

	return attr;
}

/* EIA 608-B Closed Caption decoder. */

static void
cc_timestamp_reset		(struct cc_timestamp *	ts)
{
	CLEAR (ts->sys);
	ts->pts = -1;
}

static vbi_bool
cc_timestamp_isset		(struct cc_timestamp *	ts)
{
	return (ts->pts >= 0 || 0 != (ts->sys.tv_sec | ts->sys.tv_usec));
}

static const vbi_color
cc_color_map [8] = {
	VBI_WHITE, VBI_GREEN, VBI_BLUE, VBI_CYAN,
	VBI_RED, VBI_YELLOW, VBI_MAGENTA, VBI_BLACK
};

static const int8_t
cc_pac_row_map [16] = {
	/* 0 */ 10,			/* 0x1040 */
	/* 1 */ -1,			/* no function */
	/* 2 */ 0, 1, 2, 3,		/* 0x1140 ... 0x1260 */
	/* 6 */ 11, 12, 13, 14,		/* 0x1340 ... 0x1460 */
	/* 10 */ 4, 5, 6, 7, 8, 9	/* 0x1540 ... 0x1760 */
};

static vbi_pgno
cc_channel_num			(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	return (ch - cd->channel) + 1;
}

/* Note 47 CFR 15.119 (h) Character Attributes: "(1) Transmission of
   Attributes. A character may be transmitted with any or all of four
   attributes: Color, italics, underline, and flash. All of these
   attributes are set by control codes included in the received
   data. An attribute will remain in effect until changed by another
   control code or until the end of the row is reached. Each row
   begins with a control code which sets the color and underline
   attributes. (White non-underlined is the default display attribute
   if no Preamble Address Code is received before the first character
   on an empty row.) Attributes are not affected by transparent spaces
   within a row. (i) All Mid-Row Codes and the Flash On command are
   spacing attributes which appear in the display just as if a
   standard space (20h) had been received. Preamble Address Codes are
   non-spacing and will not alter any attributes when used to position
   the cursor in the midst of a row of characters. (ii) The color
   attribute has the highest priority and can only be changed by the
   Mid-Row Code of another color. Italics has the next highest
   priority. If characters with both color and italics are desired,
   the italics Mid-Row Code must follow the color assignment. Any
   color Mid-Row Code will turn off italics. If the least significant
   bit of a Preamble Address Code or of a color or italics Mid-Row
   Code is a 1 (high), underlining is turned on. If that bit is a 0
   (low), underlining is off. (iii) The flash attribute is transmitted
   as a Miscellaneous Control Code. The Flash On command will not
   alter the status of the color, italics, or underline
   attributes. However, any coloror italics Mid-Row Code will turn off
   flash. (iv) Thus, for example, if a red, italicized, underlined,
   flashing character is desired, the attributes must be received in
   the following order: a red Mid-Row or Preamble Address Code, an
   italics Mid-Row Code with underline bit, and the Flash On
   command. The character will then be preceded by three spaces (two
   if red was assigned via a Preamble Address Code)."

   EIA 608-B Annex C.7 Preamble Address Codes and Tab Offsets
   (Regulatory/Preferred): "In general, Preamble Address Codes (PACs)
   have no immediate effect on the display. A major exception is the
   receipt of a PAC during roll-up captioning. In that case, if the
   base row designated in the PAC is not the same as the current base
   row, the display shall be moved immediately to the new base
   row. [...]


cc_channel_num                  (struct cc_decoder *    cd,
                                 struct cc_channel *    ch)
indenting PAC carries the attributes of white,
   non-italicized, and it sets underlining on or off. Tab Offset
   commands do not change these attributes. If an indenting PAC with
   underline ON is received followed by a Tab Offset and by text, the
   text shall be underlined (except as noted below). When a
   displayable character is received, it is deposited at the current
   cursor position. If there is already a displayable character in the
   column immediately to the left, the new character assumes the
   attributes of that character. The new character may be arriving as
   the result of an indenting PAC (with or without a Tab Offset), and
   that PAC may designate other attributes, but the new character is
   forced to assume the attributes of the character immediately to its
   left, and the PAC's attributes are ignored. If, when a displayable
   character is received, it overwrites an existing PAC or mid-row
   code, and there are already characters to the right of the new
   character, these existing characters shall assume the same
   attributes as the new character. This adoption can result in a
   whole caption row suddenly changing color, underline, italics,
   and/or flash attributes."

   EIA 608-B Annex C.14 Special Cases Regarding Attributes
   (Normative): "In most cases, Preamble Address Codes shall set
   attributes for the caption elements they address. It is
   theoretically possible for a service provider to use an indenting
   PAC to start a row at Column 5 or greater, and then to use
   Backspace to move the cursor to the left of the PAC into an area to
   which no attributes have been assigned. It is also possible for a
   roll-up row, having been created by a Carriage Return, to receive
   characters with no PAC used to set attributes. In these cases, and
   in any other case where no explicit attributes have been assigned,
   the display shall be white, non-underlined, non-italicized, and
   non-flashing. In case new displayable characters are received
   immediately after a Delete to End of Row (DER), the display
   attributes of the first deleted character shall remain in effect if
   there is a displayable character to the left of the cursor;
   otherwise, the most recently received PAC shall set the display
   attributes."

   47 CFR 15.119 (n) Glossary of terms: "(6) Displayable character:
   Any letter, number or symbol which is defined for on-screen
   display, plus the 20h space. [...] (13) Special characters:
   Displayable characters (except for "transparent space") [...]" */

static void
cc_format_row			(struct cc_decoder *	cd,
				 struct vbi_char *	cp,
				 struct cc_channel *	ch,
				 unsigned int		buffer,
				 unsigned int		row,
				 vbi_bool		to_upper,
				 vbi_bool		padding)
{
	struct vbi_char ac;
	unsigned int i;

	cd = cd; /* unused */

	/* 47 CFR 15.119 (h)(1). EIA 608-B Section 6.4. */
	CLEAR (ac);
	ac.foreground = VBI_WHITE;
	ac.background = VBI_BLACK;

	/* Shortcut. */
	if (0 == (ch->dirty[buffer] & (1 << row))) {
		vbi_char *end;

		ac.unicode = 0x20;
		ac.opacity = VBI_TRANSPARENT_SPACE;

		end = cp + CC_MAX_COLUMNS;
		if (padding)
			end += 2;

		while (cp < end)
			*cp++ = ac;

		return;
	}

	if (padding) {
		ac.unicode = 0x20;
		ac.opacity = VBI_TRANSPARENT_SPACE;
		*cp++ = ac;
	}

	/* EIA 608-B Section 6.4. */
	ac.opacity = VBI_OPAQUE;

	for (i = CC_FIRST_COLUMN - 1; i <= CC_LAST_COLUMN; ++i) {
		unsigned int color;
		unsigned int c;

		ac.unicode = 0x20;

		c = ch->buffer[buffer][row][i];
		if (0 == c) {
			if (padding
			    && VBI_TRANSPARENT_SPACE != cp[-1].opacity
			    && 0x20 != cp[-1].unicode) {
				/* Append a space with the same colors
				   and opacity (opaque or
				   transp. backgr.) as the text to the
				   left of it. */
				*cp++ = ac;
				/* We don't underline spaces, see
				   below. */
				vbi_char_clear_attr (cp - 1, -1);
			} else if (i > 0) {
				*cp++ = ac;
				cp[-1].opacity = VBI_TRANSPARENT_SPACE;
			}

			continue;
		} else if (c < 0x1040) {
			if (padding
			    && VBI_TRANSPARENT_SPACE == cp[-1].opacity) {
				/* Prepend a space with the same
				   colors and opacity (opaque or
				   transp. backgr.) as the text to the
				   right of it. */
				cp[-1] = ac;
				/* We don't underline spaces, see
				   below. */
				vbi_char_clear_attr (cp - 1, -1);
			}

			if ((c >= 'a' && c <= 'z')
			    || 0x7E == c /* n with tilde */) {
				/* We do not force these characters to
				   upper case because the standard
				   character set includes upper case
				   versions of these characters and
				   lower case was probably
				   deliberately transmitted. */
				ac.unicode = vbi_caption_unicode
					(c, /* to_upper */ FALSE);
			} else {
				ac.unicode = vbi_caption_unicode
					(c, to_upper);
			}
		} else if (c < 0x1120) {
			/* Preamble Address Codes -- 001 crrr  1ri xxxu */

			/* PAC is a non-spacing attribute and only
			   stored in the buffer at the addressed
			   column minus one if it replaces a
			   transparent space (EIA 608-B Annex C.7,
			   C.14). There's always a transparent space
			   to the left of the first column but we show
			   this zeroth column only if padding is
			   enabled. */
			if (padding
			    && VBI_TRANSPARENT_SPACE != cp[-1].opacity
			    && 0x20 != cp[-1].unicode) {
				/* See 0 == c. */
				*cp++ = ac;
				vbi_char_clear_attr (cp - 1, -1);
			} else if (i > 0) {
				*cp++ = ac;
				cp[-1].opacity = VBI_TRANSPARENT_SPACE;
			}

			vbi_char_clear_attr (&ac, VBI_UNDERLINE | VBI_ITALIC);
			if (c & 0x0001)
				vbi_char_set_attr (&ac, VBI_UNDERLINE);
			if (c & 0x0010) {
				ac.foreground = VBI_WHITE;
			} else {
				color = (c >> 1) & 7;
				if (7 == color) {
					ac.foreground = VBI_WHITE;
					vbi_char_set_attr (&ac, VBI_ITALIC);
				} else {
					ac.foreground = cc_color_map[color];
				}
			}

			continue;
		} else if (c < 0x1130) {
			/* Mid-Row Codes -- 001 c001  010 xxxu */
			/* 47 CFR 15.119 Mid-Row Codes table,
			   (h)(1)(ii), (h)(1)(iii). */

			/* 47 CFR 15.119 (h)(1)(i), EIA 608-B Section
			   6.2: Mid-Row codes, FON, BT, FA and FAU are
			   set-at spacing attributes. */

			vbi_char_clear_attr (&ac, -1);
			if (c & 0x0001)
				vbi_char_set_attr (&ac, VBI_UNDERLINE);
			color = (c >> 1) & 7;
			if (7 == color) {
				vbi_char_set_attr (&ac, VBI_ITALIC);
			} else {
				ac.foreground = cc_color_map[color];
			}
		} else if (c < 0x1220) {
			/* Special Characters -- 001 c001  011 xxxx */
			/* 47 CFR 15.119 Character Set Table. */

			if (padding
			    && VBI_TRANSPARENT_SPACE == cp[-1].opacity) {
				cp[-1] = ac;
				vbi_char_clear_attr (cp - 1, -1);
			}

			/* Note we already stored 0 instead of 0x1139
			   (transparent space) in the ch->buffer. */
			ac.unicode = vbi_caption_unicode (c, to_upper);
		} else if (c < 0x1428) {
			/* Extended Character Set -- 001 c01x  01x xxxx */
			/* EIA 608-B Section 6.4.2 */

			if (padding
			    && VBI_TRANSPARENT_SPACE == cp[-1].opacity) {
				cp[-1] = ac;
				vbi_char_clear_attr (cp - 1, -1);
			}

			/* We do not force these characters to upper
			   case because the extended character set
			   includes upper case versions of all letters
			   and lower case was probably deliberately
			   transmitted. */
			ac.unicode = vbi_caption_unicode
				(c, /* to_upper */ FALSE);
		} else if (c < 0x172D) {
			/* FON Flash On -- 001 c10f  010 1000 */
			/* 47 CFR 15.119 (h)(1)(iii). */

			vbi_char_set_attr (&ac, VBI_FLASH);
		} else if (c < 0x172E) {
			/* BT Background Transparent -- 001 c111  010 1101 */
			/* EIA 608-B Section 6.4. */

			ac.opacity = VBI_TRANSPARENT_FULL;
		} else if (c <= 0x172F) {
			/* FA Foreground Black -- 001 c111  010 111u */
			/* EIA 608-B Section 6.4. */

			if (c & 0x0001)
				vbi_char_set_attr (&ac, VBI_UNDERLINE);
			ac.foreground = VBI_BLACK;
		}

		*cp++ = ac;

		/* 47 CFR 15.119 and EIA 608-B are silent about
		   underlined spaces, but considering the example in
		   47 CFR (h)(1)(iv) which would produce something
		   ugly like "__text" I suppose we should not
		   underline them. For good measure we also clear the
		   invisible italic and flash attribute. */
		if (0x20 == ac.unicode)
			vbi_char_clear_attr (cp - 1, -1);
	}

	if (padding) {
		ac.unicode = 0x20;
		vbi_char_clear_attr (&ac, -1);

		if (VBI_TRANSPARENT_SPACE != cp[-1].opacity
		    && 0x20 != cp[-1].unicode) {
			*cp = ac;
		} else {
			ac.opacity = VBI_TRANSPARENT_SPACE;
			*cp = ac;
		}
	}
}

typedef enum {
	/**
	 * 47 CFR Section 15.119 requires caption decoders to roll
	 * caption smoothly: Nominally each character cell has a
	 * height of 13 field lines. When this flag is set the current
	 * caption should be displayed with a vertical offset of 12
	 * field lines, and after every 1001 / 30000 seconds the
	 * caption overlay should move up by one field line until the
	 * offset is zero. The roll rate should be no more than 0.433
	 * seconds/row for other character cell heights.
	 *
	 * The flag may be set again before the offset returned to
	 * zero. The caption overlay should jump to offset 12 in this
	 * case regardless.
	 */
	VBI_START_ROLLING	= (1 << 0)
} vbi_cc_page_flags;

static void
cc_display_event		(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 vbi_cc_page_flags	flags)
{
	cd = cd; /* unused */
	ch = ch;
	flags = flags;
}

/* This decoder is mainly designed to overlay caption onto live video,
   but to create transcripts we also offer an event every time a line
   of caption is complete. The event occurs when certain control codes
   are received.

   In POP_ON mode we send the event upon reception of EOC, which swaps
   the displayed and non-displayed memory.

   In ROLL_UP and TEXT mode captioners are not expected to display new
   text by erasing and overwriting a row with PAC, TOx, BS and DER so
   we ignore these codes. In ROLL_UP mode CR, EDM, EOC, RCL and RDC
   complete a line. CR moves the cursor to a new row, EDM erases the
   displayed memory. The remaining codes switch to POP_ON or PAINT_ON
   mode. In TEXT mode CR and TR are our line completion indicators. CR
   works as above and TR erases the displayed memory. EDM, EOC, RDC,
   RCL and RUx have no effect on TEXT buffers.

   In PAINT_ON mode RDC never erases the displayed memory and CR has
   no function. Instead captioners can freely position the cursor and
   erase or overwrite (parts of) rows with PAC, TOx, BS and DER, or
   erase all rows with EDM. We send an event on PAC, EDM, EOC, RCL and
   RUx, provided the characters (including spacing attributes) in the
   current row changed since the last event. PAC is the only control
   code which can move the cursor to the left and/or to a new row, and
   likely to introduce a new line. EOC, RCL and RUx switch to POP_ON
   or ROLL_UP mode. */

static void
cc_stream_event			(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		first_row,
				 unsigned int		last_row)
{
	vbi_pgno channel;
	unsigned int row;

	channel = cc_channel_num (cd, ch);

	for (row = first_row; row <= last_row; ++row) {
		struct vbi_char text[36];
		unsigned int end;

		cc_format_row (cd, text, ch,
			       ch->displayed_buffer,
			       row, /* to_upper */ FALSE,
			       /* padding */ FALSE);

		for (end = 32; end > 0; --end) {
			if (VBI_TRANSPARENT_SPACE != text[end - 1].opacity)
				break;
		}

		if (0 == end)
			continue;
	}

	cc_timestamp_reset (&ch->timestamp_c0);
}

static void
cc_put_char			(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 int			c,
				 vbi_bool		displayable,
				 vbi_bool		backspace)
{
	uint16_t *text;
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int column;

	/* 47 CFR Section 15.119 (f)(1), (f)(2), (f)(3). */
	curr_buffer = ch->displayed_buffer
		^ (CC_MODE_POP_ON == ch->mode);

	row = ch->curr_row;
	column = ch->curr_column;

	if (unlikely (backspace)) {
		/* 47 CFR 15.119 (f)(1)(vi), (f)(2)(ii),
		   (f)(3)(i). EIA 608-B Section 6.4.2, 7.4. */
		if (column > CC_FIRST_COLUMN)
			--column;
	} else {
		/* 47 CFR 15.119 (f)(1)(v), (f)(1)(vi), (f)(2)(ii),
		   (f)(3)(i). EIA 608-B Section 7.4. */
		if (column < CC_LAST_COLUMN)
			ch->curr_column = column + 1;
	}

	text = &ch->buffer[curr_buffer][row][0];
	text[column] = c;

	/* Send a display update event when the displayed buffer of
	   the current channel changed, but no more than once for each
	   pair of Closed Caption bytes. */
	/* XXX This may not be a visible change, but such cases are
	   rare and we'd need something close to format_row() to be
	   sure. */
	if (CC_MODE_POP_ON != ch->mode) {
		cd->event_pending = ch;
	}

	if (likely (displayable)) {
		/* Note EIA 608-B Annex C.7, C.14. */
		if (CC_FIRST_COLUMN == column
		    || 0 == text[column - 1]) {
			/* Note last_pac may be 0 as well. */
			text[column - 1] = ch->last_pac;
		}

		if (c >= 'a' && c <= 'z') {
			ch->uppercase_predictor = 0;
		} else if (c >= 'A' && c <= 'Z') {
			unsigned int up;

			up = ch->uppercase_predictor + 1;
			if (up > 0)
				ch->uppercase_predictor = up;
		}
	} else if (unlikely (0 == c)) {
		unsigned int i;

		/* This is Special Character "Transparent space". */

		for (i = CC_FIRST_COLUMN; i <= CC_LAST_COLUMN; ++i)
			c |= ch->buffer[curr_buffer][row][i];

		ch->dirty[curr_buffer] &= ~((0 == c) << row);

		return;
	}

	assert (sizeof (ch->dirty[0]) * 8 - 1 >= CC_MAX_ROWS);
	ch->dirty[curr_buffer] |= 1 << row;

	if (ch->timestamp_c0.pts < 0
	    && 0 == (ch->timestamp_c0.sys.tv_sec
		     | ch->timestamp_c0.sys.tv_usec)) {
		ch->timestamp_c0 = cd->timestamp;
	}
}

static void
cc_ext_control_code		(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		c2)
{
	unsigned int column;

	switch (c2) {
	case 0x21: /* TO1 */
	case 0x22: /* TO2 */
	case 0x23: /* TO3 Tab Offset -- 001 c111  010 00xx */
		/* 47 CFR 15.119 (e)(1)(ii). EIA 608-B Section 7.4,
		   Annex C.7. */
		column = ch->curr_column + (c2 & 3);
		ch->curr_column = MIN (column,
				       (unsigned int) CC_LAST_COLUMN);
		break;

	case 0x24: /* Select standard character set in normal size */
	case 0x25: /* Select standard character set in double size */
	case 0x26: /* Select first private character set */
	case 0x27: /* Select second private character set */
	case 0x28: /* Select character set GB 2312-80 (Chinese) */
	case 0x29: /* Select character set KSC 5601-1987 (Korean) */
	case 0x2A: /* Select first registered character set. */
		/* EIA 608-B Section 6.3 Closed Group Extensions. */
		break;

	case 0x2D: /* BT Background Transparent -- 001 c111  010 1101 */
	case 0x2E: /* FA Foreground Black -- 001 c111  010 1110 */
	case 0x2F: /* FAU Foregr. Black Underl. -- 001 c111  010 1111 */
		/* EIA 608-B Section 6.2. */
		cc_put_char (cd, ch, 0x1700 | c2,
			     /* displayable */ FALSE,
			     /* backspace */ TRUE);
		break;

	default:
		/* 47 CFR Section 15.119 (j): Ignore. */
		break;
	}
}

/* Send a stream event if the current row has changed since the last
   stream event. This is necessary in paint-on mode where CR has no
   function and captioners can freely position the cursor to erase or
   overwrite (parts of) rows. */
static void
cc_stream_event_if_changed	(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int i;

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	if (0 == (ch->dirty[curr_buffer] & (1 << row)))
		return;

	for (i = CC_FIRST_COLUMN; i <= CC_LAST_COLUMN; ++i) {
		unsigned int c1;
		unsigned int c2;

		c1 = ch->buffer[curr_buffer][row][i];
		if (c1 >= 0x1040) {
			if (c1 < 0x1120) {
				c1 = 0; /* PAC -- non-spacing */
			} else if (c1 < 0x1130 || c1 >= 0x1428) {
				/* MR, FON, BT, FA, FAU -- spacing */
				c1 = 0x20;
			}
		}

		c2 = ch->buffer[2][row][i];
		if (c2 >= 0x1040) {
			if (c2 < 0x1120) {
				c2 = 0;
			} else if (c2 < 0x1130 || c2 >= 0x1428) {
				c1 = 0x20;
			}
		}

		if (c1 != c2) {
			cc_stream_event (cd, ch, row, row);

			memcpy (ch->buffer[2][row],
				ch->buffer[curr_buffer][row],
				sizeof (ch->buffer[0][0]));

			ch->dirty[2] = ch->dirty[curr_buffer];

			return;
		}
	}
}

static void
cc_end_of_caption		(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* EOC End Of Caption -- 001 c10f  010 1111 */

	curr_buffer = ch->displayed_buffer;

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
	case CC_MODE_POP_ON:
		break;

	case CC_MODE_ROLL_UP:
		row = ch->curr_row;
		if (0 != (ch->dirty[curr_buffer] & (1 << row)))
			cc_stream_event (cd, ch, row, row);
		break;

	case CC_MODE_PAINT_ON:
		cc_stream_event_if_changed (cd, ch);
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	ch->displayed_buffer = curr_buffer ^= 1;

	/* 47 CFR Section 15.119 (f)(2). */
	ch->mode = CC_MODE_POP_ON;

	if (0 != ch->dirty[curr_buffer]) {
		ch->timestamp_c0 = cd->timestamp;

		cc_stream_event (cd, ch,
				 CC_FIRST_ROW,
				 CC_LAST_ROW);

		cc_display_event (cd, ch, 0);
	}
}

static void
cc_carriage_return		(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int window_rows;
	unsigned int first_row;

	/* CR Carriage Return -- 001 c10f  010 1101 */

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
		return;

	case CC_MODE_ROLL_UP:
		/* 47 CFR Section 15.119 (f)(1)(iii). */
		ch->curr_column = CC_FIRST_COLUMN;

		/* 47 CFR 15.119 (f)(1): "The cursor always remains on
		   the base row." */

		/* XXX Spec? */
		ch->last_pac = 0;

		/* No event if the buffer contains only
		   TRANSPARENT_SPACEs. */
		if (0 == ch->dirty[curr_buffer])
			return;

		window_rows = MIN (row + 1 - CC_FIRST_ROW,
				   ch->window_rows);
		break;

	case CC_MODE_POP_ON:
	case CC_MODE_PAINT_ON:
		/* 47 CFR 15.119 (f)(2)(i), (f)(3)(i): No effect. */
		return;

	case CC_MODE_TEXT:
		/* 47 CFR Section 15.119 (f)(1)(iii). */
		ch->curr_column = CC_FIRST_COLUMN;

		/* XXX Spec? */
		ch->last_pac = 0;

		/* EIA 608-B Section 7.4: "When Text Mode has
		   initially been selected and the specified Text
		   memory is empty, the cursor starts at the topmost
		   row, Column 1, and moves down to Column 1 on the
		   next row each time a Carriage Return is received
		   until the last available row is reached. A variety
		   of methods may be used to accomplish the scrolling,
		   provided that the text is legible while moving. For
		   example, as soon as all of the available rows of
		   text are on the screen, Text Mode switches to the
		   standard roll-up type of presentation." */

		if (CC_LAST_ROW != row) {
			if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
				cc_stream_event (cd, ch, row, row);
			}

			ch->curr_row = row + 1;

			return;
		}

		/* No event if the buffer contains all
		   TRANSPARENT_SPACEs. */
		if (0 == ch->dirty[curr_buffer])
			return;

		window_rows = CC_MAX_ROWS;

		break;
	}

	/* 47 CFR Section 15.119 (f)(1)(iii). In roll-up mode: "Each
	   time a Carriage Return is received, the text in the top row
	   of the window is erased from memory and from the display or
	   scrolled off the top of the window. The remaining rows of
	   text are each rolled up into the next highest row in the
	   window, leaving the base row blank and ready to accept new
	   text." */

	if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
		cc_stream_event (cd, ch, row, row);
	}

	first_row = row + 1 - window_rows;
	memmove (ch->buffer[curr_buffer][first_row],
		 ch->buffer[curr_buffer][first_row + 1],
		 (window_rows - 1) * sizeof (ch->buffer[0][0]));

	ch->dirty[curr_buffer] >>= 1;

	memset (ch->buffer[curr_buffer][row], 0,
		sizeof (ch->buffer[0][0]));

	cc_display_event (cd, ch, VBI_START_ROLLING);
}

static void
cc_erase_memory			(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		buffer)
{
	if (0 != ch->dirty[buffer]) {
		CLEAR (ch->buffer[buffer]);

		ch->dirty[buffer] = 0;

		if (buffer == ch->displayed_buffer)
			cc_display_event (cd, ch, 0);
	}
}

static void
cc_erase_displayed_memory	(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int row;

	/* EDM Erase Displayed Memory -- 001 c10f  010 1100 */

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
		/* We have not received EOC, RCL, RDC or RUx yet, but
		   ch is valid. */
		break;

	case CC_MODE_ROLL_UP:
		row = ch->curr_row;
		if (0 != (ch->dirty[ch->displayed_buffer] & (1 << row)))
			cc_stream_event (cd, ch, row, row);
		break;

	case CC_MODE_PAINT_ON:
		cc_stream_event_if_changed (cd, ch);
		break;

	case CC_MODE_POP_ON:
		/* Nothing to do. */
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	/* May send a display event. */
	cc_erase_memory (cd, ch, ch->displayed_buffer);
}

static void
cc_text_restart			(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* TR Text Restart -- 001 c10f  010 1010 */

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	/* ch->mode is invariably CC_MODE_TEXT. */

	if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
		cc_stream_event (cd, ch, row, row);
	}

	/* EIA 608-B Section 7.4. */
	/* May send a display event. */
	cc_erase_memory (cd, ch, ch->displayed_buffer);

	/* EIA 608-B Section 7.4. */
	ch->curr_row = CC_FIRST_ROW;
	ch->curr_column = CC_FIRST_COLUMN;
}

static void
cc_resume_direct_captioning	(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* RDC Resume Direct Captioning -- 001 c10f  010 1001 */

	/* 47 CFR 15.119 (f)(1)(x), (f)(2)(vi) and EIA 608-B Annex
	   B.7: Does not erase memory, does not move the cursor when
	   resuming after a Text transmission.

	   XXX If ch->mode is unknown, roll-up or pop-on, what shall
	   we do if no PAC is received between RDC and the text? */

	curr_buffer = ch->displayed_buffer;
	row = ch->curr_row;

	switch (ch->mode) {
	case CC_MODE_ROLL_UP:
		if (0 != (ch->dirty[curr_buffer] & (1 << row)))
			cc_stream_event (cd, ch, row, row);

		/* fall through */

	case CC_MODE_UNKNOWN:
	case CC_MODE_POP_ON:
		/* No change since last stream_event(). */
		memcpy (ch->buffer[2], ch->buffer[curr_buffer],
			sizeof (ch->buffer[2]));
		break;

	case CC_MODE_PAINT_ON:
		/* Mode continues. */
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	ch->mode = CC_MODE_PAINT_ON;
}

static void
cc_resize_window		(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		new_rows)
{
	unsigned int curr_buffer;
	unsigned int max_rows;
	unsigned int old_rows;
	unsigned int row1;

	curr_buffer = ch->displayed_buffer;

	/* No event if the buffer contains all TRANSPARENT_SPACEs. */
	if (0 == ch->dirty[curr_buffer])
		return;

	row1 = ch->curr_row + 1;
	max_rows = row1 - CC_FIRST_ROW;
	old_rows = MIN (ch->window_rows, max_rows);
	new_rows = MIN (new_rows, max_rows);

	/* Nothing to do unless the window shrinks. */
	if (0 == new_rows || new_rows >= old_rows)
		return;

	memset (&ch->buffer[curr_buffer][row1 - old_rows][0], 0,
		(old_rows - new_rows)
		* sizeof (ch->buffer[0][0]));

	ch->dirty[curr_buffer] &= -1 << (row1 - new_rows);

	cc_display_event (cd, ch, 0);
}

static void
cc_roll_up_caption		(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		c2)
{
	unsigned int window_rows;

	/* Roll-Up Captions -- 001 c10f  010 01xx */

	window_rows = (c2 & 7) - 3; /* 2, 3, 4 */

	switch (ch->mode) {
	case CC_MODE_ROLL_UP:
		/* 47 CFR 15.119 (f)(1)(iv). */
		/* May send a display event. */
		cc_resize_window (cd, ch, window_rows);

		/* fall through */

	case CC_MODE_UNKNOWN:
		ch->mode = CC_MODE_ROLL_UP;
		ch->window_rows = window_rows;

		/* 47 CFR 15.119 (f)(1)(ix): No cursor movements,
		   no memory erasing. */

		break;

	case CC_MODE_PAINT_ON:
		cc_stream_event_if_changed (cd, ch);

		/* fall through */

	case CC_MODE_POP_ON:
		ch->mode = CC_MODE_ROLL_UP;
		ch->window_rows = window_rows;

		/* 47 CFR 15.119 (f)(1)(ii). */
		ch->curr_row = CC_LAST_ROW;
		ch->curr_column = CC_FIRST_COLUMN;

		/* 47 CFR 15.119 (f)(1)(x). */
		/* May send a display event. */
		cc_erase_memory (cd, ch, ch->displayed_buffer);
		cc_erase_memory (cd, ch, ch->displayed_buffer ^ 1);

		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}
}

static void
cc_delete_to_end_of_row		(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int curr_buffer;
	unsigned int row;

	/* DER Delete To End Of Row -- 001 c10f  010 0100 */

	/* 47 CFR 15.119 (f)(1)(vii), (f)(2)(iii), (f)(3)(ii) and EIA
	   608-B Section 7.4: In all caption modes and Text mode
	   "[the] Delete to End of Row command will erase from memory
	   any characters or control codes starting at the current
	   cursor location and in all columns to its right on the same
	   row." */

	curr_buffer = ch->displayed_buffer
		^ (CC_MODE_POP_ON == ch->mode);

	row = ch->curr_row;

	/* No event if the row contains only TRANSPARENT_SPACEs. */
	if (0 != (ch->dirty[curr_buffer] & (1 << row))) {
		unsigned int column;
		unsigned int i;
		uint16_t c;

		column = ch->curr_column;

		memset (&ch->buffer[curr_buffer][row][column], 0,
			(CC_LAST_COLUMN - column + 1)
			* sizeof (ch->buffer[0][0][0]));

		c = 0;
		for (i = CC_FIRST_COLUMN; i < column; ++i)
			c |= ch->buffer[curr_buffer][row][i];

		ch->dirty[curr_buffer] &= ~((0 == c) << row);

		cc_display_event (cd, ch, 0);
	}
}

static void
cc_backspace			(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int curr_buffer;
	unsigned int row;
	unsigned int column;

	/* BS Backspace -- 001 c10f  010 0001 */

	/* 47 CFR Section 15.119 (f)(1)(vi), (f)(2)(ii), (f)(3)(i) and
	   EIA 608-B Section 7.4. */
	column = ch->curr_column;
	if (column <= CC_FIRST_COLUMN)
		return;

	ch->curr_column = --column;

	curr_buffer = ch->displayed_buffer
		^ (CC_MODE_POP_ON == ch->mode);

	row = ch->curr_row;

	/* No event if there's no visible effect. */
	if (0 != ch->buffer[curr_buffer][row][column]) {
		unsigned int i;
		uint16_t c;

		/* 47 CFR 15.119 (f), (f)(1)(vi), (f)(2)(ii) and EIA
		   608-B Section 7.4. */
		ch->buffer[curr_buffer][row][column] = 0;

		c = 0;
		for (i = CC_FIRST_COLUMN; i <= CC_LAST_COLUMN; ++i)
			c |= ch->buffer[curr_buffer][row][i];

		ch->dirty[curr_buffer] &= ~((0 == c) << row);

		cc_display_event (cd, ch, 0);
	}
}

static void
cc_resume_caption_loading	(struct cc_decoder *	cd,
				 struct cc_channel *	ch)
{
	unsigned int row;

	/* RCL Resume Caption Loading -- 001 c10f  010 0000 */

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
	case CC_MODE_POP_ON:
		break;

	case CC_MODE_ROLL_UP:
		row = ch->curr_row;
		if (0 != (ch->dirty[ch->displayed_buffer] & (1 << row)))
			cc_stream_event (cd, ch, row, row);
		break;

	case CC_MODE_PAINT_ON:
		cc_stream_event_if_changed (cd, ch);
		break;

	case CC_MODE_TEXT:
		/* Not reached. (ch is a caption channel.) */
		return;
	}

	/* 47 CFR 15.119 (f)(1)(x): Does not erase memory.
	   (f)(2)(iv): Cursor position remains unchanged. */

	ch->mode = CC_MODE_POP_ON;
}



/* Note curr_ch is invalid if UNKNOWN_CC_CHANNEL == cd->cc.curr_ch_num. */
static struct cc_channel *
cc_switch_channel		(struct cc_decoder *	cd,
				 struct cc_channel *	curr_ch,
				 vbi_pgno		new_ch_num,
				 enum field_num		f)
{
	struct cc_channel *new_ch;

	if (UNKNOWN_CC_CHANNEL != cd->curr_ch_num[f]
	    && CC_MODE_UNKNOWN != curr_ch->mode) {
		/* XXX Force a display update if we do not send events
		   on every display change. */
	}

	cd->curr_ch_num[f] = new_ch_num;
	new_ch = &cd->channel[new_ch_num - VBI_CAPTION_CC1];

	return new_ch;
}

/* Note ch is invalid if UNKNOWN_CC_CHANNEL == cd->cc.curr_ch_num[f]. */
static void
cc_misc_control_code		(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		c2,
				 unsigned int		ch_num0,
				 enum field_num		f)
{
	unsigned int new_ch_num;

	/* Misc Control Codes -- 001 c10f  010 xxxx */

	/* c = channel (0 -> CC1/CC3/T1/T3, 1 -> CC2/CC4/T2/T4)
	     -- 47 CFR Section 15.119, EIA 608-B Section 7.7.
	   f = field (0 -> F1, 1 -> F2)
	     -- EIA 608-B Section 8.4, 8.5. */

	/* XXX The f flag is intended to detect accidential field
	   swapping and we should use it for that purpose. */

	switch (c2 & 15) {
	case 0:	/* RCL Resume Caption Loading -- 001 c10f  010 0000 */
		/* 47 CFR 15.119 (f)(2) and EIA 608-B Section 7.7. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = cc_switch_channel (cd, ch, new_ch_num, f);
		cc_resume_caption_loading (cd, ch);
		break;

	case 1: /* BS Backspace -- 001 c10f  010 0001 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
		    || CC_MODE_UNKNOWN == ch->mode)
			break;
		cc_backspace (cd, ch);
		break;

	case 2: /* reserved (formerly AOF Alarm Off) */
	case 3: /* reserved (formerly AON Alarm On) */
		break;

	case 4: /* DER Delete To End Of Row -- 001 c10f  010 0100 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
		    || CC_MODE_UNKNOWN == ch->mode)
			break;
		cc_delete_to_end_of_row (cd, ch);
		break;

	case 5: /* RU2 */
	case 6: /* RU3 */
	case 7: /* RU4 Roll-Up Captions -- 001 c10f  010 01xx */
		/* 47 CFR 15.119 (f)(1) and EIA 608-B Section 7.7. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = cc_switch_channel (cd, ch, new_ch_num, f);
		cc_roll_up_caption (cd, ch, c2);
		break;

	case 8: /* FON Flash On -- 001 c10f  010 1000 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
		    || CC_MODE_UNKNOWN == ch->mode)
			break;

		/* 47 CFR 15.119 (h)(1)(i): Spacing attribute. */
		cc_put_char (cd, ch, 0x1428,
			     /* displayable */ FALSE,
			     /* backspace */ FALSE);
		break;

	case 9: /* RDC Resume Direct Captioning -- 001 c10f  010 1001 */
		/* 47 CFR 15.119 (f)(3) and EIA 608-B Section 7.7. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = cc_switch_channel (cd, ch, new_ch_num, f);
		cc_resume_direct_captioning (cd, ch);
		break;

	case 10: /* TR Text Restart -- 001 c10f  010 1010 */
		/* EIA 608-B Section 7.4. */
		new_ch_num = VBI_CAPTION_T1 + (ch_num0 & 3);
		ch = cc_switch_channel (cd, ch, new_ch_num, f);
		cc_text_restart (cd, ch);
		break;

	case 11: /* RTD Resume Text Display -- 001 c10f  010 1011 */
		/* EIA 608-B Section 7.4. */
		new_ch_num = VBI_CAPTION_T1 + (ch_num0 & 3);
		ch = cc_switch_channel (cd, ch, new_ch_num, f);
		/* ch->mode is invariably CC_MODE_TEXT. */
		break;

	case 12: /* EDM Erase Displayed Memory -- 001 c10f  010 1100 */
		/* 47 CFR 15.119 (f). EIA 608-B Section 7.7 and Annex
		   B.7: "[The] command shall be acted upon as
		   appropriate for caption processing without
		   terminating the Text Mode data stream." */

		/* We need not check cd->curr_ch_num because bit 2 is
		   implied, bit 1 is the known field number and bit 0
		   is coded in the control code. */
		ch = &cd->channel[ch_num0 & 3];

		cc_erase_displayed_memory (cd, ch);

		break;

	case 13: /* CR Carriage Return -- 001 c10f  010 1101 */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f])
			break;
		cc_carriage_return (cd, ch);
		break;

	case 14: /* ENM Erase Non-Displayed Memory -- 001 c10f  010 1110 */
		/* 47 CFR 15.119 (f)(2)(v). EIA 608-B Section 7.7 and
		   Annex B.7: "[The] command shall be acted upon as
		   appropriate for caption processing without
		   terminating the Text Mode data stream." */

		/* See EDM. */
		ch = &cd->channel[ch_num0 & 3];

		cc_erase_memory (cd, ch, ch->displayed_buffer ^ 1);

		break;

	case 15: /* EOC End Of Caption -- 001 c10f  010 1111 */
		/* 47 CFR 15.119 (f), (f)(2), (f)(3)(iv) and EIA 608-B
		   Section 7.7, Annex C.11. */
		new_ch_num = VBI_CAPTION_CC1 + (ch_num0 & 3);
		ch = cc_switch_channel (cd, ch, new_ch_num, f);
		cc_end_of_caption (cd, ch);
		break;
	}
}

static void
cc_move_window			(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		new_base_row)
{
	uint8_t *base;
	unsigned int curr_buffer;
	unsigned int bytes_per_row;
	unsigned int old_max_rows;
	unsigned int new_max_rows;
	unsigned int copy_bytes;
	unsigned int erase_begin;
	unsigned int erase_end;

	curr_buffer = ch->displayed_buffer;

	/* No event if we do not move the window or the buffer
	   contains only TRANSPARENT_SPACEs. */
	if (new_base_row == ch->curr_row
	    || 0 == ch->dirty[curr_buffer])
		return;

	base = (void *) &ch->buffer[curr_buffer][CC_FIRST_ROW][0];
	bytes_per_row = sizeof (ch->buffer[0][0]);

	old_max_rows = ch->curr_row + 1 - CC_FIRST_ROW;
	new_max_rows = new_base_row + 1 - CC_FIRST_ROW;
	copy_bytes = MIN (MIN (old_max_rows, new_max_rows),
			  ch->window_rows) * bytes_per_row;

	if (new_base_row < ch->curr_row) {
		erase_begin = (new_base_row + 1) * bytes_per_row;
		erase_end = (ch->curr_row + 1) * bytes_per_row;

		memmove (base + erase_begin - copy_bytes,
			 base + erase_end - copy_bytes, copy_bytes);

		ch->dirty[curr_buffer] >>= ch->curr_row - new_base_row;
	} else {
		erase_begin = (ch->curr_row + 1) * bytes_per_row
			- copy_bytes;
		erase_end = (new_base_row + 1) * bytes_per_row
			- copy_bytes;

		memmove (base + erase_end,
			 base + erase_begin, copy_bytes);

		ch->dirty[curr_buffer] <<= new_base_row - ch->curr_row;
		ch->dirty[curr_buffer] &= CC_ALL_ROWS_MASK;
	}

	memset (base + erase_begin, 0, erase_end - erase_begin);

	cc_display_event (cd, ch, 0);
}

static void
cc_preamble_address_code	(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 unsigned int		c1,
				 unsigned int		c2)
{
	unsigned int row;

	/* PAC Preamble Address Codes -- 001 crrr  1ri xxxu */

	row = cc_pac_row_map[(c1 & 7) * 2 + ((c2 >> 5) & 1)];
	if ((int) row < 0)
		return;

	switch (ch->mode) {
	case CC_MODE_UNKNOWN:
		return;

	case CC_MODE_ROLL_UP:
		/* EIA 608-B Annex C.4. */
		if (ch->window_rows > row + 1)
			row = ch->window_rows - 1;

		/* 47 CFR Section 15.119 (f)(1)(ii). */
		/* May send a display event. */
		cc_move_window (cd, ch, row);

		ch->curr_row = row;

		break;

	case CC_MODE_PAINT_ON:
		cc_stream_event_if_changed (cd, ch);

		/* fall through */

	case CC_MODE_POP_ON:
		/* XXX 47 CFR 15.119 (f)(2)(i), (f)(3)(i): In Pop-on
		   and paint-on mode "Preamble Address Codes can be
		   used to move the cursor around the screen in random
		   order to place captions on Rows 1 to 15." We do not
		   have a limit on the number of displayable rows, but
		   as EIA 608-B Annex C.6 points out, if more than
		   four rows must be displayed they were probably
		   received in error and we should respond
		   accordingly. */

		/* 47 CFR Section 15.119 (d)(1)(i) and EIA 608-B Annex
		   C.7. */
		ch->curr_row = row;

		break;

	case CC_MODE_TEXT:
		/* 47 CFR 15.119 (e)(1) and EIA 608-B Section 7.4:
		   Does not change the cursor row. */
		break;
	}

	if (c2 & 0x10) {
		/* 47 CFR 15.119 (e)(1)(i) and EIA 608-B Table 71. */
		ch->curr_column = CC_FIRST_COLUMN + (c2 & 0x0E) * 2;
	}

	/* PAC is a non-spacing attribute for the next character, see
	   cc_put_char(). */
	ch->last_pac = 0x1000 | c2;
}

static void
cc_control_code			(struct cc_decoder *	cd,
				 unsigned int		c1,
				 unsigned int		c2,
				 enum field_num		f)
{
	struct cc_channel *ch;
	unsigned int ch_num0;

	/* Caption / text, field 1 / 2, primary / secondary channel. */
	ch_num0 = (((cd->curr_ch_num[f] - VBI_CAPTION_CC1) & 4)
		   + f * 2
		   + ((c1 >> 3) & 1));

	/* Note ch is invalid if UNKNOWN_CC_CHANNEL ==
	   cd->curr_ch_num[f]. */
	ch = &cd->channel[ch_num0];

	if (c2 >= 0x40) {
		/* Preamble Address Codes -- 001 crrr  1ri xxxu */
		if (UNKNOWN_CC_CHANNEL != cd->curr_ch_num[f])
			cc_preamble_address_code (cd, ch, c1, c2);
		return;
	}

	switch (c1 & 7) {
	case 0:
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
		    || CC_MODE_UNKNOWN == ch->mode)
			break;

		if (c2 < 0x30) {
			/* Backgr. Attr. Codes -- 001 c000  010 xxxt */
			/* EIA 608-B Section 6.2. */
			cc_put_char (cd, ch, 0x1000 | c2,
				     /* displayable */ FALSE,
				     /* backspace */ TRUE);
		} else {
			/* Undefined. */
		}

		break;

	case 1:
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
		    || CC_MODE_UNKNOWN == ch->mode)
			break;

		if (c2 < 0x30) {
			/* Mid-Row Codes -- 001 c001  010 xxxu */
			/* 47 CFR 15.119 (h)(1)(i): Spacing attribute. */
			cc_put_char (cd, ch, 0x1100 | c2,
				     /* displayable */ FALSE,
				     /* backspace */ FALSE);
		} else {
			/* Special Characters -- 001 c001  011 xxxx */
			if (0x39 == c2) {
				/* Transparent space. */
				cc_put_char (cd, ch, 0,
					     /* displayable */ FALSE,
					     /* backspace */ FALSE);
			} else {
				cc_put_char (cd, ch, 0x1100 | c2,
					     /* displayable */ TRUE,
					     /* backspace */ FALSE);
			}
		}

		break;

	case 2:
	case 3: /* Extended Character Set -- 001 c01x  01x xxxx */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
		    || CC_MODE_UNKNOWN == ch->mode)
			break;

		/* EIA 608-B Section 6.4.2. */
		cc_put_char (cd, ch, (c1 * 256 + c2) & 0x777F,
			     /* displayable */ TRUE,
			     /* backspace */ TRUE);
		break;

	case 4:
	case 5:
		if (c2 < 0x30) {
			/* Misc. Control Codes -- 001 c10f  010 xxxx */
			cc_misc_control_code (cd, ch, c2, ch_num0, f);
		} else {
			/* Undefined. */
		}

		break;

	case 6: /* reserved */
		break;

	case 7:	/* Extended control codes -- 001 c111  01x xxxx */
		if (UNKNOWN_CC_CHANNEL == cd->curr_ch_num[f]
		    || CC_MODE_UNKNOWN == ch->mode)
			break;

		cc_ext_control_code (cd, ch, c2);

		break;
	}
}

static vbi_bool
cc_characters			(struct cc_decoder *	cd,
				 struct cc_channel *	ch,
				 int			c)
{
	if (0 == c) {
		if (CC_MODE_UNKNOWN == ch->mode)
			return TRUE;

		/* XXX After x NUL characters (presumably a caption
		   pause), force a display update if we do not send
		   events on every display change. */

		return TRUE;
	}

	if (c < 0x20) {
		/* Parity error or invalid data. */

		if (c < 0 && CC_MODE_UNKNOWN != ch->mode) {
			/* 47 CFR Section 15.119 (j)(1). */
			cc_put_char (cd, ch, 0x7F,
				     /* displayable */ TRUE,
				     /* backspace */ FALSE);
		}

		return FALSE;
	}

	if (CC_MODE_UNKNOWN != ch->mode) {
		cc_put_char (cd, ch, c,
			     /* displayable */ TRUE,
			     /* backspace */ FALSE);
	}

	return TRUE;
}

vbi_bool
cc_feed				(struct cc_decoder *	cd,
				 const uint8_t		buffer[2],
				 unsigned int		line,
				 const struct timeval *	tv,
				 int64_t		pts)
{
	int c1, c2;
	enum field_num f;
	vbi_bool all_successful;

	assert (NULL != cd);

	f = FIELD_1;

	switch (line) {
	case 21: /* NTSC */
	case 22: /* PAL/SECAM */
		break;

	case 284: /* NTSC */
		f = FIELD_2;
		break;

	default:
		return FALSE;
	}

	cd->timestamp.sys = *tv;
	cd->timestamp.pts = pts;

	/* FIXME deferred reset here */

	c1 = vbi_unpar8 (buffer[0]);
	c2 = vbi_unpar8 (buffer[1]);

	all_successful = TRUE;

	/* 47 CFR 15.119 (2)(i)(4): "If the first transmission of a
	   control code pair passes parity, it is acted upon within
	   one video frame. If the next frame contains a perfect
	   repeat of the same pair, the redundant code is ignored. If,
	   however, the next frame contains a different but also valid
	   control code pair, this pair, too, will be acted upon (and
	   the receiver will expect a repeat of this second pair in
	   the next frame).  If the first byte of the expected
	   redundant control code pair fails the parity check and the
	   second byte is identical to the second byte in the
	   immediately preceding pair, then the expected redundant
	   code is ignored. If there are printing characters in place
	   of the redundant code, they will be processed normally."

	   EIA 608-B Section 8.3: Caption control codes on field 2 may
	   repeat as on field 1. Section 8.6.2: XDS control codes
	   shall not repeat. */

	if (unlikely (c1 < 0)) {
		goto parity_error;
	} else if (c1 == cd->expect_ctrl[f][0]
		   && c2 == cd->expect_ctrl[f][1]) {
		/* Already acted upon. */
		cd->expect_ctrl[f][0] = -1;
		goto finish;
	}

	if (c1 >= 0x10 && c1 < 0x20) {
		/* Caption control code. */

		/* There's no XDS on field 1, we just
		   use an array to save a branch. */
		cd->in_xds[f] = FALSE;

		/* 47 CFR Section 15.119 (i)(1), (i)(2). */
		if (c2 < 0x20) {
			/* Parity error or invalid control code.
			   Let's hope it repeats. */
			goto parity_error;
		}

		cc_control_code (cd, c1, c2, f);

		if (cd->event_pending) {
			cc_display_event (cd, cd->event_pending, 0);
			cd->event_pending = NULL;
		}

		cd->expect_ctrl[f][0] = c1;
		cd->expect_ctrl[f][1] = c2;
	} else {
		cd->expect_ctrl[f][0] = -1;

		if (c1 < 0x10) {
			if (FIELD_1 == f) {
				/* 47 CFR Section 15.119 (i)(1): "If the
				   non-printing character in the pair is
				   in the range 00h to 0Fh, that character
				   alone will be ignored and the second
				   character will be treated normally." */
				c1 = 0;
			} else if (0x0F == c1) {
				/* XDS packet terminator. */
				cd->in_xds[FIELD_2] = FALSE;
				goto finish;
			} else if (c1 >= 0x01) {
				/* XDS packet start or continuation.
				   EIA 608-B Section 7.7, 8.5: Also
				   interrupts a Text mode
				   transmission. */
				cd->in_xds[FIELD_2] = TRUE;
				goto finish;
			}
		}

		{
			struct cc_channel *ch;
			vbi_pgno ch_num;

			ch_num = cd->curr_ch_num[f];
			if (UNKNOWN_CC_CHANNEL == ch_num)
				goto finish;

			ch_num = ((ch_num - VBI_CAPTION_CC1) & 5) + f * 2;
			ch = &cd->channel[ch_num];

			all_successful &= cc_characters (cd, ch, c1);
			all_successful &= cc_characters (cd, ch, c2);

			if (cd->event_pending) {
				cc_display_event (cd, cd->event_pending, 0);
				cd->event_pending = NULL;
			}
		}
	}

 finish:
	cd->error_history = cd->error_history * 2 + all_successful;

	return all_successful;

 parity_error:
	cd->expect_ctrl[f][0] = -1;

	/* XXX Some networks stupidly transmit 0x0000 instead of
	   0x8080 as filler. Perhaps we shouldn't take that as a
	   serious parity error. */
	cd->error_history *= 2;

	return FALSE;
}

void
cc_reset			(struct cc_decoder *	cd)
{
	unsigned int ch_num;

	assert (NULL != cd);

	for (ch_num = 0; ch_num < MAX_CC_CHANNELS; ++ch_num) {
		struct cc_channel *ch;

		ch = &cd->channel[ch_num];

		if (ch_num <= 3) {
			ch->mode = CC_MODE_UNKNOWN;

			/* Something suitable for roll-up mode. */
			ch->curr_row = CC_LAST_ROW;
			ch->curr_column = CC_FIRST_COLUMN;
			ch->window_rows = 4;
		} else {
			ch->mode = CC_MODE_TEXT; /* invariable */

			/* EIA 608-B Section 7.4: "When Text Mode has
			   initially been selected and the specified
			   Text memory is empty, the cursor starts at
			   the topmost row, Column 1." */
			ch->curr_row = CC_FIRST_ROW;
			ch->curr_column = CC_FIRST_COLUMN;
			ch->window_rows = 0; /* n/a */
		}

		ch->displayed_buffer = 0;

		ch->last_pac = 0;

		CLEAR (ch->buffer);
		CLEAR (ch->dirty);

		cc_timestamp_reset (&ch->timestamp);
		cc_timestamp_reset (&ch->timestamp_c0);
	}

	cd->curr_ch_num[0] = UNKNOWN_CC_CHANNEL;
	cd->curr_ch_num[1] = UNKNOWN_CC_CHANNEL;

	memset (cd->expect_ctrl, -1, sizeof (cd->expect_ctrl));

	CLEAR (cd->in_xds);

	cd->event_pending = NULL;
}

void
cc_init			(struct cc_decoder *	cd)
{
	cc_reset (cd);

	cd->error_history = 0;

	cc_timestamp_reset (&cd->timestamp);
}

#if 0 /* to be replaced */

static int webtv_check(struct caption_recorder *cr, char * buf,int len)
{
	unsigned long   sum;
	unsigned long   nwords;
	unsigned short  csum=0;
	char temp[9];
	int nbytes=0;

	while (buf[0]!='<' && len > 6)  //search for the start
	{
		buf++; len--;
	}

	if (len == 6) //failure to find start
		return 0;


	while (nbytes+6 <= len)
	{
		//look for end of object checksum, it's enclosed in []'s and there shouldn't be any [' after
		if (buf[nbytes] == '[' && buf[nbytes+5] == ']' && buf[nbytes+6] != '[')
			break;
		else
			nbytes++;
	}
	if (nbytes+6>len) //failure to find end
		return 0;

	nwords = nbytes >> 1; sum = 0;

	//add up all two byte words
	while (nwords-- > 0) {
		sum += *buf++ << 8;
		sum += *buf++;
	}
	if (nbytes & 1) {
		sum += *buf << 8;
	}
	csum = (unsigned short)(sum >> 16);
	while(csum !=0) {
		sum = csum + (sum & 0xffff);
		csum = (unsigned short)(sum >> 16);
	}
	sprintf(temp,"%04X\n",(int)~sum&0xffff);
	buf++;
	if (!strncmp(buf,temp,4))
	{
		buf[5]=0;
		if (cr->cur_ch[cr->field] >= 0 && cr->cc_fp[cr->cur_ch[cr->field]]) {
		if (!cr->plain)
			fprintf(cr->cc_fp[cr->cur_ch[cr->field]], "\33[35mWEBTV: %s\33[0m\n",buf-nbytes-1);
		else
			fprintf(cr->cc_fp[cr->cur_ch[cr->field]], "WEBTV: %s\n",buf-nbytes-1);
		fflush (cr->cc_fp[cr->cur_ch[cr->field]]);
		}
	}
	return 0;
}

#endif /* 0 */

/* CEA 708-C Digital TV Closed Caption decoder. */

static const uint8_t
dtvcc_c0_length [4] = {
	1, 1, 2, 3
};

static const uint8_t
dtvcc_c1_length [32] = {
	/* 0x80 CW0 ... CW7 */ 1, 1, 1, 1,  1, 1, 1, 1,
	/* 0x88 CLW */ 2,
	/* 0x89 DSW */ 2,
	/* 0x8A HDW */ 2,
	/* 0x8B TGW */ 2,

	/* 0x8C DLW */ 2,
	/* 0x8D DLY */ 2,
	/* 0x8E DLC */ 1,
	/* 0x8F RST */ 1,

	/* 0x90 SPA */ 3,
	/* 0x91 SPC */ 4,
	/* 0x92 SPL */ 3,
	/* CEA 708-C Section 7.1.5.1: 0x93 ... 0x96 are
	   reserved one byte codes. */ 1, 1, 1, 1,
	/* 0x97 SWA */ 5,
	/* 0x98 DF0 ... DF7 */ 7, 7, 7, 7,  7, 7, 7, 7
};

static const uint16_t
dtvcc_g2 [96] = {
	/* Note Unicode defines no transparent spaces. */
	0x0020, /* 0x1020 Transparent space */
	0x00A0, /* 0x1021 Non-breaking transparent space */

	0,      /* 0x1022 reserved */
	0,
	0,
	0x2026, /* 0x1025 Horizontal ellipsis */
	0,
	0,
	0,
	0,
	0x0160, /* 0x102A S with caron */
	0,
	0x0152, /* 0x102C Ligature OE */
	0,
	0,
	0,

	/* CEA 708-C Section 7.1.8: "The character (0x30) is a solid
	   block which fills the entire character position with the
	   text foreground color." */
	0x2588, /* 0x1030 Full block */

	0x2018, /* 0x1031 Left single quotation mark */
	0x2019, /* 0x1032 Right single quotation mark */
	0x201C, /* 0x1033 Left double quotation mark */
	0x201D, /* 0x1034 Right double quotation mark */
	0x2022,
	0,
	0,
	0,
	0x2122, /* 0x1039 Trademark sign */
	0x0161, /* 0x103A s with caron */
	0,
	0x0153, /* 0x103C Ligature oe */
	0x2120, /* 0x103D Service mark */
	0,
	0x0178, /* 0x103F Y with diaeresis */

	/* Code points 0x1040 ... 0x106F reserved. */
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,

	0,      /* 0x1070 reserved */
	0,
	0,
	0,
	0,
	0,
	0x215B, /* 0x1076 1/8 */
	0x215C, /* 0x1077 3/8 */
	0x215D, /* 0x1078 5/8 */
	0x215E, /* 0x1079 7/8 */
	0x2502, /* 0x107A Box drawings vertical */
	0x2510, /* 0x107B Box drawings down and left */
	0x2514, /* 0x107C Box drawings up and right */
	0x2500, /* 0x107D Box drawings horizontal */
	0x2518, /* 0x107E Box drawings up and left */
	0x250C  /* 0x107F Box drawings down and right */
};

static vbi_rgba
dtvcc_color_map[8] = {
	VBI_RGBA(0x00, 0x00, 0x00), VBI_RGBA(0xFF, 0x00, 0x00),
	VBI_RGBA(0x00, 0xFF, 0x00), VBI_RGBA(0xFF, 0xFF, 0x00),
	VBI_RGBA(0x00, 0x00, 0xFF), VBI_RGBA(0xFF, 0x00, 0xFF),
	VBI_RGBA(0x00, 0xFF, 0xFF), VBI_RGBA(0xFF, 0xFF, 0xFF)
};


static void
dtvcc_set_page_color_map(vbi_decoder *vbi, vbi_page *pg)
{
	vbi_transp_colormap(vbi, pg->color_map,	dtvcc_color_map, 8);
}

static vbi_color
dtvcc_map_color(dtvcc_color c)
{
	vbi_color ret = VBI_BLACK;

	c &= 0x2A;
	switch(c){
		case 0:
			ret = VBI_BLACK;
			break;
		case 0x20:
			ret = VBI_RED;
			break;
		case 0x08:
			ret = VBI_GREEN;
			break;
		case 0x28:
			ret = VBI_YELLOW;
			break;
		case 0x02:
			ret = VBI_BLUE;
			break;
		case 0x22:
			ret = VBI_MAGENTA;
			break;
		case 0x0A:
			ret = VBI_CYAN;
			break;
		case 0x2A:
			ret = VBI_WHITE;
			break;
	}

	return ret;
}

unsigned int
dtvcc_unicode_real			(unsigned int		c)
{
	if (c <= 0x1200)
	{
		if (unlikely (0 == (c & 0x60))) {
			/* C0, C1, C2, C3 */
			return 0;
		} else if (likely (c < 0x100)) {
			/* G0, G1 */
			if (unlikely (0xAD == c))
				return 0x2D;
			else if (unlikely (0x7F == c))
				return 0x266A; /* music note */
			else
				return c;
		} else if (c < 0x1080) {
			if (unlikely (c < 0x1020))
				return 0;
			else
				return dtvcc_g2[c - 0x1020];
		} else if (0x10A0 == c) {
			/* We map all G2/G3 characters which are not
			   representable in Unicode to private code U+E900
			   ... U+E9FF. */
			return 0xf101; /* caption icon */
		}

		return 0;
	}
	return c;
}

unsigned int
dtvcc_unicode			(unsigned int		c)
{
	unsigned int uc = dtvcc_unicode_real(c);
	if (uc == 0)
		uc = '_';

	return uc;
}

static void
dtvcc_render(struct dtvcc_decoder *	dc, struct dtvcc_service *	ds)
{
#if 0
	vbi_event event;
	struct tvcc_decoder *td = PARENT(dc, struct tvcc_decoder, dtvcc);
	struct dtvcc_window *win[8];
	int i, cnt;

	//printf("render check\n");

	cnt = 8;
	dtvcc_get_visible_windows(ds, &cnt, win);
	//if (!cnt)
	//	return;

	if (cnt != ds->old_win_cnt) {
		//printf("cnt changed\n");
		goto changed;
	}

	for (i = 0; i < cnt; i ++) {
		struct dtvcc_window *w1 = win[i];
		struct dtvcc_window *w2 = &ds->old_window[i];

		if (memcmp(w1->buffer, w2->buffer, sizeof(w1->buffer))) {
			//printf("text changed\n");
			goto changed;
		}

		if (memcmp(&w1->style, &w2->style, sizeof(w1->style))) {
			//printf("style changed\n");
			goto changed;
		}

		if (memcmp(&w1->curr_pen, &w2->curr_pen, sizeof(w1->curr_pen))) {
			//printf("pen changed\n");
			goto changed;
		}

		if (w1->row_count != w2->row_count) {
			//printf("row changed\n");
			goto changed;
		}

		if (w1->column_count != w2->column_count) {
			//printf("col changed\n");
			goto changed;
		}

		if (w1->visible != w2->visible) {
			//printf("vis changed\n");
			goto changed;
		}
	}

	return;
changed:
	for (i = 0; i < cnt; i ++) {
		ds->old_window[i] = *win[i];
	}
	ds->old_win_cnt = cnt;
#endif
	ds->update = 1;
#if 0

	event.type = VBI_EVENT_CAPTION;
	event.ev.caption.pgno = ds - dc->service + 1 + 8/*after 8 cc channels*/;

	/* Permits calling tvcc_fetch_page from handler */
	pthread_mutex_unlock(&td->mutex);

	vbi_send_event(td->vbi, &event);

	pthread_mutex_lock(&td->mutex);
#endif
}

static void
dtvcc_reset_service		(struct dtvcc_service *	ds);

static unsigned int
dtvcc_window_id			(struct dtvcc_service *	ds,
				 struct dtvcc_window *	dw)
{
	return dw - ds->window;
}

static unsigned int
dtvcc_service_num		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds)
{
	return ds - dc->service + 1;
}

/* Up to eight windows can be visible at once, so which one displays
   the caption? Let's take a guess. */
static struct dtvcc_window *
dtvcc_caption_window		(struct dtvcc_service *	ds)
{
	struct dtvcc_window *dw;
	unsigned int max_priority;
	unsigned int window_id;

	dw = NULL;
	max_priority = 8;

	for (window_id = 0; window_id < 8; ++window_id) {
		if (0 == (ds->created & (1 << window_id)))
			continue;
		if (!ds->window[window_id].visible)
			continue;
		/*if (DIR_BOTTOM_TOP
		    != ds->window[window_id].style.scroll_direction)
			continue; */
		if (ds->window[window_id].priority < max_priority) {
			dw = &ds->window[window_id];
			max_priority = ds->window[window_id].priority;
		}
	}

	return dw;
}

static void
dtvcc_stream_event		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 struct dtvcc_window *	dw,
				 unsigned int		row)
{
	vbi_char text[48];
	vbi_char ac;
	unsigned int column;

	if (NULL == dw || dw != dtvcc_caption_window (ds))
		return;

	/* Note we only stream windows with scroll direction
	   upwards. */
	if (0 != (dw->streamed & (1 << row))
	   /* || !cc_timestamp_isset (&dw->timestamp_c0)*/)
		return;

	dw->streamed |= 1 << row;

	for (column = 0; column < dw->column_count; ++column) {
		if (0 != dw->buffer[row][column])
			break;
	}

	/* Row contains only transparent spaces. */
	if (column >= dw->column_count)
		return;


	dtvcc_render(dc, ds);

	/* TO DO. */
	CLEAR (ac);
	ac.foreground = VBI_WHITE;
	ac.background = VBI_BLACK;
	ac.opacity = VBI_OPAQUE;

	for (column = 0; column < dw->column_count; ++column) {
		unsigned int c;

		c = dw->buffer[row][column];
		if (0 == c) {
			ac.unicode = 0x20;
		} else {
			ac.unicode = dtvcc_unicode (c);
			if (0 == ac.unicode) {
				ac.unicode = 0x20;
			}
		}
		text[column] = ac;
	}

	cc_timestamp_reset (&dw->timestamp_c0);
}

static vbi_bool
dtvcc_put_char			(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 unsigned int		c)
{
	struct dtvcc_window *dw;
	unsigned int row;
	unsigned int column,i;
	dc = dc; /* unused */

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		//AM_DEBUG(1, "================ window null !!!!!");
		return FALSE;
	}
	dw->latest_cmd_cr = 0;

	column = dw->curr_column;
	row = dw->curr_row;

	/* FIXME how should we handle TEXT_TAG_NOT_DISPLAYABLE? */
	/* Add row column lock support */
	switch (dw->style.print_direction) {
		case DIR_LEFT_RIGHT:
		if (column >= dw->column_count)
		{
			if (dw->column_lock == 1)
			{
				if (dw->row_lock == 0) //rl 0 cl 1
				{
					column = 0;
					row++;
					if (row >= dw->row_count)
					{
						// row moves up
						memmove(dw->buffer, &dw->buffer[1][0], sizeof(dw->buffer[0])*(dw->row_count));
						memmove(dw->pen, &dw->pen[1][0], sizeof(dw->pen[0])*(dw->row_count));
						row = dw->row_count - 1;
					}
				}
				else //rl 1 cl 1
				{
					return TRUE;
				}
			}
			else //cl 0
			{
				if (column < 42)
				{
					if (dw->column_no_lock_length < column + 1)
						dw->column_no_lock_length = column + 1;
				}
				else
				{
					if (dw->row_lock == 1)
						return TRUE;
					else //cl 0 rl 0
					{
						column = 0;
						row++;
						if (row >= dw->row_count)
						{
							// row moves up
							memmove(dw->buffer, &dw->buffer[1][0], sizeof(dw->buffer[0])*(dw->row_count));
							memmove(dw->pen, &dw->pen[1][0], sizeof(dw->pen[0])*(dw->row_count));
							row = dw->row_count - 1;
							dw->row_no_lock_length = row;
						}
					}
				}
			}
		}
		break;
		case DIR_RIGHT_LEFT:
		case DIR_TOP_BOTTOM:
		case DIR_BOTTOM_TOP:
		break;
	}
	dw->buffer[row][column] = c;
	dw->pen[row][column]    = dw->curr_pen.style;
	if (c == 0x1020 || c == 0x1021)
	{
		if (c == 0x1020)
			dw->buffer[row][column] = 0x20;
		else
			dw->buffer[row][column] = 0xA0;
		dw->pen[row][column].bg_opacity = OPACITY_TRANSPARENT;
		dw->pen[row][column].fg_opacity = OPACITY_TRANSPARENT;
	}
	if (dw->visible)
		dtvcc_render(dc, ds);

	switch (dw->style.print_direction) {
	case DIR_LEFT_RIGHT:
		dw->streamed &= ~(1 << row);
		if (!cc_timestamp_isset (&dw->timestamp_c0))
			dw->timestamp_c0 = ds->timestamp;
		++column;
		break;

	case DIR_RIGHT_LEFT:
		dw->streamed &= ~(1 << row);
		if (!cc_timestamp_isset (&dw->timestamp_c0))
			dw->timestamp_c0 = ds->timestamp;
		column--;
		break;

	case DIR_TOP_BOTTOM:
		dw->streamed &= ~(1 << column);
		if (!cc_timestamp_isset (&dw->timestamp_c0))
			dw->timestamp_c0 = ds->timestamp;
		++row;
		break;

	case DIR_BOTTOM_TOP:
		dw->streamed &= ~(1 << column);
		if (!cc_timestamp_isset (&dw->timestamp_c0))
			dw->timestamp_c0 = ds->timestamp;
		row--;
		break;
	}

	dw->curr_row = row;
	dw->curr_column = column;
	return TRUE;
}

static vbi_bool
dtvcc_set_pen_location		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 const uint8_t *	buf)
{
	struct dtvcc_window *dw;
	int row;
	int column;
	int off;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	row = buf[1];
	/* We check the top four zero bits. */
	if (row >= 16) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	column = buf[2];
	/* We also check the top two zero bits. */
	if (column >= 42 || row >= 16) {
		ds->error_line = __LINE__;
		return FALSE;
	}

//	if ((row == dw->curr_row) && (column == dw->curr_column))
//		return TRUE;

	dtvcc_stream_event (dc, ds, dw, dw->curr_row);

	/* FIXME there's more. */
	dw->curr_row = row;
	dw->curr_column = column;

	//if (dw->curr_column == 0 && dw->latest_cmd_cr == 1) If latest cmd is cr, no need to judge column
	if (dw->latest_cmd_cr == 1)
	{
		switch (dw->style.scroll_direction) {
			case DIR_LEFT_RIGHT:
				for (column = 0; column <= dw->curr_column; column ++) {
					int empty = 1;
					for ( row = 0; row < dw->row_count; row ++) {
						if (dw->buffer[row][column]) {
							empty = 0;
							break;
						}
					}
					if (!empty)
						break;
				}

				off = dw->curr_column - column + 1;

				if (off) {
					for (column = dw->column_count - 1; column > dw->curr_column; column --) {
						for (row = 0; row < dw->row_count; row ++) {
							dw->buffer[row][column] =
								dw->buffer[row][column - off];
							dw->pen[row][column] =
								dw->pen[row][column - off];

						}
					}
					for (column = 0; column <= dw->curr_column; column ++) {
						for (row = 0; row < dw->row_count; row ++) {
							dw->buffer[row][column] = 0;
							memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
						}
					}
				}
				break;

			case DIR_RIGHT_LEFT:
				for (column = dw->column_count - 1; column >= dw->curr_column; column --) {
					int empty = 1;
					for ( row = 0; row < dw->row_count; row ++) {
						if (dw->buffer[row][column]) {
							empty = 0;
							break;
						}
					}
					if (!empty)
						break;
				}

				off = column - dw->curr_column + 1;

				if (off) {
					for (column = 0; column < dw->curr_column; column ++) {
						for (row = 0; row < dw->row_count; row ++) {
							dw->buffer[row][column] =
								dw->buffer[row][column + off];
							dw->pen[row][column] =
								dw->pen[row][column + off];

						}
					}
					for (column = dw->curr_column; column < dw->column_count; column ++) {
						for (row = 0; row < dw->row_count; row ++) {
							dw->buffer[row][column] = 0;
							memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
						}
					}
				}
				break;

			case DIR_TOP_BOTTOM:
				for (row = 0; row <= dw->curr_row; row ++) {
					int empty = 1;
					for ( column = 0; column < dw->column_count; column ++) {
						if (dw->buffer[row][column]) {
							empty = 0;
							break;
						}
					}
					if (!empty)
						break;
				}

				off = dw->curr_row - row + 1;

				if (off) {
					for (row = dw->row_count - 1; row > dw->curr_row; row --) {
						for (column = 0; column < dw->column_count; column ++) {
							dw->buffer[row][column] =
								dw->buffer[row - off][column];
							dw->pen[row][column] =
								dw->pen[row - off][column];

						}
					}
					for (row = 0; row <= dw->curr_row; row ++) {
						for (column = 0; column < dw->column_count; column ++) {
							dw->buffer[row][column] = 0;
							memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
						}
					}
				}
				break;

			case DIR_BOTTOM_TOP:
				for (row = dw->row_count - 1; row >= dw->curr_row; row --) {
					int empty = 1;
					for ( column = 0; column < dw->column_count; column ++) {
						if (dw->buffer[row][column]) {
							empty = 0;
							break;
						}
					}
					if (!empty)
						break;
				}

				off = row - dw->curr_row + 1;

				if (off) {
					for (row = 0; row < dw->curr_row; row ++) {
						for (column = 0; column < dw->column_count; column ++) {
							dw->buffer[row][column] =
								dw->buffer[row + off][column];
							dw->pen[row][column] =
								dw->pen[row + off][column];

						}
					}
					for (row = dw->curr_row; row < dw->row_count; row ++) {
						for (column = 0; column < dw->column_count; column ++) {
							dw->buffer[row][column] = 0;
							memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
						}
					}
				}
				break;
		}
		dw->latest_cmd_cr = 0;
	}

	return TRUE;
}

static vbi_bool
dtvcc_set_pen_color		(struct dtvcc_service *	ds,
				 const uint8_t *	buf)
{
	struct dtvcc_window *dw;
	unsigned int c;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[3];
	if (0 != (c & 0xC0)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	dw->curr_pen.style.edge_color = c;
	c = buf[1];
	dw->curr_pen.style.fg_opacity = c >> 6;
	dw->curr_pen.style.fg_color = c & 0x3F;
	dw->curr_pen.style.fg_flash = ((c>>6)==1)?1:0;
	c = buf[2];
	dw->curr_pen.style.bg_opacity = c >> 6;
	dw->curr_pen.style.bg_color = c & 0x3F;
	dw->curr_pen.style.bg_flash = ((c>>6)==1)?1:0;

	return TRUE;
}

static vbi_bool
dtvcc_set_pen_attributes	(struct dtvcc_service *	ds,
				 const uint8_t *	buf)
{
	struct dtvcc_window *dw;
	unsigned int c;
	enum pen_size pen_size;
	enum offset offset;
	enum edge edge_type;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[1];
	offset = (c >> 2) & 3;
	pen_size = c & 3;
	//TODO: why not larger than 3
	/*
	if ((offset | pen_size) >= 3) {
		ds->error_line = __LINE__;
		return FALSE;
	} */

	c = buf[2];
	edge_type = (c >> 3) & 7;
	if (edge_type >= 6) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[1];
	dw->curr_pen.text_tag = c >> 4;
	dw->curr_pen.style.offset = offset;
	dw->curr_pen.style.pen_size = pen_size;
	c = buf[2];
	dw->curr_pen.style.italics = c >> 7;
	dw->curr_pen.style.underline = (c >> 6) & 1;
	dw->curr_pen.style.edge_type = edge_type;
	dw->curr_pen.style.font_style = c & 7;

	return TRUE;
}

static vbi_bool
dtvcc_set_window_attributes	(struct dtvcc_service *	ds,
				 const uint8_t *	buf)
{
	struct dtvcc_window *dw;
	unsigned int c;
	enum edge border_type;
	enum display_effect display_effect;

	dw = ds->curr_window;
	if (NULL == dw)
		return FALSE;

	c = buf[2];
	border_type = ((buf[3] >> 5) & 0x04) | (c >> 6);
	if (border_type >= 6)
		return FALSE;

	c = buf[4];
	display_effect = c & 3;
	if (display_effect >= 3)
		return FALSE;

	c = buf[1];
	dw->style.fill_opacity = c >> 6;
	dw->style.fill_color = c & 0x3F;
	dw->style.window_flash = ((c>>6)==1)?1:0;
	c = buf[2];
	dw->style.border_type = border_type;
	dw->style.border_color = c & 0x3F;
	c = buf[3];
	dw->style.wordwrap = (c >> 6) & 1;
	dw->style.print_direction = (c >> 4) & 3;
	dw->style.scroll_direction = (c >> 2) & 3;
	dw->style.justify = c & 3;
	c = buf[4];
	dw->style.effect_speed = c >> 4;
	dw->style.effect_direction = (c >> 2) & 3;
	dw->style.display_effect = display_effect;

	return TRUE;
}

static vbi_bool
dtvcc_clear_windows		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 dtvcc_window_map	window_map)
{
	unsigned int i;

	window_map &= ds->created;

	for (i = 0; i < 8; ++i) {
		struct dtvcc_window *dw;

		if (0 == (window_map & (1 << i)))
			continue;

		dw = &ds->window[i];

		dtvcc_stream_event (dc, ds, dw, dw->curr_row);

		memset (dw->buffer, 0, sizeof (dw->buffer));
		memset (dw->pen, 0, sizeof(dw->pen));

		dw->curr_column = 0;
		dw->curr_row = 0;

		dw->streamed = 0;
		dw->style.display_effect = 0;
		dw->effect_status = 0;
		if (dw->visible)
			dtvcc_render(dc, ds);

		/* FIXME CEA 708-C Section 7.1.4 (Form Feed)
		   and 8.10.5.3 confuse me. */
		if (0) {
			dw->curr_column = 0;
			dw->curr_row = 0;
		}
	}

	return TRUE;
}

static vbi_bool
dtvcc_define_window		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 uint8_t *		buf)
{
	static const struct dtvcc_window_style window_styles [7] = {
		{
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_TRANSPARENT, EDGE_NONE, 0
		}, {
			JUSTIFY_CENTER, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			TRUE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			TRUE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_TRANSPARENT, EDGE_NONE, 0
		}, {
			JUSTIFY_CENTER, DIR_LEFT_RIGHT, DIR_BOTTOM_TOP,
			TRUE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}, {
			JUSTIFY_LEFT, DIR_TOP_BOTTOM, DIR_RIGHT_LEFT,
			FALSE, DISPLAY_EFFECT_SNAP, 0, 0, 0,
			OPACITY_SOLID, EDGE_NONE, 0
		}
	};
	static const struct dtvcc_pen_style pen_styles [7] = {
		{
			PEN_SIZE_STANDARD, 0, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 1, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 2, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 3, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 4, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_NONE, 0x3F, OPACITY_SOLID,
			0x00, OPACITY_SOLID, 0
		}, {
			PEN_SIZE_STANDARD, 3, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_UNIFORM, 0x3F, OPACITY_SOLID,
			0, OPACITY_TRANSPARENT, 0x00
		}, {
			PEN_SIZE_STANDARD, 4, OFFSET_NORMAL, FALSE,
			FALSE, EDGE_UNIFORM, 0x3F, OPACITY_SOLID,
			0, OPACITY_TRANSPARENT, 0x00
		}
	};
	struct dtvcc_window *dw;
	dtvcc_window_map window_map;
	vbi_bool anchor_relative;
	unsigned int anchor_vertical;
	unsigned int anchor_horizontal;
	unsigned int anchor_point;
	unsigned int column_count_m1;
	unsigned int window_id;
	unsigned int window_style_id;
	unsigned int pen_style_id;
	unsigned int c;

	//printf("define window\n");

	if (0 != ((buf[1] | buf[6]) & 0xC0)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[2];
	anchor_relative = (c >> 7) & 1;
	anchor_vertical = c & 0x7F;
	anchor_horizontal = buf[3];
	if (0 == anchor_relative) {
		if (unlikely (anchor_vertical >= 75
			      || anchor_horizontal >= 210)) {
			ds->error_line = __LINE__;
			return FALSE;
		}
	} else {
		if (unlikely (anchor_vertical >= 100
			      || anchor_horizontal >= 100)) {
			ds->error_line = __LINE__;
			return FALSE;
		}
	}

	c = buf[4];
	anchor_point = c >> 4;
	if (unlikely (anchor_point >= 9)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	//printf("define window %d\n", column_count_m1);

	column_count_m1 = buf[5];
	/* We also check the top two zero bits. */
	/* For atsc eng, column maximum is 42, for kor, is 84 */
	char* lang_korea;
	int column_count;
	lang_korea = strstr(dc->lang, "kor");
	AM_DEBUG(0, "kor lang, width 84");

	/* Korea has full word and half word */
	if (lang_korea)
		column_count = 84;
	else
		column_count = 42;
	if (unlikely (column_count_m1 >= column_count)) {
		ds->error_line = __LINE__;
		return FALSE;
	}
	//printf("define windowa\n");

	window_id = buf[0] & 7;
	dw = &ds->window[window_id];
	window_map = 1 << window_id;

	ds->curr_window = dw;

	c = buf[1];
	dw->visible = (c >> 5) & 1;
	dw->row_lock = (c >> 4) & 1;
	dw->column_lock = (c >> 3) & 1;
	dw->priority = c & 7;

	dw->anchor_relative = anchor_relative;
	dw->anchor_vertical = anchor_vertical;
	dw->anchor_horizontal = anchor_horizontal;
	dw->anchor_point = anchor_point;

	c = buf[4];
	dw->row_count = (c & 15) + 1;
	dw->column_count = column_count_m1 + 1;
	dw->column_no_lock_length = 0;

	c = buf[6];
	window_style_id = (c >> 3) & 7;
	pen_style_id = c & 7;

	if (window_style_id > 0) {
		dw->style = window_styles[window_style_id-1];
	} else if (0 == (ds->created & window_map)) {
		dw->style = window_styles[1];
	}

	if (pen_style_id > 0) {
		dw->curr_pen.style = pen_styles[pen_style_id];
	} else if (0 == (ds->created & window_map)) {
		dw->curr_pen.style = pen_styles[1];
	}

	if (0 != (ds->created & window_map))
		return TRUE;

	/* Has to be something, no? */
	dw->curr_pen.text_tag = TEXT_TAG_NOT_DISPLAYABLE;

	dw->curr_column = 0;
	dw->curr_row = 0;

	dw->streamed = 0;

	cc_timestamp_reset (&dw->timestamp_c0);

	ds->created |= window_map;

	//printf("define %x %x\n", ds->curr_window, ds->created);

	return dtvcc_clear_windows (dc, ds, window_map);
}

static vbi_bool
dtvcc_display_windows		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 unsigned int		c,
				 dtvcc_window_map	window_map)
{
	unsigned int i;

	window_map &= ds->created;

	//printf("display %02x %p %02x\n", c, ds->curr_window, ds->created);
	if (ds->curr_window == NULL && ds->created == 0) {
		return FALSE;
		//return TRUE;
	}

	for (i = 0; i < 8; ++i) {
		struct dtvcc_window *dw;
		vbi_bool was_visible;

		if (0 == (window_map & (1 << i)))
			continue;

		dw = &ds->window[i];
		was_visible = dw->visible;

		switch (c) {
		case 0x89: /* DSW DisplayWindows */
			dw->visible = TRUE;
			dw->effect_status = CC_EFFECT_DISPLAY;
			break;

		case 0x8A: /* HDW HideWindows */
			dw->visible = FALSE;
			dw->effect_status = CC_EFFECT_HIDE;
			break;

		case 0x8B: /* TGW ToggleWindows */
			dw->visible = was_visible ^ TRUE;
			dw->effect_status =
				(dw->visible == TRUE)?CC_EFFECT_DISPLAY:CC_EFFECT_HIDE;
			break;
		}

		clock_gettime(CLOCK_REALTIME, &dw->effect_timer);

		if (!was_visible) {
			unsigned int row;

			dw->timestamp_c0 = ds->timestamp;
			for (row = 0; row < dw->row_count; ++row) {
				dtvcc_stream_event (dc, ds, dw, row);
			}
		}
	}

	dtvcc_render(dc, ds);

	return TRUE;
}

static vbi_bool
dtvcc_carriage_return		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds)
{
	struct dtvcc_window *dw;
	unsigned int row;
	unsigned int column;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	dtvcc_stream_event (dc, ds, dw, dw->curr_row);

	row = dw->curr_row;
	column = dw->curr_column;
	dw->latest_cmd_cr = 1;

	switch (dw->style.scroll_direction) {
	case DIR_LEFT_RIGHT:
		if (dw->style.print_direction == DIR_BOTTOM_TOP)
		{
			dw->curr_row = dw->row_count - 1;
			if (column > 0) {
				dw->curr_column = column - 1;
				break;
			}
		}
		else
		{
			dw->curr_row = 0;
			if (column > 0) {
				dw->curr_column = column - 1;
				break;
			}
		}
		dw->streamed = (dw->streamed << 1)
			& ~(1 << dw->column_count);
		for (row = 0; row < dw->row_count; ++row) {
			for (column = dw->column_count - 1;
			     column > 0; --column) {
				dw->buffer[row][column] =
					dw->buffer[row][column - 1];
				dw->pen[row][column] =
					dw->pen[row][column - 1];
			}
			dw->buffer[row][column] = 0;
			memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
		}
		break;

	case DIR_RIGHT_LEFT:
		if (dw->style.print_direction == DIR_BOTTOM_TOP)
		{
			dw->curr_row = dw->row_count - 1;
			if (column + 1 < dw->row_count) {
				dw->curr_column = column + 1;
				break;
			}
		}
		else
		{
			dw->curr_row = 0;
			if (column + 1 < dw->row_count) {
				dw->curr_column = column + 1;
				break;
			}
		}
		dw->streamed >>= 1;
		for (row = 0; row < dw->row_count; ++row) {
			for (column = 0;
			     column < dw->column_count - 1; ++column) {
				dw->buffer[row][column] =
					dw->buffer[row][column + 1];
				dw->pen[row][column] =
					dw->pen[row][column + 1];
			}
			dw->buffer[row][column] = 0;
			memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
		}
		break;

	case DIR_TOP_BOTTOM:
		if (dw->style.print_direction == DIR_RIGHT_LEFT)
		{
			AM_DEBUG(0, "CR: print_r_l cur_col %d row %d", dw->curr_column, dw->curr_row);
			dw->curr_column = dw->column_count - 1;
			if (row > 0) {
				dw->curr_row = row - 1;
				break;
			}
		}
		else
		{
			dw->curr_column = 0;
			if (row > 0) {
				dw->curr_row = row - 1;
				break;
			}
		}
		dw->streamed = (dw->streamed << 1)
			& ~(1 << dw->row_count);
		memmove (&dw->buffer[1], &dw->buffer[0],
			 sizeof (dw->buffer[0]) * (dw->row_count - 1));
		memmove (&dw->pen[1], &dw->pen[0],
			 sizeof (dw->pen[0]) * (dw->row_count - 1));
		memset (&dw->buffer[0], 0, sizeof (dw->buffer[0]));
		memset (&dw->pen[0], 0, sizeof(dw->pen[0]));
		break;

	case DIR_BOTTOM_TOP:
		if (dw->style.print_direction == DIR_RIGHT_LEFT)
		{
			dw->curr_column = dw->column_count - 1;;
			if (row + 1 < dw->row_count) {
				dw->curr_row = row + 1;
				break;
			}
		}
		else
		{
			dw->curr_column = 0;
			if (row + 1 < dw->row_count) {
				dw->curr_row = row + 1;
				break;
			}
		}
		dw->streamed >>= 1;
		memmove (&dw->buffer[0], &dw->buffer[1],
			 sizeof (dw->buffer[0]) * (dw->row_count - 1));
		memmove (&dw->pen[0], &dw->pen[1],
			 sizeof (dw->pen[0]) * (dw->row_count - 1));
		memset (&dw->buffer[row], 0, sizeof (dw->buffer[0]));
		memset (&dw->pen[row], 0, sizeof (dw->pen[0]));
		break;
	}

	return TRUE;
}

static vbi_bool
dtvcc_form_feed			(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds)
{
	struct dtvcc_window *dw;
	dtvcc_window_map window_map;

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	window_map = 1 << dtvcc_window_id (ds, dw);

	if (!dtvcc_clear_windows (dc, ds, window_map))
		return FALSE;

	dw->curr_row = 0;
	dw->curr_column = 0;

	return TRUE;
}

static vbi_bool
dtvcc_backspace			(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds)
{
	struct dtvcc_window *dw;
	unsigned int row;
	unsigned int column;
	unsigned int mask;

	dc = dc; /* unused */

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	row = dw->curr_row;
	column = dw->curr_column;

	switch (dw->style.print_direction) {
	case DIR_LEFT_RIGHT:
		mask = 1 << row;
		if (column-- <= 0)
			return TRUE;
		break;

	case DIR_RIGHT_LEFT:
		mask = 1 << row;
		if (++column >= dw->column_count)
			return TRUE;
		break;

	case DIR_TOP_BOTTOM:
		mask = 1 << column;
		if (row-- <= 0)
			return TRUE;
		break;

	case DIR_BOTTOM_TOP:
		mask = 1 << column;
		if (++row >= dw->row_count)
			return TRUE;
		break;
	}

	if (0 != dw->buffer[row][column]) {
		dw->streamed &= ~mask;
		dw->buffer[row][column] = 0;
		memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
	}

	dw->curr_row = row;
	dw->curr_column = column;

	return TRUE;
}

static vbi_bool
dtvcc_hor_carriage_return	(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds)
{
	struct dtvcc_window *dw;
	unsigned int row;
	unsigned int column;
	unsigned int mask;

	dc = dc; /* unused */

	dw = ds->curr_window;
	if (NULL == dw) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	row = dw->curr_row;
	column = dw->curr_column;

	switch (dw->style.print_direction) {
	case DIR_LEFT_RIGHT:
	case DIR_RIGHT_LEFT:
		mask = 1 << row;
		memset (&dw->buffer[row][0], 0,
			sizeof (dw->buffer[0]));
		memset (&dw->pen[row][0], 0,
			sizeof (dw->pen[0]));
		if (DIR_LEFT_RIGHT == dw->style.print_direction)
			dw->curr_column = 0;
		else
			dw->curr_column = dw->column_count - 1;
		break;

	case DIR_TOP_BOTTOM:
	case DIR_BOTTOM_TOP:
		mask = 1 << column;
		for (row = 0; row < dw->column_count; ++row) {
			dw->buffer[row][column] = 0;
			memset(&dw->pen[row][column], 0, sizeof(dw->pen[0][0]));
		}
		if (DIR_TOP_BOTTOM == dw->style.print_direction)
			dw->curr_row = 0;
		else
			dw->curr_row = dw->row_count - 1;
		break;
	}

	dw->streamed &= ~mask;

	return TRUE;
}

static vbi_bool
dtvcc_delete_windows		(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 dtvcc_window_map	window_map)
{
	struct dtvcc_window *dw;
	int i;
	int changed = 0;
	for (i = 0; i < N_ELEMENTS(ds->window); i ++) {
		dw = &ds->window[i];

		if (NULL != dw) {
			unsigned int window_id;
			window_id = dtvcc_window_id (ds, dw);
			if (ds->created & (1 << window_id)) {
				if (window_map & (1 << window_id)) {
					//printf("delete window %d\n", window_id);
					dtvcc_stream_event (dc, ds, dw, dw->curr_row);

					if (dw == ds->curr_window)
						ds->curr_window = NULL;

					if (dw->visible)
						dtvcc_render(dc, ds);

					memset (dw->buffer, 0, sizeof (dw->buffer));
					memset (dw->pen, 0, sizeof(dw->pen));
					dw->visible = 0;
					dw->effect_status = 0;
					dw->effect_percent = 0;
					dw->style.display_effect = 0;

					changed = 1;
				}
			}
		}
	}

	ds->created &= ~window_map;

	return TRUE;
}

static vbi_bool
dtvcc_delay_cmd (struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 uint8_t delay_cmd,
				 uint8_t	delay_time)
{
	struct timespec now_ts;
	int plus_one_second = 0;
	clock_gettime(CLOCK_REALTIME, &now_ts);
	if (delay_cmd == 0x8D)
	{
		/* Set trigger time
			Until that time, sevice stop decoding.
			*/
		/* Delay time is tenths of second */
		/* We set the timer a little faster to avoid double delay conflict */
		if ((1000000000 - (delay_time%10) * 100000000) > now_ts.tv_nsec)
		{
			ds->delay_timer.tv_nsec = ((delay_time % 10) * 100000000 + now_ts.tv_nsec) % 1000000000;
			ds->delay_timer.tv_sec = (1 + now_ts.tv_sec + delay_time/10) -1;
		}
		else
		{
			ds->delay_timer.tv_nsec = (delay_time % 10) * 100000000 + now_ts.tv_nsec;
			ds->delay_timer.tv_sec = (now_ts.tv_sec + delay_time / 10) -1;
		}
		ds->delay_timer.tv_sec = now_ts.tv_sec + 1;
		ds->delay = 1;
		ds->delay_cancel = 0;
		//AM_DEBUG(1, "Enter delay cmd, now %d until %d", now_ts.tv_sec, ds->delay_timer.tv_sec);
	}
	else if (delay_cmd == 0x8E)
	{
		ds->delay = 0;
		ds->delay_cancel = 1;
	}
	return TRUE;
}

static vbi_bool
dtvcc_command			(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 unsigned int *		se_length,
				 uint8_t *		buf,
				 unsigned int		n_bytes)
{
	unsigned int c;
	unsigned int window_id;


	c = buf[0];
	if ((int8_t) c < 0) {
		*se_length = dtvcc_c1_length[c - 0x80];
	} else {
		*se_length = dtvcc_c0_length[c >> 3];
	}

	if (*se_length > n_bytes) {
		ds->error_line = __LINE__;
		return FALSE;
	}
	switch (c) {
	case 0x08: /* BS Backspace */
		return dtvcc_backspace (dc, ds);

	case 0x0C: /* FF Form Feed */
		return dtvcc_form_feed (dc, ds);

	case 0x0D: /* CR Carriage Return */
		return dtvcc_carriage_return (dc, ds);

	case 0x0E: /* HCR Horizontal Carriage Return */
		return dtvcc_hor_carriage_return (dc, ds);

	case 0x80 ... 0x87: /* CWx SetCurrentWindow */
		window_id = c & 7;
		if (0 == (ds->created & (1 << window_id))) {
			ds->error_line = __LINE__;
			return FALSE;
		}
		ds->curr_window = &ds->window[window_id];
		return TRUE;

	case 0x88: /* CLW ClearWindows */
		return dtvcc_clear_windows (dc, ds, buf[1]);

	case 0x89: /* DSW DisplayWindows */
		return dtvcc_display_windows (dc, ds, c, buf[1]);

	case 0x8D:
		dtvcc_delay_cmd(dc, ds, 0x8d, buf[1]);
		return 0;

	case 0x8E:
		dtvcc_delay_cmd(dc, ds, 0x8e, buf[1]);
		return 0;

	case 0x8A: /* HDW HideWindows */
		return dtvcc_display_windows (dc, ds, c, buf[1]);

	case 0x8B: /* TGW ToggleWindows */
		return dtvcc_display_windows (dc, ds, c, buf[1]);

	case 0x8C: /* DLW DeleteWindows */
		return dtvcc_delete_windows (dc, ds, buf[1]);

	case 0x8F: /* RST Reset */
		dtvcc_reset_service (ds);
		return TRUE;

	case 0x90: /* SPA SetPenAttributes */
		return dtvcc_set_pen_attributes (ds, buf);

	case 0x91: /* SPC SetPenColor */
		return dtvcc_set_pen_color (ds, buf);

	case 0x92: /* SPL SetPenLocation */
		return dtvcc_set_pen_location (dc, ds, buf);

	case 0x97: /* SWA SetWindowAttributes */
		return dtvcc_set_window_attributes (ds, buf);

	case 0x98 ... 0x9F: /* DFx DefineWindow */
		return dtvcc_define_window (dc, ds, buf);

	default:
		return TRUE;
	}
}

static vbi_bool
dtvcc_decode_se			(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 unsigned int *		se_length,
				 uint8_t *		buf,
				 unsigned int		n_bytes)
{
	unsigned int c;
	char* lang_korea;
	lang_korea = strstr(dc->lang, "kor");

	c = buf[0];
	if (unlikely (c == 0x18) && lang_korea)
	{
		*se_length = 3;
		c = buf[1]<<8|buf[2];
		AM_DEBUG(0, "lang korea decode_se 0x%x", c);
		//Conv

		return dtvcc_put_char (dc, ds, c);
	}

	if (likely (0 != (c & 0x60))) {
		/* G0/G1 character. */
		*se_length = 1;
		return dtvcc_put_char (dc, ds, c);
	}

	if (0x10 != c) {
		/* C0/C1 control code. */
		return dtvcc_command (dc, ds, se_length,
				      buf, n_bytes);
	}

	if (unlikely (n_bytes < 2)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	c = buf[1];
	if (likely (0 != (c & 0x60))) {
		/* G2/G3 character. */
		*se_length = 2;
		return dtvcc_put_char (dc, ds, 0x1000 | c);
	}

	/* CEA 708-C defines no C2 or C3 commands. */
	if (ds->curr_window)
		ds->curr_window->latest_cmd_cr = 0;
	if ((int8_t) c >= 0) {
		/* C2 code. */
		*se_length = (c >> 3) + 2;
	} else if (c < 0x90) {
		/* C3 Fixed Length Commands. */
		*se_length = (c >> 3) - 10;
	} else {
		/* C3 Variable Length Commands. */

		if (unlikely (n_bytes < 3)) {
			ds->error_line = __LINE__;
			return FALSE;
		}

		/* type [2], zero_bit [1],
		   length [5] */
		*se_length = (buf[2] & 0x1F) + 3;
	}

	if (unlikely (n_bytes < *se_length)) {
		ds->error_line = __LINE__;
		return FALSE;
	}

	return TRUE;
}

static vbi_bool
dtvcc_decode_syntactic_elements	(struct dtvcc_decoder *	dc,
				 struct dtvcc_service *	ds,
				 uint8_t *		buf,
				 unsigned int		n_bytes)
{
	ds->timestamp = dc->timestamp;
	struct timespec ts_now;

#if 0
	AM_DEBUG(1, "+++++++++++++++++++++ servie %d\n", n_bytes);
	{
		int i;

		for (i = 0; i < n_bytes; i ++)
			AM_DEBUG(1, "++++++++++++++ %02x ", buf[i]);
	}
#endif
	while (n_bytes > 0) {
		unsigned int se_length;

		//printf("dec se %02x\n", buf[0]);

		if (!dtvcc_decode_se (dc, ds,
				      &se_length,
				      buf, n_bytes)) {
			return FALSE;
		}

		buf += se_length;
		n_bytes -= se_length;
	}

	return TRUE;
}

static void
dtvcc_try_decode_channels (struct dtvcc_decoder *dc)
{
	int i;

	for (i = 0; i < 6; ++i) {
		struct dtvcc_service *ds;
		struct program *pr;
		vbi_bool success;

		ds = &dc->service[i];
		if (0 == ds->service_data_in)
			continue;
		if (!ds->delay ||
			(ds->delay && ds->service_data_in>=128))
		{
			//AM_DEBUG(1, "service datain %d", ds->service_data_in);
			success = dtvcc_decode_syntactic_elements
				(dc, ds, ds->service_data, ds->service_data_in);
			if (ds->service_data_in >= 128)
			{
				ds->delay = 0;
				ds->delay_cancel = 0;
			}
			ds->service_data_in = 0;

			if (success)
				continue;
		}
		//dtvcc_reset_service (ds);
		//dc->next_sequence_number = -1;
	}
}

void
dtvcc_decode_packet		(struct dtvcc_decoder *	dc,
				 const struct timeval *	tv,
				 int64_t		pts)
{
	unsigned int packet_size_code;
	unsigned int packet_size;
	unsigned int i;
	int sequence_number;

#if 0
	printf("%d dtvcc decode packet %d: ", get_input_offset(), dc->packet_size);

	for (i = 0; i < dc->packet_size; i ++) {
		printf("%02x ", dc->packet[i]);
	}
	printf("\n");
#endif
	dc->timestamp.sys = *tv;
	dc->timestamp.pts = pts;

	/* Packet Layer. */

	/* sequence_number [2], packet_size_code [6],
	   packet_data [n * 8] */
#if 1
	sequence_number = dc->packet[0]>>6;
	if (dc->next_sequence_number >= 0
	    && sequence_number != dc->next_sequence_number) {
		dtvcc_reset (dc);
		return;
	}
#endif
	dc->next_sequence_number = (sequence_number+1)&3;
	//AM_DEBUG(0, "sn %d nsn %d", sequence_number, dc->next_sequence_number);
	packet_size_code = dc->packet[0] & 0x3F;
	packet_size = 128;
	if (packet_size_code > 0)
		packet_size = packet_size_code * 2;

	/* CEA 708-C Section 5: Apparently packet_size need not be
	   equal to the actually transmitted amount of data. */
	if (packet_size > dc->packet_size) {
		/*
		dtvcc_reset (dc);
		return;*/
		packet_size = dc->packet_size;
	}

	/* Service Layer. */

	/* CEA 708-C Section 6.2.5, 6.3: Service Blocks and syntactic
	   elements must not cross Caption Channel Packet
	   boundaries. */

	for (i = 1; i < packet_size;) {
		unsigned int service_number;
		unsigned int block_size;
		unsigned int header_size;
		unsigned int c;

		header_size = 1;

		/* service_number [3], block_size [5],
		   (null_fill [2], extended_service_number [6]),
		   (Block_data [n * 8]) */

		c = dc->packet[i];
		service_number = (c & 0xE0) >> 5;

		//printf("srv %d\n", service_number);

		/* CEA 708-C Section 6.3: Ignore block_size if
		   service_number is zero. */
		if (0 == service_number) {
			/* NULL Service Block Header, no more data in
			   this Caption Channel Packet. */
			dc->next_sequence_number = -1;
			break;
		}

		/* CEA 708-C Section 6.2.1: Apparently block_size zero
		   is valid, although properly it should only occur in
		   NULL Service Block Headers. */
		block_size = c & 0x1F;

		if (7 == service_number) {
			if (i + 1 > packet_size)
				break;

			header_size = 2;
			c = dc->packet[i + 1];

			/* We also check the null_fill bits. */
			if (c < 7 || c > 63)
				break;

			service_number = c;
		}

		//printf("srv %d %d %d %d\n", service_number, header_size, block_size, packet_size);

		if (i + header_size + block_size > packet_size)
		{
			break;
		}

		if (service_number <= 6) {
			struct dtvcc_service *ds;
			unsigned int in;

			ds = &dc->service[service_number - 1];
			in = ds->service_data_in;
			memcpy (ds->service_data + in,
				dc->packet + i + header_size,
				block_size);
			ds->service_data_in = in + block_size;
		}

		i += header_size + block_size;
	}

	dtvcc_try_decode_channels(dc);
	return;
}

static int
dtvcc_get_se_len (unsigned char *p, int left)
{
	unsigned char c;
	int se_length;

	if (left < 1)
		return 0;

	c = p[0];

	if ((c == 0x8d) && (c == 0x8e))
		return 1;

	if (0 != (c & 0x60))
		return 1;

	if (0x10 != c) {
		if ((int8_t) c < 0) {
			se_length = dtvcc_c1_length[c - 0x80];
		} else {
			se_length = dtvcc_c0_length[c >> 3];
		}

		if (left < se_length)
			return 0;

		return se_length;
	}

	if (left < 2)
		return 0;

	c = p[1];
	if (0 != (c & 0x60))
		return 2;

	if ((int8_t) c >= 0) {
		se_length = (c >> 3) + 2;
	} else if (c < 0x90) {
		se_length = (c >> 3) - 10;
	} else {
		if (left < 3)
			return 0;

		se_length = (p[2] & 0x1F) + 3;
	}

	if (left < se_length)
		return 0;

	return se_length;
}

void
dtvcc_try_decode_packet		(struct dtvcc_decoder *	dc,
				 const struct timeval *	tv,
				 int64_t		pts,
				 int* pgno)
{
	unsigned int packet_size_code;
	unsigned int packet_size;
	unsigned char *p;
	int left;

	if (dc->packet_size < 1)
		return;

	packet_size_code = dc->packet[0] & 0x3F;

	packet_size = 128;
	if (packet_size_code > 0)
		packet_size = packet_size_code * 2;

	if (packet_size <= dc->packet_size) {
		dtvcc_decode_packet(dc, tv, pts);
		dc->packet_size = 0;
		return;
	}

	p    = dc->packet + 1;
	left = dc->packet_size - 1;
	while (left > 0) {
		unsigned int service_number;
		unsigned int block_size;
		unsigned int header_size;
		unsigned int c;

		header_size = 1;

		c = p[0];
		service_number = (c & 0xE0) >> 5;
		if (0 == service_number)
			break;

		block_size = c & 0x1F;

		if (7 == service_number) {
			if (left < 2)
				break;

			header_size = 2;
			c = p[1];

			if (c < 7 || c > 63)
				break;

			service_number = c;
		}
		if (service_number <= 6)
			*pgno = service_number;

		if (left >= header_size + block_size) {
			if (service_number <= 6) {
				struct dtvcc_service *ds;
				unsigned int in;

				ds = &dc->service[service_number - 1];
				in = ds->service_data_in;
				if (block_size + in > 128)
				{
					AM_DEBUG(0, "Unexpect data length, warn");
					block_size = 128 - in;
				}
				memcpy (ds->service_data + in,
						p + header_size,
						block_size);

				ds->service_data_in = in + block_size;
			}
		} else {
			unsigned char *s = p + header_size;
			int sleft = left - header_size;

			while (sleft > 0) {
				int se_len;

				se_len = dtvcc_get_se_len(s, sleft);
				if (se_len <= 0)
					break;

				s     += se_len;
				sleft -= se_len;
			}

			if (sleft != left - header_size) {
				int parsed = left - header_size - sleft;

				if (service_number <= 6) {
					struct dtvcc_service *ds;
					unsigned int in;

					ds = &dc->service[service_number - 1];
					in = ds->service_data_in;
					if (parsed + in > 128)
					{
						parsed = 128 - in;
						AM_DEBUG(0, "Unexpect data length, warn");
					}
					memcpy (ds->service_data + in,
							p + header_size,
							parsed);

					ds->service_data_in = in + parsed;
				}

				memmove(p + header_size, s, sleft);
				block_size -= parsed;
				left -= parsed;

				p[0] &= ~0x1f;
				p[0] |= block_size;
			}
			break;
		}

		p    += header_size + block_size;
		left -= header_size + block_size;
	}

	if (left != dc->packet_size - 1) {
		int parsed = dc->packet_size - 1 - left;

		memmove(dc->packet + 1, p, left);

		packet_size_code = ((dc->packet[0] & 0x3f) << 1) - parsed;
		if (packet_size_code & 1)
			packet_size_code ++;
		packet_size_code >>= 1;

		dc->packet[0] &= ~0x3f;
		dc->packet[0] |= packet_size_code;
		dc->packet_size = left + 1;

		dtvcc_try_decode_channels(dc);
	}
}

static void
dtvcc_reset_service		(struct dtvcc_service *	ds)
{
	int i;
	ds->curr_window = NULL;
	ds->created = 0;
	ds->delay = 0;
	ds->delay_cancel = 0;

	struct dtvcc_window *dw;
	for (i=0;i<8;i++)
	{
		dw = &ds->window[i];
		ds->window[i].visible = 0;
		ds->window[i].latest_cmd_cr = 0;
		memset (dw->buffer, 0, sizeof (dw->buffer));
		memset (dw->pen, 0, sizeof(dw->pen));
		dw->effect_status = 0;
		dw->streamed = 0;
	}

	ds->update = 1;
	cc_timestamp_reset (&ds->timestamp);
}

void
dtvcc_reset			(struct dtvcc_decoder *	dc)
{
	int i;
	for (i=0; i<N_ELEMENTS(dc->service); i++)
		dtvcc_reset_service (&dc->service[i]);

	dc->packet_size = 0;
	dc->next_sequence_number = -1;
}

void
dtvcc_init		(struct dtvcc_decoder *	dc, char* lang, int lang_len, int decoder_param)
{
	int i;
	memset(dc, 0, sizeof(struct dtvcc_decoder));
	dtvcc_reset (dc);
	cc_timestamp_reset (&dc->timestamp);
	for (i=0;i<6;i++)
		dc->service[i].id = i;
	strncpy(dc->lang, lang, lang_len);
	dc->decoder_param = decoder_param;
}

static void dtvcc_window_to_page(vbi_decoder *vbi, struct dtvcc_window *dw, struct vbi_page *pg)
{
	int i, j, c;
	vbi_char ac;
	vbi_opacity fg_opacity, bg_opacity;

	static const vbi_opacity vbi_opacity_map[]={
		VBI_OPAQUE,
		VBI_OPAQUE,
		VBI_SEMI_TRANSPARENT,
		VBI_TRANSPARENT_SPACE
	};

	memset(pg, 0, sizeof(struct vbi_page));
#if 1
	pg->rows = dw->row_count;
	pg->columns = dw->column_count;

	dtvcc_set_page_color_map(vbi, pg);
	memset(dw->row_start, 0, sizeof(dw->row_start));

	for (i=0; i<pg->rows; i++)
	{
		for (j=0; j<pg->columns; j++)
		{
			memset(&ac, 0, sizeof(ac));
			/*use the curr_pen to draw all the text, actually this isn't reasonable*/
			if (dw->curr_pen.style.fg_opacity >= N_ELEMENTS(vbi_opacity_map)){
				fg_opacity = VBI_OPAQUE;
			}else{
				fg_opacity = vbi_opacity_map[dw->curr_pen.style.fg_opacity];
			}

			if (dw->curr_pen.style.bg_opacity >= N_ELEMENTS(vbi_opacity_map)){
				bg_opacity = VBI_OPAQUE;
			}else{
				bg_opacity = vbi_opacity_map[dw->curr_pen.style.bg_opacity];
			}

			ac.opacity = (fg_opacity<<4) | bg_opacity;
			ac.foreground = dtvcc_map_color(dw->curr_pen.style.fg_color);
			ac.background = dtvcc_map_color(dw->curr_pen.style.bg_color);

			c = dw->buffer[i][j];
			if (0 == c) {
				ac.unicode = 0x20;
				ac.opacity = VBI_TRANSPARENT_SPACE;
				dw->row_start[i] ++;
			} else {
				ac.unicode = dtvcc_unicode (c);
				if (0 == ac.unicode) {
					ac.unicode = 0x20;
					dw->row_start[i] ++;
				}
			}
			pg->text[i*pg->columns + j] = ac;
		}
	}
#else
	pg->rows = 15;
	pg->columns = 42;
	dtvcc_set_page_color_map(vbi, pg);
	for (i=0; i<pg->rows; i++)
	{
		for (j=0; j<pg->columns; j++)
		{
			memset(&ac, 0, sizeof(ac));
			ac.opacity = VBI_OPAQUE;
			ac.foreground = VBI_WHITE;
			ac.background = VBI_BLACK;
			if (j == 0)
				c = '0' + (i%10);
			else
				c = '0' + (j%10);
			ac.unicode = dtvcc_unicode (c);
			LOGI("TEXT(%x): %c", ac.unicode, ac.unicode);
			pg->text[i*pg->columns + j] = ac;
		}
	}
#endif
	pg->screen_color = 0;
	pg->screen_opacity = VBI_OPAQUE;
	//pg->font[0] = vbi_font_descriptors; /* English */
	//pg->font[1] = vbi_font_descriptors;
}

static int dtvcc_compare_window_priority(const void *window1, const void *window2)
{
	struct dtvcc_window *w1 = (struct dtvcc_window*)window1;
	struct dtvcc_window *w2 = (struct dtvcc_window*)window2;

	return (w1->priority - w2->priority);
}

static void dtvcc_get_visible_windows(struct dtvcc_service *ds, int *cnt, struct dtvcc_window **windows)
{
	int i, j = 0;

	for (i=0; i<N_ELEMENTS(ds->window); i++){
		if (ds->window[i].visible && j < *cnt){
			windows[j++] = &ds->window[i];
		}
	}

	qsort(windows, j, sizeof(windows[0]), dtvcc_compare_window_priority);

	*cnt = j;
}

void tvcc_fetch_page(struct tvcc_decoder *td, int pgno, int *sub_cnt, struct vbi_page *sub_pages)
{
	int sub_pg = 0;

	if (pgno < 1 || pgno > 14 || *sub_cnt <= 0)
		goto fetch_done;

	if (pgno <= 8){
		if (vbi_fetch_cc_page(td->vbi, &sub_pages[0], pgno, 1)){
			sub_pg = 1;
			sub_pages[0].pgno = pgno;
		}
	}else{
		int i;
		struct dtvcc_service *ds = &td->dtvcc.service[pgno - 1 - 8];
		struct dtvcc_window *dw;
		struct dtvcc_window *visible_windows[8];

		sub_pg = *sub_cnt;
		if (sub_pg > 8)
			sub_pg = 8;

		dtvcc_get_visible_windows(ds, &sub_pg, visible_windows);

		for (i=0; i<sub_pg; i++){
			dw = visible_windows[i];

			dtvcc_window_to_page(td->vbi, dw, &sub_pages[i]);
			sub_pages[i].vbi = td->vbi;
			sub_pages[i].pgno = pgno;
			sub_pages[i].subno = dw - ds->window;
		}
	}

fetch_done:
	*sub_cnt = sub_pg;
}

/* ATSC A/53 Part 4:2007 Closed Caption Data decoder */

/* Only handle effect */
static void update_service_status_internal (struct tvcc_decoder *td)
{
	int i, j, k, l;
	struct timespec ts_now;
	struct dtvcc_decoder *decoder;
	struct dtvcc_pen_style *target_pen;
	int flash;

	decoder = &td->dtvcc;
	clock_gettime(CLOCK_REALTIME, &ts_now);

	flash = (ts_now.tv_nsec / 250000000) & 1;

	/* CS1 - CS6 */
	for (i = 0; i < 6; ++i)
	{
		struct dtvcc_service *ds;
		struct program *pr;
		vbi_bool success;
		ds = &decoder->service[i];
		if (ds->created == 0)
			continue;
		/* Check every effect */
		if (ds->delay)
		{
			struct vbi_event event;
			/* time is up */
			if ((ts_now.tv_sec > ds->delay_timer.tv_sec) ||
			((ts_now.tv_sec == ds->delay_timer.tv_sec) &&(ts_now.tv_nsec > ds->delay_timer.tv_nsec)) ||
			ds->delay_cancel)
			{
				//AM_DEBUG(1, "delay timeup");
				ds->delay = 0;
				ds->delay_cancel = 0;
				dtvcc_decode_syntactic_elements
					(decoder, ds, ds->service_data, ds->service_data_in);

				ds->service_data_in = 0;
			}
		}

		if (flash == decoder->flash_state)
			continue;

		for (j = 0; j < 8; j++)
		{
			struct dtvcc_window *target_window;
			target_window = &ds->window[j];
			if (target_window->visible == 0)
				continue;
	        /*window flash treatment */
	        if (target_window->style.window_flash)
				ds->update = 1;

			/* Wipe and fade treatment */
			if (target_window->style.display_effect != 0 &&
				target_window->effect_status != 0)
			{
				target_window->effect_percent =
					((ts_now.tv_sec - target_window->effect_timer.tv_sec) * 1000 +
					(ts_now.tv_nsec - target_window->effect_timer.tv_nsec) / 1000000) *100/
					(target_window->style.effect_speed * 500);
				if (target_window->effect_percent > 100)
					target_window->effect_percent = 100;
				ds->update = 1;
			}

			/* Pen flash treatment */
			for (k = 0; k < 16; k++)
			{
				for (l=0; l<42; l++)
				{
					target_pen = &target_window->pen[k][l];
					if (target_pen->bg_flash)
						ds->update = 1;
					if (target_pen->fg_flash)
						ds->update = 1;
				}
			}
	        }
        }

	decoder->flash_state = flash;
}

static void
update_display (struct tvcc_decoder *td)
{
	int i;

	for (i = 0; i < N_ELEMENTS(td->dtvcc.service); i ++) {
		struct dtvcc_service *ds = &td->dtvcc.service[i];
		//if (ds->created == 0)
		//		continue;

		if (ds->update) {
			struct vbi_event event;

			event.type = VBI_EVENT_CAPTION;
			event.ev.caption.pgno = i + 1 + 8/*after 8 cc channels*/;
			event.ev.caption.inform_pgno = 0;

			/* Permits calling tvcc_fetch_page from handler */
			pthread_mutex_unlock(&td->mutex);

			vbi_send_event(td->vbi, &event);
			pthread_mutex_lock(&td->mutex);
			ds->update = 0;
		}
	}
}

static void
notify_curr_data_pgno(struct tvcc_decoder *td, int pgno)
{
	struct vbi_event event;

	event.type = VBI_EVENT_CAPTION;
	event.ev.caption.pgno = pgno + 8/*after 8 cc channels*/;
	event.ev.caption.inform_pgno = 1;

	/* Permits calling tvcc_fetch_page from handler */
	pthread_mutex_unlock(&td->mutex);
	vbi_send_event(td->vbi, &event);
	pthread_mutex_lock(&td->mutex);
}


/* Note pts may be < 0 if no PTS was received. */
void
tvcc_decode_data			(struct tvcc_decoder *td,
				 int64_t		pts,
				 const uint8_t *	buf,
				 unsigned int		n_bytes)
{
	unsigned int process_cc_data_flag;
	unsigned int cc_count;
	unsigned int i, max_cc_count;
	vbi_bool dtvcc;
	struct timeval now;
	int pgno = -1;

	if (buf[0] != 0x03)
		return;
	process_cc_data_flag = buf[1] & 0x40;
	if (!process_cc_data_flag)
	{
		return;
	}

	cc_count = buf[1] & 0x1F;
	dtvcc = FALSE;

#if 0
	char dbuff[1024];
	for (i=0; i<n_bytes; i++)
		sprintf(dbuff + 3*i, " %02x", buf[i]);
	AM_DEBUG(0, "incoming data: %s", dbuff);
#endif
	pthread_mutex_lock(&td->mutex);

	//printf("cc count %d\n", cc_count);
	max_cc_count = (n_bytes - 3)/3;
	cc_count = (cc_count > max_cc_count)?max_cc_count:cc_count;

	for (i = 0; i < cc_count; ++i) {
		unsigned int b0;
		unsigned int cc_valid;
		enum cc_type cc_type;
		unsigned int cc_data_1;
		unsigned int cc_data_2;
		unsigned int j;

		b0 = buf[3 + i * 3];
		cc_valid = b0 & 4;
		cc_type = (enum cc_type)(b0 & 3);
		cc_data_1 = buf[4 + i * 3];
		cc_data_2 = buf[5 + i * 3];

		AM_DEBUG(4,"cc type %02x %02x %02x %02x\n", cc_type, cc_valid, cc_data_1, cc_data_2);

		switch (cc_type) {
		case NTSC_F1:
		case NTSC_F2:
			//printf("ntsc cc\n");
			/* Note CEA 708-C Table 4: Only one NTSC pair
			   will be present in field picture user_data
			   or in progressive video pictures, and up to
			   three can occur if the frame rate < 30 Hz
			   or repeat_first_field = 1. */
			if (!cc_valid || i >= 3 || dtvcc) {
				/* Illegal, invalid or filler. */
				break;
			}
#if 0
			cc_feed (&td->cc, &buf[12 + i * 3],
				 /* line */ (NTSC_F1 == cc_type) ? 21 : 284,
				 &now, pts);
#endif
			vbi_decode_caption(td->vbi, (NTSC_F1 == cc_type) ? 21 : 284, &buf[4 + i * 3]);
			break;

		case DTVCC_DATA:
			j = td->dtvcc.packet_size;
			if (j <= 0) {
				/* Missed packet start. */
				break;
			} else if (!cc_valid) {
				/* End of DTVCC packet. */
			} else if (j + 2 > 128) {
				/* Packet buffer overflow. */
				dtvcc_reset (&td->dtvcc);
				td->dtvcc.packet_size = 0;
			} else {
				td->dtvcc.packet[j] = cc_data_1;
				td->dtvcc.packet[j + 1] = cc_data_2;
				td->dtvcc.packet_size = j + 2;

				dtvcc_try_decode_packet (&td->dtvcc, &now, pts, &pgno);
			}
			break;

		case DTVCC_START:
			dtvcc = TRUE;
			j = td->dtvcc.packet_size;
			if (j > 0) {
				/* End of DTVCC packet. */
				dtvcc_decode_packet (&td->dtvcc,
						     &now, pts);
			}
			if (!cc_valid) {
				/* No new data. */
				td->dtvcc.packet_size = 0;
			} else {
				td->dtvcc.packet[0] = cc_data_1;
				td->dtvcc.packet[1] = cc_data_2;
				td->dtvcc.packet_size = 2;
				dtvcc_try_decode_packet(&td->dtvcc, &now, pts, &pgno);
			}
			break;
		}
	}
	if (pgno != -1)
	    notify_curr_data_pgno(td, pgno);

	update_service_status_internal(td);
	update_display(td);

	pthread_mutex_unlock(&td->mutex);
}

/* Only handle effect */
void update_service_status(struct tvcc_decoder *td)
{
	pthread_mutex_lock(&td->mutex);

	update_service_status_internal(td);
	update_display(td);

	pthread_mutex_unlock(&td->mutex);
}

void tvcc_init(struct tvcc_decoder *td, char* lang, int lang_len, unsigned int decoder_param)
{
	pthread_mutex_init(&td->mutex, NULL);
	dtvcc_init(&td->dtvcc, lang, lang_len, decoder_param);
	AM_DEBUG(0, "tvcc_init lang %s dp %x", lang, decoder_param);
	cc_init(&td->cc);
	td->vbi = vbi_decoder_new();
}

void tvcc_destroy(struct tvcc_decoder *td)
{
	if (td){
		vbi_decoder_delete(td->vbi);
		td->vbi = NULL;
		pthread_mutex_destroy(&td->mutex);
	}
}

void tvcc_reset(struct tvcc_decoder *td)
{
	dtvcc_reset(&td->dtvcc);
	cc_reset(&td->cc);
}

