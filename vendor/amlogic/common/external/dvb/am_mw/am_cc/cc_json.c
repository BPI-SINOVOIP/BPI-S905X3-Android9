/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "dtvcc.h"
#include <am_iconv.h>
#include "am_debug.h"


typedef struct {
	char *po;
	int   left;
	int   has_prop;
	int flash;
} Output;

static int
put_uc (uint8_t *buf, int uc)
{
	int r;

	if (uc <= 0x7f) {
		buf[0] = uc;
		r = 1;
	} else if (uc <= 0x7ff) {
		buf[0] = (uc >> 6) | 0xc0;
		buf[1] = (uc & 0x3f) | 0x80;
		r = 2;
	} else {
		buf[0] = (uc >> 12) | 0xe0;
		buf[1] = ((uc >> 6) & 0x3f) | 0x80;
		buf[2] = (uc & 0x3f) | 0x80;
		r = 3;
	}

	return r;
}

static void
o_init (Output *out, char *buf, size_t len)
{
	struct timespec ts_now;
	clock_gettime(CLOCK_REALTIME, &ts_now);
	out->po   = buf;
	out->left = len;
	out->has_prop = 0;
	if ((ts_now.tv_nsec /250000000)&1)
		out->flash = 1;
	else
		out->flash = 0;
}

static int
o_printf (Output *out, const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);

	r = vsnprintf(out->po, out->left, fmt, ap);

	va_end(ap);

	if (r >= out->left)
		return -1;

	out->po   += r;
	out->left -= r;

	return r;
}

static inline int
o_prop_begin (Output *out, const char *key)
{
	if (out->has_prop) {
		if (o_printf(out, ",") < 0)
			return -1;
	}

	if (o_printf(out, "\"%s\":", key) < 0)
		return -1;

	out->has_prop = 0;
	return 0;
}

static inline int
o_prop_end (Output *out)
{
	out->has_prop = 1;

	return 0;
}

static int
o_symbol (Output *out, char sym)
{
	if (out->has_prop && ((sym == '{') || (sym == '['))) {
		if (o_printf(out, ",") < 0)
			return -1;
	}

	if (o_printf(out, "%c", sym) < 0)
		return -1;

	if ((sym == '[') || (sym == '{'))
		out->has_prop = 0;
	else if ((sym == ']') || (sym == '}'))
		out->has_prop = 1;

	return 0;
}

static int
o_bool_prop (Output *out, const char *key, int val)
{
	if (o_prop_begin(out, key) < 0)
		return -1;

	if (o_printf(out, "%s", val ? "true" : "false") < 0)
		return -1;

	if (o_prop_end(out) < 0)
		return -1;

	return 0;
}

static int
o_int_prop (Output *out, const char *key, int val)
{
	if (o_prop_begin(out, key) < 0)
		return -1;

	if (o_printf(out, "%d", val) < 0)
		return -1;

	if (o_prop_end(out) < 0)
		return -1;

	return 0;
}

static int
o_str_prop (Output *out, const char *key, const char *val)
{
	uint8_t *pi;

	if (o_prop_begin(out, key) < 0)
		return -1;

	if (o_printf(out, "\"") < 0)
		return -1;

	pi = (uint8_t*)val;


#define PUTC(c)\
	do {\
		if (!out->left)\
			return -1;\
		out->po[0] = c;\
		out->po ++;\
		out->left --;\
	} while (0)

	while (*pi) {
		switch (*pi) {
		case '\\':
			PUTC('\\');
			PUTC('\\');
			break;
		case '\n':
			PUTC('\\');
			PUTC('n');
			break;
		case '\r':
			PUTC('\\');
			PUTC('r');
			break;
		case '\"':
			PUTC('\\');
			PUTC('"');
			break;
		default:
			PUTC(*pi);
			break;
		}
		pi ++;
	}

	if (o_printf(out, "\"") < 0)
		return -1;

	if (o_prop_end(out) < 0)
		return -1;

	return 0;
}

static int
vbi_style_cmp (vbi_char *s1, vbi_char *s2)
{
	if (s1->foreground != s2->foreground)
		return 1;
	if (s1->background != s2->background)
		return 1;
	if (s1->opacity != s2->opacity)
		return 1;
	if (s1->underline != s2->underline)
		return 1;
	if (s1->italic != s2->italic)
		return 1;
	if (s1->flash != s2->flash)
			return 1;

	return 0;
}

static char*
vbi_transparent_to_str (int v)
{
	char *str;

	if (v == VBI_TRANSPARENT_SPACE) {
		str = "transparent";
	} else if (v == VBI_SEMI_TRANSPARENT) {
		str = "translucent";
	} else {
		str = "solid";
	}

	return str;
}

static int
vbi_color_value (vbi_rgba col)
{
	int r, g, b, v;

	r = col & 0xff;
	g = (col >> 8) & 0xff;
	b = (col >> 16) & 0xff;
	v = ((r >> 6) << 4) | ((g >> 6) << 2) | (b >> 6);

	return v;
}

static char*
effect_status_to_str(enum cc_effect_status status)
{
	char* str;
	switch (status)
	{
		case CC_EFFECT_NONE:
			str = "NONE";
		case CC_EFFECT_DISPLAY:
			str = "DISPLAY";
		case CC_EFFECT_HIDE:
			str = "HIDE";
		default:
			str = "NONE";
	};
	return str;
}

static int
vbi_text_to_json (struct vbi_page *pg, uint8_t *buf, int len, vbi_char *st, Output *out)
{
	uint8_t  *pb = NULL;
	vbi_rgba  col;
	int       o;
	int i;
	uint8_t blank_str[64];

	if (!len)
		return 0;
/*
	for (pb = buf; pb < buf + len; pb ++) {
		if (*pb != 0x20)
			break;
	}
*/
/*
	if (pb >= buf + len)
		return 0;
*/
	buf[len] = 0;

	if (o_symbol(out, '{') < 0)
		return -1;

	col = pg->color_map[st->foreground];
	if (o_int_prop(out, "fg_color", vbi_color_value(col)) < 0)
		return 0;

	col = pg->color_map[st->background];
	if (o_int_prop(out, "bg_color", vbi_color_value(col)) < 0)
		return 0;
	o = st->flash;
	if (o)
	{
		if (out->flash)
			o = VBI_TRANSPARENT_SPACE;
		else
			o = VBI_OPAQUE;

		if (o_str_prop(out, "fg_opacity", vbi_transparent_to_str(o)) < 0)
			return -1;
	}
	else
	{
		o = VBI_OPAQUE;
		if (o_str_prop(out, "fg_opacity", vbi_transparent_to_str(o)) < 0)
			return -1;
	}
	o = st->opacity;
	/* It seems like no independent attribute to control background opacity */
		if (o_str_prop(out, "bg_opacity", vbi_transparent_to_str(o)) < 0)
			return -1;
	o = st->underline;
		if (o_bool_prop(out, "underline", o) < 0)
			return -1;
	o = st->italic;
	if (o_bool_prop(out, "italics", o) < 0)
		return -1;
	o = st->flash;

	if (o && (out->flash))
	{
		for (i=0; i<strlen(buf);i++)
			blank_str[i] = 0x20;
		blank_str[strlen(buf)] = 0;
		if (o_str_prop(out, "data", (char*)blank_str) < 0)
			return -1;
	}
	else
		if (o_str_prop(out, "data", (char*)buf) < 0)
			return -1;

	if (o_symbol(out, '}') < 0)
		return -1;

	return 0;
}

static int
vbi_page_row_to_json (struct vbi_page *pg, Output *out, int row, vbi_char *st)
{
	uint8_t buf[pg->columns * 3 + 1];
	uint8_t *pb = buf;
	int len = 0;
	int col;
	int      start = 0, continuance_flag = 0;

	if (o_symbol(out, '{') < 0)
			return -1;

		if (o_prop_begin(out, "content") < 0)
			return -1;

	if (o_symbol(out, '[') < 0)
		return -1;
	for (col = 0; col < pg->columns; col ++) {
		vbi_char *pc = &pg->text[row * pg->columns + col];
		int uc = pc->unicode;
		int r;
		//AM_DEBUG(1, "unicode %x op %d start %d", uc, pc->opacity,start);
		if (vbi_style_cmp(st, pc)) {
			if (vbi_text_to_json(pg, buf, len, st, out) < 0)
				return -1;

			pb  = buf;
			len = 0;

			*st = *pc;
		}

		if (uc == 0 && continuance_flag == 0) {
			start ++;
		}
		else
			continuance_flag = 1;

		//if (!uc)
		//	uc = 0x20;

		if (continuance_flag) {
		if (uc == 0)
			break;
		r = put_uc(pb, uc);
		pb  += r;
		len += r;
		}
	}

	if (vbi_text_to_json(pg, buf, len, st, out) < 0)
		return -1;
	if (o_symbol(out, ']') < 0)
		return -1;
	if (o_prop_end(out) < 0)
		return -1;
	if (o_int_prop(out, "row_start", start) < 0)
		return -1;
	if (o_symbol(out, '}') < 0)
			return -1;

	return 0;
}

static int
vbi_page_to_json (struct vbi_page *pg, Output *out)
{
	vbi_char st;
	int i;

	if (o_symbol(out, '{') < 0)
		return -1;

	if (o_str_prop(out, "type", "cea608") < 0)
		return -1;

	if (o_prop_begin(out, "windows") < 0)
		return -1;

	if (o_symbol(out, '[') < 0)
		return -1;

	if (o_symbol(out, '{') < 0)
		return -1;

	if (o_int_prop(out, "anchor_point", 0) < 0)
		return -1;

	if (o_int_prop(out, "anchor_horizontal", 0) < 0)
		return -1;

	if (o_int_prop(out, "anchor_vertical", 0) < 0)
		return -1;

	if (o_bool_prop(out, "anchor_relative", 0) < 0)
		return -1;

	if (o_int_prop(out, "row_count", 15) < 0)
		return -1;

	if (o_int_prop(out, "column_count", 42) < 0)
		return -1;

	if (o_bool_prop(out, "row_lock", 1) < 0)
		return -1;

	if (o_bool_prop(out, "column_lock", 1) < 0)
		return -1;

	if (o_str_prop(out, "justify", "left") < 0)
		return -1;

	if (o_str_prop(out, "print_direction", "left_right") < 0)
		return -1;

	if (o_str_prop(out, "scroll_direction", "bottom_top") < 0)
		return -1;

	if (o_bool_prop(out, "wordwrap", 1) < 0)
		return -1;

	if (o_str_prop(out, "display_effect", "snap") < 0)
		return -1;

	if (o_str_prop(out, "effect_direction", "left_right") < 0)
		return -1;

	if (o_int_prop(out, "effect_speed", 0) < 0)
		return -1;

	/* Text mode need a black background */
	if (pg->pgno >= 5 && pg->pgno <= 8)
	{
		if (o_str_prop(out, "fill_opacity", "solid") < 0)
			return -1;
	}
	else
	{
		if (o_str_prop(out, "fill_opacity", "transparent") < 0)
			return -1;
	}

	if (o_int_prop(out, "fill_color", 0) < 0)
		return -1;

	if (o_str_prop(out, "border_type", "none") < 0)
		return -1;

	if (o_int_prop(out, "border_color", 0) < 0)
		return -1;

	if (o_prop_begin(out, "rows") < 0)
		return -1;

	if (o_symbol(out, '[') < 0)
		return -1;

	memset(&st, 0, sizeof(st));
	for (i = 0; i < pg->rows; i ++) {
		vbi_page_row_to_json(pg, out, i, &st);
	}

	if (o_symbol(out, ']') < 0)
		return -1;

	if (o_symbol(out, '}') < 0)
		return -1;

	if (o_symbol(out, ']') < 0)
		return -1;

	if (o_symbol(out, '}') < 0)
		return -1;

	return 0;
}

static int
cmp_win_prio (const void *p1, const void *p2)
{
	struct dtvcc_window *w1, *w2;

	w1 = *(struct dtvcc_window**)p1;
	w2 = *(struct dtvcc_window**)p2;

	return w1->priority - w2->priority;
}

static char*
direction_to_str (enum direction dir)
{
	char *str;

	switch (dir) {
	case DIR_LEFT_RIGHT:
	default:
		str = "left_right";
		break;
	case DIR_RIGHT_LEFT:
		str = "right_left";
		break;
	case DIR_TOP_BOTTOM:
		str = "top_bottom";
		break;
	case DIR_BOTTOM_TOP:
		str = "bottom_top";
		break;
	}

	return str;
}

static char*
opacity_to_str (enum opacity v)
{
	char *str;

	switch (v) {
	case OPACITY_SOLID:
	default:
		str = "solid";
		break;
	case OPACITY_FLASH:
		str = "flash";
		break;
	case OPACITY_TRANSLUCENT:
		str = "translucent";
		break;
	case OPACITY_TRANSPARENT:
		str = "transparent";
		break;
	}

	return str;
}

static char*
edge_to_str (enum edge e)
{
	char *str;

	switch (e) {
	case EDGE_NONE:
		str = "none";
		break;
	case EDGE_RAISED:
		str = "raised";
		break;
	case EDGE_DEPRESSED:
		str = "depressed";
		break;
	case EDGE_UNIFORM:
		str = "uniform";
		break;
	case EDGE_SHADOW_LEFT:
		str = "shadow_left";
		break;
	case EDGE_SHADOW_RIGHT:
		str = "shadow_right";
		break;
	default:
		str = "none";
		// AM_DEBUG(1, "edge style error: %s", e);
		break;
	}

	return str;
}

static int
text_to_json (uint8_t *buf, int len, struct dtvcc_pen_style *pt, Output *out)
{
	char *str;

	if (o_symbol(out, '{') < 0)
		return -1;

	switch (pt->pen_size) {
	case PEN_SIZE_SMALL:
		str = "small";
		break;
	case PEN_SIZE_STANDARD:
	default:
		str = "standard";
		break;
	case PEN_SIZE_LARGE:
		str = "large";
		break;
	}

	if (o_str_prop(out, "pen_size", str) < 0)
		return -1;

	switch (pt->font_style) {
	case FONT_STYLE_DEFAULT:
	default:
		str = "default";
		break;
	case FONT_STYLE_MONO_SERIF:
		str = "mono_serif";
		break;
	case FONT_STYLE_PROP_SERIF:
		str = "prop_serif";
		break;
	case FONT_STYLE_MONO_SANS:
		str = "mono_sans";
		break;
	case FONT_STYLE_PROP_SANS:
		str = "prop_sans";
		break;
	case FONT_STYLE_CASUAL:
		str = "casual";
		break;
	case FONT_STYLE_CURSIVE:
		str = "cursive";
		break;
	case FONT_STYLE_SMALL_CAPS:
		str = "small_caps";
		break;
	}

	if (o_str_prop(out, "font_style", str) < 0)
		return -1;

	switch (pt->offset) {
	case OFFSET_SUBSCRIPT:
		str = "subscript";
		break;
	case OFFSET_NORMAL:
	default:
		str = "normal";
		break;
	case OFFSET_SUPERSCRIPT:
		str = "subscript";
		break;
	}

	if (o_str_prop(out, "offset", str) < 0)
		return -1;

	if (o_bool_prop(out, "italics", pt->italics) < 0)
		return -1;

	if (o_bool_prop(out, "underline", pt->underline) < 0)
		return -1;

	if (o_str_prop(out, "edge_type", edge_to_str(pt->edge_type)) < 0)
		return -1;

	if (o_int_prop(out, "edge_color", pt->edge_color) < 0)
		return -1;

	if (o_int_prop(out, "fg_color", pt->fg_color) < 0)
		return -1;

	if (o_str_prop(out, "fg_opacity", opacity_to_str(pt->fg_opacity)) < 0)
		return -1;

	if (o_int_prop(out, "bg_color", pt->bg_color) < 0)
		return -1;

	if (o_str_prop(out, "bg_opacity", opacity_to_str(pt->bg_opacity)) < 0)
		return -1;

	buf[len] = 0;
#if 0
	iconv_t cd = iconv_open("utf-8", "euc-kr");
	if (cd == -1) AM_DEBUG(0, "iconv open failed");
	char tobuffer[1024] = {0};
	int outLen = 1024;
	char* tmpTobuffer = tobuffer;
	int ret_len = iconv(cd, (const char**)&buf, (size_t *)&len, &tmpTobuffer, (size_t *)&outLen);
	iconv_close(cd);
#endif
	if (o_str_prop(out, "data", (char*)buf) < 0)
		return -1;

	if (o_symbol(out, '}') < 0)
		return -1;

	return 0;
}

static int
row_to_json (struct tvcc_decoder *td, struct dtvcc_window *win, Output *out, int row, struct dtvcc_pen_style *pt)
{
	//uint8_t  buf[win->column_count * 3 + 1];
	uint8_t  buf[1024];
	uint8_t *pb  = buf;
	int      len = 0;
	int      col;
	int      start = 0, continuance_flag = 0;

	if (o_symbol(out, '{') < 0)
		return -1;

	if (o_prop_begin(out, "content") < 0)
		return -1;

	if (o_symbol(out, '[') < 0)
		return -1;

	for (col = 0; col < win->column_count; col ++) {
		int c = win->buffer[row][col];
		struct dtvcc_pen_style *cpt = &win->pen[row][col];
		uint32_t uc;
		int r;

		if (c == 0 && continuance_flag == 0) {
			start ++;
		}
		else
			continuance_flag = 1;

	char* lang_korea;
	int lang_korea_unicode;
	lang_korea = strstr(td->dtvcc.lang, "kor");
	lang_korea_unicode = td->dtvcc.decoder_param & (0x1<<13);
	if (lang_korea && (lang_korea_unicode == 0))
	{
		AM_DEBUG(0, "cc_json convert json in kor lang");
		if (c) {
			iconv_t cd = iconv_open("utf-8", "euc-kr");
			if (cd == -1)
				AM_DEBUG(0, "iconv open failed");
			char tobuffer[16] = {0};
			size_t outLen = 16;
			size_t inLen = 2;
			char* in_pointer = &c;
			char inbuffer[2];
			char* srcStart = inbuffer;
			if (!in_pointer[1])
			{
				inbuffer[0] = in_pointer[0];
				inbuffer[1] = 0;
			}
			else
			{
				inbuffer[0] = in_pointer[1];
				inbuffer[1] = in_pointer[0];
			}
			char* tmpTobuffer = tobuffer;
			*pt = *cpt;
			int ret_len = iconv(cd, (const char**)&srcStart, (size_t *)&inLen, &tmpTobuffer, (size_t *)&outLen);
			AM_DEBUG(0, "ret: %d in: %02x%02x inLen: %d outLen: %d out: %s outlen: %d %x",
				ret_len, inbuffer[0], inbuffer[1], inLen, outLen, tobuffer, strlen(tobuffer), tobuffer[0]);
			iconv_close(cd);
			strcpy(pb, tobuffer);
			len += strlen(tobuffer);
			pb += strlen(tobuffer);
		}
	}
	else
	{
		if (c) {
			if (memcmp(pt, cpt, sizeof(*pt))) {
				if (len) {
					if (text_to_json(buf, len, pt, out) < 0)
						return -1;
					len = 0;
					pb  = buf;
				}

				*pt = *cpt;
			}

			uc = dtvcc_unicode(c);
			if (uc == 0)
				uc = 0x20;

			r = put_uc(pb, uc);


			pb  += r;
			len += r;
		}
		}

	}

	if (len) {
		if (text_to_json(buf, len, pt, out) < 0)
			return -1;
	}

	if (o_symbol(out, ']') < 0)
		return -1;
	if (o_prop_end(out) < 0)
		return -1;
	if (o_int_prop(out, "row_start", start) < 0)
		return -1;
	if (o_symbol(out, '}') < 0)
			return -1;

	return 0;
}

static int
dtvcc_window_to_json (struct tvcc_decoder *td, struct dtvcc_window *win, Output *out)
{
	struct dtvcc_pen_style  pt;
	char                   *str;
	int                     i;
	int                     column_count_no_lock = 0;
	int                     column_final_count;
	int                     row_count_no_lock = 0;
	int                     row_final_count;

	if (o_symbol(out, '{') < 0)
		return -1;

	if (o_int_prop(out, "anchor_point", win->anchor_point) < 0)
		return -1;

	if (o_int_prop(out, "anchor_horizontal", win->anchor_horizontal) < 0)
		return -1;

	if (o_int_prop(out, "anchor_vertical", win->anchor_vertical) < 0)
		return -1;

	if (o_bool_prop(out, "anchor_relative", win->anchor_relative) < 0)
		return -1;

	if (o_bool_prop(out, "row_lock", win->row_lock) < 0)
		return -1;

	if (o_bool_prop(out, "column_lock", win->column_lock) < 0)
		return -1;

	if (win->row_lock == 0)
	{
		row_count_no_lock = win->row_no_lock_length;
		row_final_count =
			(row_count_no_lock > win->row_count) ?
			row_count_no_lock : win->row_count;
	}
	else
		row_final_count = win->row_count;
	win->row_count = row_final_count;

	if (o_int_prop(out, "row_count", win->row_count) < 0)
		return -1;

	if (win->column_lock == 0)
	{
		column_count_no_lock = win->column_no_lock_length;
		column_final_count =
			(column_count_no_lock > win->column_count) ?
			column_count_no_lock : win->column_count;
	}
	else
		column_final_count = win->column_count;
	win->column_count = column_final_count;
	if (o_int_prop(out, "column_count", win->column_count) < 0)
		return -1;

	switch (win->style.justify) {
	case JUSTIFY_LEFT:
	default:
		str = "left";
		break;
	case JUSTIFY_RIGHT:
		str = "right";
		break;
	case JUSTIFY_CENTER:
		str = "center";
		break;
	case JUSTIFY_FULL:
		str = "full";
		break;
	}

	if (o_str_prop(out, "justify", str) < 0)
		return -1;

	if (o_str_prop(out, "print_direction", direction_to_str(win->style.print_direction)) < 0)
		return -1;

	if (o_str_prop(out, "scroll_direction", direction_to_str(win->style.scroll_direction)) < 0)
		return -1;

	if (o_bool_prop(out, "wordwrap", win->style.wordwrap) < 0)
		return -1;

	if (o_int_prop(out, "effect_percent", win->effect_percent) < 0)
		return -1;

	if (o_str_prop(out, "effect_status", effect_status_to_str(win->effect_status)) < 0)
		return -1;

	switch (win->style.display_effect) {
	case DISPLAY_EFFECT_SNAP:
	default:
		str = "snap";
		break;
	case DISPLAY_EFFECT_FADE:
		str = "fade";
		break;
	case DISPLAY_EFFECT_WIPE:
		str = "wipe";
		break;
	}

	if (o_str_prop(out, "display_effect", str) < 0)
		return -1;

	if (o_str_prop(out, "effect_direction", direction_to_str(win->style.effect_direction)) < 0)
		return -1;

	if (o_int_prop(out, "effect_speed", win->style.effect_speed) < 0)
		return -1;

	if (o_str_prop(out, "fill_opacity", opacity_to_str(win->style.fill_opacity)) < 0)
		return -1;

	if (o_int_prop(out, "fill_color", win->style.fill_color) < 0)
		return -1;

	if (o_str_prop(out, "border_type", edge_to_str(win->style.border_type)) < 0)
		return -1;

	if (o_int_prop(out, "border_color", win->style.border_color) < 0)
		return -1;

	if (o_int_prop(out, "heart_beat", out->flash) < 0)
		return -1;

	if (o_prop_begin(out, "rows") < 0)
		return -1;

	if (o_symbol(out, '[') < 0)
		return -1;

	memset(&pt, 0, sizeof(pt));
	for (i = 0; i < win->row_count; i ++) {
		if (row_to_json(td, win, out, i, &pt) < 0)
			return -1;
	}

	if (o_symbol(out, ']') < 0)
		return -1;

	if (o_symbol(out, '}') < 0)
		return -1;

	if (o_prop_end(out) < 0)
		return -1;

	return 0;
}

static int
dtvcc_service_to_json (struct tvcc_decoder *td, struct dtvcc_service *srv, Output *out)
{
	struct dtvcc_window *wins[8];
	int i, j, n;

	if (o_symbol(out, '{') < 0)
		return -1;

	if (o_str_prop(out, "type", "cea708") < 0)
		return -1;

	n = 0;
	for (i = 0; i < 8; i ++) {
		if (srv->window[i].visible) {
			wins[n ++] = &srv->window[i];
		}
	}

	//qsort(wins, n, sizeof(struct dtvcc_window*), cmp_win_prio);
	for (i = 0; i < n; i ++) {
		for (j = i + 1; j < n; j ++) {
			if (wins[i]->priority > wins[j]->priority) {
				struct dtvcc_window *tw;

				tw = wins[i];
				wins[i] = wins[j];
				wins[j] = tw;
			}
		}
	}

	if (o_prop_begin(out, "windows") < 0)
		return -1;

	if (o_symbol(out, '[') < 0)
		return -1;

	for (i = 0; i < n; i ++) {
		if (dtvcc_window_to_json(td, wins[i], out) < 0)
			return -1;
	}

	if (o_symbol(out, ']') < 0)
		return -1;

	if (o_symbol(out, '}') < 0)
		return -1;

	return 0;
}

int
tvcc_empty_json (int pgno, char *buf, size_t len)
{
	Output out;

	o_init(&out, buf, len);

	if (o_symbol(&out, '{') < 0)
		return -1;

	if (pgno <= 8) {
		if (o_str_prop(&out, "type", "cea608") < 0)
			return -1;
	} else {
		if (o_str_prop(&out, "type", "cea708") < 0)
			return -1;
	}

	if (o_symbol(&out, '}') < 0)
		return -1;

	return 0;
}

int
tvcc_to_json (struct tvcc_decoder *td, int pgno, char *buf, size_t len)
{
	Output out;
	int    r;

	o_init(&out, buf, len);

	if (pgno <= 8) {
		struct vbi_page pg;

		if (!vbi_fetch_cc_page(td->vbi, &pg, pgno, 1))
			return -1;

		r = vbi_page_to_json(&pg, &out);
	} else {
		struct dtvcc_service *ds = &td->dtvcc.service[pgno - 1 - 8];

		r = dtvcc_service_to_json(td, ds, &out);
	}

	return r;
}

