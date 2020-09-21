/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <am_misc.h>
#include <am_time.h>
#include <am_debug.h>
#include <am_cond.h>
#include "list.h"
#include <am_isdb.h>

#define isdb_log(...) __android_log_print(ANDROID_LOG_INFO, "ISDB" TAG_EXT, __VA_ARGS__)

#define RGBA(r,g,b,a) (((unsigned)(255 - (a)) << 24) | ((b) << 16) | ((g) << 8) | (r))

typedef uint32_t rgba;
static rgba Default_clut[128] =
{
	//0-7
	RGBA(0,0,0,255), RGBA(255,0,0,255), RGBA(0,255,0,255), RGBA(255,255,0,255),
	RGBA(0,0,255,255), RGBA(255,0,255,255), RGBA(0,255,255,255), RGBA(255,255,255,255),
	//8-15
	RGBA(0,0,0,0), RGBA(170,0,0,255), RGBA(0,170,0,255), RGBA(170,170,0,255),
	RGBA(0,0,170,255), RGBA(170,0,170,255), RGBA(0,170,170,255), RGBA(170,170,170,255),
	//16-23
	RGBA(0,0,85,255), RGBA(0,85,0,255), RGBA(0,85,85,255), RGBA(0,85,170,255),
	RGBA(0,85,255,255), RGBA(0,170,85,255), RGBA(0,170,255,255), RGBA(0,255,85,255),
	//24-31
	RGBA(0,255,170,255), RGBA(85,0,0,255), RGBA(85,0,85,255), RGBA(85,0,170,255),
	RGBA(85,0,255,255), RGBA(85,85,0,255), RGBA(85,85,85,255), RGBA(85,85,170,255),
	//32-39
	RGBA(85,85,255,255), RGBA(85,170,0,255), RGBA(85,170,85,255), RGBA(85,170,170,255),
	RGBA(85,170,255,255), RGBA(85,255,0,255), RGBA(85,255,85,255), RGBA(85,255,170,255),
	//40-47
	RGBA(85,255,255,255), RGBA(170,0,85,255), RGBA(170,0,255,255), RGBA(170,85,0,255),
	RGBA(170,85,85,255), RGBA(170,85,170,255), RGBA(170,85,255,255), RGBA(170,170,85,255),
	//48-55
	RGBA(170,170,255,255), RGBA(170,255,0,255), RGBA(170,255,85,255), RGBA(170,255,170,255),
	RGBA(170,255,255,255), RGBA(255,0,85,255), RGBA(255,0,170,255), RGBA(255,85,0,255),
	//56-63
	RGBA(255,85,85,255), RGBA(255,85,170,255), RGBA(255,85,255,255), RGBA(255,170,0,255),
	RGBA(255,170,85,255), RGBA(255,170,170,255), RGBA(255,170,255,255), RGBA(255,255,85,255),
	//64
	RGBA(255,255,170,255),
	// 65-127 are caliculated later.
};

struct cc_subtitle
{
	/**
	 * A generic data which contain data according to decoder
	 * @warn decoder cant output multiple types of data
	 */
	void *data;
	enum subdatatype datatype;

	/** number of data */
	unsigned int nb_data;

	/**  type of subtitle */
	enum subtype type;

	/** Encoding type of Text, must be ignored in case of subtype as bitmap or cc_screen*/
	enum ccx_encoding_type  enc_type;

	/* set only when all the data is to be displayed at same time */
	LLONG start_time;
	LLONG end_time;

	/* flags */
	int flags;

	/* index of language table */
	int lang_index;

	/** flag to tell that decoder has given output */
	int got_output;

	char mode[5];
	char info[4];

	struct cc_subtitle *next;
	struct cc_subtitle *prev;
};

typedef struct
{
	int nb_char;
	int nb_line;
	int need_update;
	uint64_t timestamp;
	uint64_t prev_timestamp;
	/**
	 * List of string for each row there
	 * TODO test with vertical string
	 */
	struct list_head text_list_head;
	/**
	 * Keep second list to confirm that string does not get repeated
	 * Used in No Rollup in Configuration and ISDB specs have string in
	 * rollup mode
	 * For Second Pass
	 */
	struct list_head buffered_text;
	ISDBSubState current_state; //modified default_state[lang_tag]
	enum isdb_tmd tmd;
	int nb_lang;
	struct
	{
		int hour;
		int min;
		int sec;
		int milli;
	}offset_time;
	uint8_t dmf;
	uint8_t dc;
	int cfg_no_rollup;
	char* json_buffer;
	AM_Isdb_UpdataJson_t json_update;
	void* user_data;
	int horizon_layout;
	int code_set_flag;
#define JSON_SIZE 4096
}ctx_t;

struct ISDBText
{
	char *buf;
	size_t len;
	size_t used;
	struct ISDBPos pos;
	size_t txt_tail; // tail of the text, excluding trailing control sequences.
	//Time Stamp when first string is recieved
	uint64_t timestamp;
	int refcount;
	struct list_head list;
};

typedef struct {
	struct cc_subtitle cc_subtitle;
	ctx_t ctx;
}AM_ISDB_Parser_t;

/*************************************************/
static int put_uc (uint8_t *buf, int uc)
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

static void o_init (Output *out, char *buf, size_t len)
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

static int o_printf (Output *out, const char *fmt, ...)
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

static inline int o_prop_begin (Output *out, const char *key)
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

static inline int o_prop_end (Output *out)
{
	out->has_prop = 1;

	return 0;
}

static int o_symbol (Output *out, char sym)
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

static int o_bool_prop (Output *out, const char *key, int val)
{
	if (o_prop_begin(out, key) < 0)
		return -1;

	if (o_printf(out, "%s", val ? "true" : "false") < 0)
		return -1;

	if (o_prop_end(out) < 0)
		return -1;

	return 0;
}

static int o_int_prop (Output *out, const char *key, int val)
{
	if (o_prop_begin(out, key) < 0)
		return -1;

	if (o_printf(out, "%d", val) < 0)
		return -1;

	if (o_prop_end(out) < 0)
		return -1;

	return 0;
}

static int o_str_prop (Output *out, const char *key, const char *val)
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

/***************************************************/
static int row_to_json(ctx_t *ctx, Output *out)
{
	struct ISDBText *text = NULL;
	int row_count = 0;
	char buf[8] = {0};


	list_for_each_entry(text, &ctx->text_list_head, list, struct ISDBText)
	{
		if (o_symbol(out, '{') < 0)
			return -1;
#if 0
		isdb_log("text: %s x: %d y: %d bgc:0x%x fgc:0x%x fsize:%d",
				text->buf,
				text->pos.x,
				text->pos.y,
				ctx->current_state.bg_color,
				ctx->current_state.fg_color,
				ctx->current_state.layout_state.font_size);
#endif
		if (o_str_prop(out, "text", text->buf) < 0)
			return -1;

		if (o_int_prop(out, "x", text->pos.x) < 0)
			return -1;

		if (o_int_prop(out, "y", text->pos.y) < 0)
			return -1;

		if (o_symbol(out, '}') < 0)
			return -1;
		row_count++;
	}

	return row_count;
}

static int isdb_empty_json (ctx_t* ctx)
{
	Output out;

	o_init(&out, ctx->json_buffer, JSON_SIZE);

	if (o_symbol(&out, '{') < 0)
		return -1;

	if (o_str_prop(&out, "type", "isdb") < 0)
		return -1;

	if (o_symbol(&out, '}') < 0)
		return -1;

	if (ctx->json_update)
		ctx->json_update(ctx->user_data);

	return 0;
}

static int isdb_json (ctx_t *ctx)
{
	Output out;
	int row_count = -1;
	if (ctx->text_list_head.next == &ctx->text_list_head)
	{
		isdb_empty_json(ctx);
		return 0;
	}

	o_init(&out, ctx->json_buffer, JSON_SIZE);

	if (o_symbol(&out, '{') < 0)
		return -1;

	if (o_str_prop(&out, "type", "isdb") < 0)
		return -1;

	if (o_int_prop(&out, "fsize", ctx->current_state.layout_state.font_size) < 0)
		return -1;

	if (o_int_prop(&out, "fscale", ctx->current_state.layout_state.font_scale.fscx*ctx->current_state.layout_state.font_scale.fscy) < 0)
		return -1;

	if (o_int_prop(&out, "fgcolor", ctx->current_state.fg_color) < 0)
		return -1;

	if (o_int_prop(&out, "bgcolor", ctx->current_state.bg_color) < 0)
		return -1;

	ISDBSubLayout *ls = &ctx->current_state.layout_state;
	if (o_int_prop(&out, "horizon_layout", IS_HORIZONTAL_LAYOUT(ls->format)) < 0)
		return -1;

	if (o_prop_begin(&out, "rows") < 0)
		return -1;

	if (o_symbol(&out, '[') < 0)
		return -1;

	row_count = row_to_json(ctx, &out);
	if (row_count <= 0)
		return -1;

	if (o_symbol(&out, ']') < 0)
		return -1;

	if (o_int_prop(&out, "row_count", row_count) < 0)
		return -1;

	if (o_symbol(&out, '}') < 0)
		return -1;

	if (ctx->json_update)
		ctx->json_update(ctx->user_data);
	return 0;
}

static void init_layout(ISDBSubLayout *ls)
{
	ls->font_size = 36;
	ls->display_area.x = 0;
	ls->display_area.y = 0;

	ls->font_scale.fscx = 100;
	ls->font_scale.fscy = 100;

}

static int get_csi_params(const uint8_t *q, unsigned int *p1, unsigned int *p2)
{
	const uint8_t *q_pivot = q;
	if (!p1)
		return -1;

	*p1 = 0;
	for (; *q >= 0x30 && *q <= 0x39; q++)
	{
		*p1 *= 10;
		*p1 += *q - 0x30;
	}
	if (*q != 0x20 && *q != 0x3B)
		return -1;
	q++;
	if (!p2)
		return q - q_pivot;
	*p2 = 0;
	for (; *q >= 0x30 && *q <= 0x39; q++)
	{
		*p2 *= 10;
		*p2 += *q - 0x30;
	}
	q++;
	return q - q_pivot;
}

static void set_writing_format(ctx_t *ctx, uint8_t *arg)
{
	ISDBSubLayout *ls = &ctx->current_state.layout_state;

	/* One param means its initialization */
	if ( *(arg+1) == 0x20)
	{
		ls->format = (arg[0] & 0x0F);
		return;
	}

	/* P1 I1 p2 I2 P31 ~ P3i I3 P41 ~ P4j I4 F */
	if ( *(arg + 1) == 0x3B)
	{
		ls->format = WF_HORIZONTAL_CUSTOM;
		arg += 2;
	}
	if ( *(arg + 1) == 0x3B)
	{
		switch (*arg & 0x0f) {
			case 0:
				//ctx->font_size = SMALL_FONT_SIZE;
				break;
			case 1:
				//ctx->font_size = MIDDLE_FONT_SIZE;
				break;
			case 2:
				//ctx->font_size = STANDARD_FONT_SIZE;
			default:
				break;
		}
		arg += 2;
	}
	/* P3 */
	isdb_log("character numbers in one line in decimal:");
	while (*arg != 0x3b && *arg != 0x20)
	{
		ctx->nb_char = *arg;
		printf(" %x",*arg & 0x0f);
		arg++;
	}
	if (*arg == 0x20)
		return;
	arg++;
	isdb_log("line numbers in decimal: ");
	while (*arg != 0x20)
	{
		ctx->nb_line = *arg;
		printf(" %x",*arg & 0x0f);
		arg++;
	}

	return;
}

static int parse_csi(ctx_t *ctx, const uint8_t *buf, int len)
{
	uint8_t arg[10] = {0};
	int i = 0;
	int ret = 0;
	unsigned int p1,p2;
	const uint8_t *buf_pivot = buf;
	ISDBSubState *state = &ctx->current_state;
	ISDBSubLayout *ls = &state->layout_state;

	//Copy buf in arg
	for (i = 0; *buf != 0x20; i++)
	{
		if (i >= (sizeof(arg))+ 1)
		{
			isdb_log("UnExpected CSI %d >= %d", sizeof(arg) + 1, i);
			break;
		}
		arg[i] = *buf;
		buf++;
	}
	/* ignore terminating 0x20 character */
	arg[i] = *buf++;

	switch (*buf) {
		/* Set Writing Format */
		case CSI_CMD_SWF:
			isdb_log("Command:CSI: SWF\n");
			set_writing_format(ctx, arg);
			break;
			/* Composite Character Composition */
		case CSI_CMD_CCC:
			isdb_log("Command:CSI: CCC\n");
			ret = get_csi_params(arg, &p1, NULL);
			if (ret > 0)
			{
				ls->ccc = p1;
			}
			break;
			/* Set Display Format */
		case CSI_CMD_SDF:
			ret = get_csi_params(arg, &p1, &p2);
			if (ret > 0)
			{
				ls->display_area.w = p1;
				ls->display_area.h = p2;
			}
			isdb_log("Command:CSI: SDF (w:%d, h:%d)\n", p1, p2);
			break;
			/* Character composition dot designation */
		case CSI_CMD_SSM:
			ret = get_csi_params(arg, &p1, &p2);
			if (ret > 0)
				ls->font_size = p1;
			isdb_log("Command:CSI: SSM (x:%d y:%d)\n", p1, p2);
			break;
			/* Set Display Position */
		case CSI_CMD_SDP:
			ret = get_csi_params(arg, &p1, &p2);
			if (ret > 0)
			{
				ls->display_area.x = p1;
				ls->display_area.y = p2;
			}
			isdb_log("Command:CSI: SDP (x:%d, y:%d)\n", p1, p2);
			break;
			/* Raster Colour command */
		case CSI_CMD_RCS:
			ret = get_csi_params(arg, &p1, NULL);
			if (ret > 0)
				ctx->current_state.raster_color = Default_clut[ctx->current_state.clut_high_idx << 4 | p1];
			isdb_log("Command:CSI: RCS (%d)\n", p1);
			break;
			/* Set Horizontal Spacing */
		case CSI_CMD_SHS:
			ret = get_csi_params(arg, &p1, NULL);
			if (ret > 0)
				ls->cell_spacing.col = p1;
			isdb_log("Command:CSI: SHS (%d)\n", p1);
			break;
			/* Set Vertical Spacing */
		case CSI_CMD_SVS:
			ret = get_csi_params(arg, &p1, NULL);
			if (ret > 0)
				ls->cell_spacing.row = p1;
			isdb_log("Command:CSI: SVS (%d)\n", p1);
			break;
			/* Active Coordinate Position Set */
		case CSI_CMD_ACPS:
			isdb_log("Command:CSI: ACPS\n");
			ret = get_csi_params(arg, &p1, &p2);
			if (ret > 0)
				ls->acps[0] = p1;
			ls->acps[1] = p1;
			break;
		default:
			isdb_log("Command:CSI: Unknown command 0x%x\n", *buf);
			break;
	}
	buf++;
	/* ACtual CSI command */
	return buf - buf_pivot;
}

static void move_penpos(ctx_t *ctx, int col, int row)
{
	ISDBSubLayout *ls = &ctx->current_state.layout_state;

	ls->cursor_pos.x = row;
	ls->cursor_pos.y = col;
}


static void set_position(ctx_t *ctx, unsigned int p1, unsigned int p2)
{
	ISDBSubLayout *ls = &ctx->current_state.layout_state;
	int cw, ch;
	int col, row;


	if (IS_HORIZONTAL_LAYOUT(ls->format))
	{
		cw = (ls->font_size + ls->cell_spacing.col) * ls->font_scale.fscx / 100;
		ch = (ls->font_size + ls->cell_spacing.row) * ls->font_scale.fscy / 100;
		// pen position is at bottom left
		col = p2 * cw;
		row = p1 * ch + ch;
	}
	else
	{
		cw = (ls->font_size + ls->cell_spacing.col) * ls->font_scale.fscy / 100;
		ch = (ls->font_size + ls->cell_spacing.row) * ls->font_scale.fscx / 100;
		// pen position is at upper center,
		// but in -90deg rotated coordinates, it is at middle left.
		col = p2 * cw;
		row = p1 * ch + ch / 2;
	}
	move_penpos(ctx, col, row);
}

static int ccx_strstr_ignorespace(const unsigned char *str1, const unsigned char *str2)
{
	int i;
	for ( i = 0; str2[i] != '\0'; i++)
	{
		if (isspace(str2[i]))
			continue;
		if (str2[i] != str1[i])
			return 0;
	}
	return 1;
}

static struct ISDBText *allocate_text_node(ISDBSubLayout *ls)
{
	struct ISDBText *text = NULL;

	text = malloc(sizeof(struct ISDBText));
	if (!text)
		return NULL;

	text->used = 0;
	text->buf = malloc(128);
	text->len = 128;
	*text->buf = 0;
	return text;
}

static int add_cc_sub_text(struct cc_subtitle *sub, char *str, LLONG start_time,
		LLONG end_time, char *info, char *mode, enum ccx_encoding_type e_type)
{
	if (str == NULL || strlen(str) == 0)
		return 0;
	if (sub->nb_data)
	{
		for (;sub->next;sub = sub->next);
		sub->next = malloc(sizeof(struct cc_subtitle));
		if (!sub->next)
			return -1;
		sub->next->prev = sub;
		sub = sub->next;
	}

	sub->type = CC_TEXT;
	sub->enc_type = e_type;
	sub->data = strdup(str);
	isdb_log("text: %s", str);
	sub->datatype = CC_DATATYPE_GENERIC;
	sub->nb_data = str? strlen(str): 0;
	sub->start_time = start_time;
	sub->end_time = end_time;
	if (info)
		strncpy(sub->info, info, 4);
	if (mode)
		strncpy(sub->mode, mode, 4);
	sub->got_output = 1;
	sub->next = NULL;

	return 0;
}

static int parse_command(ctx_t *ctx, const uint8_t *buf, int len)
{
	const uint8_t *buf_pivot = buf;
	uint8_t code_lo = *buf & 0x0f;
	uint8_t code_hi = (*buf & 0xf0) >> 4;
	int ret;
	ISDBSubState *state = &ctx->current_state;
	ISDBSubLayout *ls = &state->layout_state;

	buf++;
	if ( code_hi == 0x00)
	{
		switch (code_lo) {
			/* NUL Control code, which can be added or deleted without effecting to
			   information content. */
			case 0x0:
				isdb_log("Command: NUL\n");
				break;

				/* BEL Control code used when calling attention (alarm or signal) */
			case 0x7:
				//TODO add bell character here
				isdb_log("Command: BEL\n");
				break;

				/**
				 *  APB: Active position goes backward along character path in the length of
				 *	character path of character field. When the reference point of the character
				 *	field exceeds the edge of display area by this movement, move in the
				 *	opposite side of the display area along the character path of the active
				 * 	position, for active position up.
				 */
			case 0x8:
				isdb_log("Command: ABP\n");
				break;

				/**
				 *  APF: Active position goes forward along character path in the length of
				 *	character path of character field. When the reference point of the character
				 *	field exceeds the edge of display area by this movement, move in the
				 * 	opposite side of the display area along the character path of the active
				 *	position, for active position down.
				 */
			case 0x9:
				isdb_log("Command: APF\n");
				break;

				/**
				 *  APD: Moves to next line along line direction in the length of line direction of
				 *	the character field. When the reference point of the character field exceeds
				 *	the edge of display area by this movement, move to the first line of the
				 *	display area along the line direction.
				 */
			case 0xA:
				isdb_log("Command: APD\n");
				break;

				/**
				 * APU: Moves to the previous line along line direction in the length of line
				 *	direction of the character field. When the reference point of the character
				 *	field exceeds the edge of display area by this movement, move to the last
				 *	line of the display area along the line direction.
				 */
			case 0xB:
				isdb_log("Command: APU\n");
				break;

				/**
				 * CS: Display area of the display screen is erased.
				 * Specs does not say clearly about whether we have to clear cursor
				 * Need Samples to see whether CS is called after pen move or before it
				 */
			case 0xC:
				isdb_log("Command: CS clear Screen\n");
				struct list_head *target = NULL;
				struct list_head *to_free = NULL;

				target = ctx->text_list_head.next;
				while (target != &ctx->text_list_head)
				{
					to_free = target;
					target = target->next;
					free(to_free);
				}
				INIT_LIST_HEAD(&ctx->text_list_head);
				ctx->need_update = 1;
				//isdb_empty_json(ctx);
				break;

				/** APR: Active position down is made, moving to the first position of the same
				 *	line.
				 */
			case 0xD:
				isdb_log("Command: APR\n");
				break;

				/* LS1: Code to invoke character code set. */
			case 0xE:
				isdb_log("Command: LS1\n");
				break;

				/* LS0: Code to invoke character code set. */
			case 0xF:
				isdb_log("Command: LS0\n");
				break;
				/* Verify the new version of specs or packet is corrupted */
			default:
				isdb_log("Command: Unknown\n");
				break;
		}
	}
	else if (code_hi == 0x01)
	{
		switch (code_lo) {
			/**
			 * PAPF: Active position forward is made in specified times by parameter P1 (1byte).
			 *	Parameter P1 shall be within the range of 04/0 to 07/15 and time shall be
			 *	specified within the range of 0 to 63 in binary value of 6-bit from b6 to b1.
			 *	(b8 and b7 are not used.)
			 */
			case 0x6:
				isdb_log("Command: PAPF\n");
				break;

				/**
				 * CAN: From the current active position to the end of the line is covered with
				 *	background colour in the width of line direction in the current character
				 *	field. Active position is not moved.
				 */
			case 0x8:
				isdb_log("Command: CAN\n");
				break;

				/* SS2: Code to invoke character code set. */
			case 0x9:
				isdb_log("Command: SS2\n");
				break;

				/* ESC:Code for code extension. */
			case 0xB:
				isdb_log("Command: ESC\n");
				break;

				/** APS: Specified times of active position down is made by P1 (1 byte) of the first
				 *	parameter in line direction length of character field from the first position
				 *	of the first line of the display area. Then specified times of active position
				 *	forward is made by the second parameter P2 (1 byte) in the character path
				 *	length of character field. Each parameter shall be within the range of 04/0
				 *	to 07/15 and specify time within the range of 0 to 63 in binary value of 6-
				 *	bit from b6 to b1. (b8 and b7 are not used.)
				 */
			case 0xC:
				isdb_log("Command: APS\n");
				set_position(ctx, *buf & 0x3F, *(buf+1) & 0x3F);
				buf += 2;
				break;

				/* SS3: Code to invoke character code set. */
			case 0xD:
				isdb_log("Command: SS3\n");
				ctx->code_set_flag = 3;
				break;

				/**
				 * RS: It is information division code and declares identification and
				 * 	introduction of data header.
				 */
			case 0xE:
				isdb_log("Command: RS\n");
				break;

				/**
				 * US: It is information division code and declares identification and
				 *	introduction of data unit.
				 */
			case 0xF:
				isdb_log("Command: US\n");
				break;

				/* Verify the new version of specs or packet is corrupted */
			default:
				isdb_log("Command: Unknown\n");
				break;
		}
	}
	else if (code_hi == 0x8)
	{
		switch (code_lo) {
			/* BKF */
			case 0x0:
				/* RDF */
			case 0x1:
				/* GRF */
			case 0x2:
				/* YLF */
			case 0x3:
				/* BLF 	*/
			case 0x4:
				/* MGF */
			case 0x5:
				/* CNF */
			case 0x6:
				/* WHF */
			case 0x7:
				isdb_log("Command: Forground color (0x%X)\n", Default_clut[code_lo]);
				state->fg_color = Default_clut[code_lo];
				break;

				/* SSZ */
			case 0x8:
				isdb_log("Command: SSZ\n");
				ls->font_scale.fscx = 50;
				ls->font_scale.fscy = 50;
				break;

				/* MSZ */
			case 0x9:
				ls->font_scale.fscx = 200;
				ls->font_scale.fscy = 200;
				isdb_log("Command: MSZ\n");
				break;

				/* NSZ */
			case 0xA:
				ls->font_scale.fscx = 100;
				ls->font_scale.fscy = 100;
				isdb_log("Command: NSZ\n");
				break;

				/* SZX */
			case 0xB:
				isdb_log("Command: SZX\n");
				buf++;
				break;

				/* Verify the new version of specs or packet is corrupted */
			default:
				isdb_log("Command: Unknown\n");
				break;

		}
	}
	else if ( code_hi == 0x9)
	{
		switch (code_lo) {
			/* COL */
			case 0x0:
				/* Pallete Col */
				if (*buf == 0x20)
				{
					isdb_log("Command: COL: Set Clut %d\n",(buf[0] & 0x0F));
					buf++;
					ctx->current_state.clut_high_idx = (buf[0] & 0x0F);
				}
				else if ((*buf & 0XF0) == 0x40)
				{
					isdb_log("Command: COL: Set Forground 0x%08X\n", Default_clut[*buf & 0x0F]);
					ctx->current_state.fg_color = Default_clut[*buf & 0x0F];
				}
				else if ((*buf & 0XF0) == 0x50)
				{
					isdb_log("Command: COL: Set Background 0x%08X\n", Default_clut[*buf & 0x0F]);
					ctx->current_state.bg_color = Default_clut[*buf & 0x0F];
				}
				else if ((*buf & 0XF0) == 0x60)
				{
					isdb_log("Command: COL: Set half Forground 0x%08X\n", Default_clut[*buf & 0x0F]);
					ctx->current_state.hfg_color = Default_clut[*buf & 0x0F];
				}
				else if ((*buf & 0XF0) == 0x70)
				{
					isdb_log("Command: COL: Set Half Background 0x%8X\n", Default_clut[*buf & 0x0F]);
					ctx->current_state.hbg_color = Default_clut[*buf & 0x0F];
				}

				buf++;
				break;

				/* FLC */
			case 0x1:
				isdb_log("Command: FLC\n");
				buf++;
				break;

				/* CDC */
			case 0x2:
				isdb_log("Command: CDC\n");
				buf++;
				buf++;
				buf++;
				break;

				/* POL */
			case 0x3:
				isdb_log("Command: POL\n");
				buf++;
				break;

				/* WMM */
			case 0x4:
				isdb_log("Command: WMM\n");
				buf++;
				buf++;
				buf++;
				break;

				/* MACRO */
			case 0x5:
				isdb_log("Command: MACRO\n");
				buf++;
				break;

				/* HLC */
			case 0x7:
				isdb_log("Command: HLC\n");
				buf++;
				break;

				/* RPC */
			case 0x8:
				isdb_log("Command: RPC\n");
				buf++;
				break;

				/* SPL */
			case 0x9:
				isdb_log("Command: SPL\n");
				break;

				/* STL */
			case 0xA:
				isdb_log("Command: STL\n");
				break;

				/* CSI Code for code system extension indicated*/
			case 0xB:
				ret = parse_csi(ctx, buf, len);
				buf += ret;
				break;

				/* TIME */
			case 0xD:
				isdb_log("Command: TIME\n");
				buf++;
				buf++;
				break;

				/* Verify the new version of specs or packet is corrupted */
			default:
				isdb_log("Command: Unknown\n");
				break;
		}
	}

	return buf - buf_pivot;

}

static int reserve_buf(struct ISDBText *text, size_t len)
{
	size_t blen;
	unsigned char *ptr;

	if (text->len >= text->used + len)
		return AM_SUCCESS;

	// Allocate always in multiple of 128
	blen = ((text->used + len + 127) >> 7) << 7;
	ptr = realloc(text->buf, blen);
	if (!ptr)
	{
		isdb_log("ISDB: out of memory for ctx->text.buf\n");
		return AM_FAILURE;
	}
	text->buf = ptr;
	text->len = blen;
	isdb_log ("expanded ctx->text(%lu)\n", blen);
	return AM_SUCCESS;
}

static int append_char(ctx_t *ctx, const char ch)
{
	ISDBSubLayout *ls = &ctx->current_state.layout_state;
	struct ISDBText *text = NULL;
	// Current Line Position
	int cur_lpos;
	//Space taken by character
	int csp;

	if (IS_HORIZONTAL_LAYOUT(ls->format))
	{
		cur_lpos = ls->cursor_pos.x;
		csp = ls->font_size * ls->font_scale.fscx / 100;;
	}
	else
	{
		cur_lpos = ls->cursor_pos.y;
		csp = ls->font_size * ls->font_scale.fscy / 100;;
	}

	list_for_each_entry(text, &ctx->text_list_head, list, struct ISDBText)
	{
		//Text Line Position
		int text_lpos;
		if (IS_HORIZONTAL_LAYOUT(ls->format))
			text_lpos = text->pos.x;
		else
			text_lpos = text->pos.y;

		if (text_lpos == cur_lpos)
		{
			break;
		}
		else if (text_lpos > cur_lpos)
		{
			struct ISDBText *text1 = NULL;
			//Allocate Text here so that list is always sorted
			text1 = allocate_text_node(ls);
			text1->pos.x = ls->cursor_pos.x;
			text1->pos.y = ls->cursor_pos.y;
			list_add(&text1->list, text->list.prev);
			text = text1;
			break;
		}
	}
	if (&text->list == &ctx->text_list_head)
	{
		text = allocate_text_node(ls);
		text->pos.x = ls->cursor_pos.x;
		text->pos.y = ls->cursor_pos.y;
		list_add_tail(&text->list, &ctx->text_list_head);
	}

	//Check position of character and if moving backword then clean text
	if (IS_HORIZONTAL_LAYOUT(ls->format))
	{
		if (ls->cursor_pos.y < text->pos.y)
		{
			text->pos.y = ls->cursor_pos.y;
			text->used = 0;
		}
		ls->cursor_pos.y += csp;
		text->pos.y += csp;
	}
	else
	{
		if (ls->cursor_pos.y < text->pos.y)
		{
			text->pos.y = ls->cursor_pos.y;
			text->used = 0;
		}
		ls->cursor_pos.x += csp;
		text->pos.x += csp;
	}

	reserve_buf(text, 2); //+1 for terminating '\0'
	if (ctx->code_set_flag == 3 && ch == 0x21)
		text->buf[text->used] = 0xEF;
	else
		text->buf[text->used] = ch;
	ctx->code_set_flag = 0;
	text->used ++;
	text->buf[text->used] = '\0';

	ctx->need_update = 1;

	return 1;
}

static int screen_to_json(ctx_t *ctx)
{
	struct ISDBText *text = NULL;

	list_for_each_entry(text, &ctx->text_list_head, list, struct ISDBText)
	{
		isdb_log("text: %s x: %d y: %d bgc:0x%x fgc:0x%x fsize:%d",
				text->buf,
				text->pos.x,
				text->pos.y,
				ctx->current_state.bg_color,
				ctx->current_state.fg_color,
				ctx->current_state.layout_state.font_size);
	}
	return 0;
}

static int get_text(ctx_t *ctx, unsigned char *buffer, int len)
{
	ISDBSubLayout *ls = &ctx->current_state.layout_state;
	struct ISDBText *text = NULL;
	struct ISDBText *sb_text = NULL;
	struct ISDBText *sb_temp = NULL;
	struct ISDBText *wtrepeat_text = NULL;
	//TO keep track we dont over flow in buffer from user
	int index = 0;

	if (ctx->cfg_no_rollup || (ctx->cfg_no_rollup == ctx->current_state.rollup_mode))
		// Abhinav95: Forcing -noru to perform deduplication even if stream doesn't honor it
	{
		wtrepeat_text = NULL;
		if (list_empty(&ctx->buffered_text))
		{
			list_for_each_entry(text, &ctx->text_list_head, list, struct ISDBText)
			{
				sb_text = allocate_text_node(ls);
				list_add_tail(&sb_text->list, &ctx->buffered_text);
				reserve_buf(sb_text, text->used);
				memcpy(sb_text->buf, text->buf, text->used);
				sb_text->buf[text->used] = '\0';
				sb_text->used = text->used;
			}
			//Dont do anything else, other then copy buffer in start state
			return 0;
		}

		//Update Secondary Buffer for new entry in primary buffer
		list_for_each_entry(text, &ctx->text_list_head, list, struct ISDBText)
		{
			int found = AM_FAILURE;
			list_for_each_entry(sb_text, &ctx->buffered_text, list, struct ISDBText)
			{
				if (ccx_strstr_ignorespace(text->buf, sb_text->buf))
				{
					found = AM_SUCCESS;
					//See if complete string is there if not update that string
					if (!ccx_strstr_ignorespace(sb_text->buf, text->buf))
					{
						reserve_buf(sb_text, text->used);
						memcpy(sb_text->buf, text->buf, text->used);
					}
					break;
				}
			}
			if (found == AM_FAILURE)
			{
				sb_text = allocate_text_node(ls);
				list_add_tail(&sb_text->list, &ctx->buffered_text);
				reserve_buf(sb_text, text->used);
				memcpy(sb_text->buf, text->buf, text->used);
				sb_text->buf[text->used] = '\0';
				sb_text->used = text->used;
			}
		}

		//Flush Secondary Buffer if text not in primary buffer
		list_for_each_entry_safe(sb_text, sb_temp, &ctx->buffered_text, list, struct ISDBText)
		{
			int found = AM_FAILURE;
			list_for_each_entry(text, &ctx->text_list_head, list, struct ISDBText)
			{
				if (ccx_strstr_ignorespace(text->buf, sb_text->buf))
				{
					found = AM_SUCCESS;
					break;
				}
			}
			if (found == AM_FAILURE)
			{
				// Write that buffer in file
				if (len - index > sb_text->used + 2 && sb_text->used  > 0)
				{
					memcpy(buffer+index, sb_text->buf, sb_text->used);
					buffer[sb_text->used+index] = '\n';
					buffer[sb_text->used+index+1] = '\r';
					index += sb_text->used + 2;
					sb_text->used = 0;
					list_del(&sb_text->list);
					free(sb_text->buf);
					free(sb_text);
					//delete entry from SB here
				}
			}
		}
	}
	else
	{
		list_for_each_entry(text, &ctx->text_list_head, list, struct ISDBText)
		{
			if (len - index > text->used + 2 && text->used  > 0)
			{
				memcpy(buffer+index, text->buf, text->used);
				buffer[text->used+index] = '\n';
				buffer[text->used+index+1] = '\r';
				index += text->used + 2;
				text->used = 0;
				text->buf[0] = '\0';
			}
		}
	}
	return index;
}


static int parse_statement(void* handle, const uint8_t *buf, int size)
{
	const uint8_t *buf_pivot = buf;
	int ret;
	int left;
	AM_ISDB_Parser_t *parser = handle;
	ctx_t *ctx = &parser->ctx;

	while ((left = (buf - buf_pivot)) < size)
	{
		unsigned char code    = (*buf & 0xf0) >> 4;
		unsigned char code_lo = *buf & 0x0f;
		if (code <= 0x1)
			ret = parse_command(ctx, buf, size);
		/* Special case *1(SP) */
		else if ( code == 0x2 && code_lo == 0x0 )
			ret = append_char(ctx,buf[0]);
		/* Special case *3(DEL) */
		else if ( code == 0x7 && code_lo == 0xF )
			/*TODO DEL should have block in fg color */
			ret = append_char(ctx, buf[0]);
		else if ( code <= 0x7)
			ret = append_char(ctx, buf[0]);
		else if ( code <= 0x9)
			ret = parse_command(ctx, buf, size);
		/* Special case *2(10/0) */
		else if ( code == 0xA && code_lo == 0x0 )
			/*TODO handle */;
		/* Special case *4(15/15) */
		else if (code == 0x0F && code_lo == 0XF )
			/*TODO handle */;
		else
			ret = append_char(ctx, buf[0]);
		if (ret < 0)
			break;
		buf += ret;
	}
	return 0;
}


static int parse_data_unit(void* handle,const uint8_t *buf, int size)
{
	int unit_parameter;
	int len;
	AM_ISDB_Parser_t *parser = handle;
	ctx_t *ctx = &parser->ctx;

	/* unit seprator */
	buf++;

	unit_parameter = *buf++;
	len = RB24(buf);

	buf += 3;
	switch (unit_parameter)
	{
		/* statement body */
		case 0x20:
			parse_statement(handle, buf, len);
	}
	return 0;
}

static int parse_caption_statement_data(void *handle, int lang_id, const uint8_t *buf, int size, struct cc_subtitle *sub)
{
	int tmd;
	int len;
	int ret;
	unsigned char buffer[1024] = "";

	AM_ISDB_Parser_t *parser = handle;
	ctx_t *ctx = &parser->ctx;

	tmd = *buf >> 6;
	buf++;
	/* skip timing data */
	if (tmd == 1 || tmd == 2)
	{
		buf += 5;
	}

	len = RB24(buf);

	buf += 3;
	ret = parse_data_unit(handle, buf, len);
	if ( ret < 0)
		return -1;
#if 0

	ret = get_text(ctx, buffer, 1024);
	/* Copy data if there in buffer */
	if (ret < 0 )
		return AM_SUCCESS;

	if (ret > 0)
	{
		add_cc_sub_text(sub, buffer, ctx->prev_timestamp, ctx->timestamp, "NA", "ISDB", CCX_ENC_UTF_8);
		if (sub->start_time == sub->end_time)
			sub->end_time += 2;
		ctx->prev_timestamp = ctx->timestamp;
	}
#endif
	return 0;
}

static int parse_caption_management_data(void *handle, const uint8_t *buf, int size)
{
	const uint8_t *buf_pivot = buf;
	int i;
	AM_ISDB_Parser_t *parser = handle;
	ctx_t *ctx = &parser->ctx;

	ctx->tmd = (*buf >> 6);
	isdb_log("CC MGMT DATA: TMD: %d\n", ctx->tmd);
	buf++;
	if (ctx->tmd == ISDB_TMD_FREE)
	{
		isdb_log("Playback time is not restricted to synchronize to the clock.\n");
	}
	else if (ctx->tmd == ISDB_TMD_OFFSET_TIME)
	{
		/**
		 * This 36-bit field indicates offset time to add to the playback time when the
		 * clock control mode is in offset time mode. Offset time is coded in the
		 * order of hour, minute, second and millisecond, using nine 4-bit binary
		 * coded decimals (BCD).
		 *
		 * +-----------+-----------+---------+--------------+
		 * |  hour     |   minute  |   sec   |  millisecond |
		 * +-----------+-----------+---------+--------------+
		 * |  2 (4bit) | 2 (4bit)  | 2 (4bit)|    3 (4bit)  |
		 * +-----------+-----------+---------+--------------+
		 */
		ctx->offset_time.hour = ((*buf>>4) * 10) + (*buf&0xf);
		buf++;
		ctx->offset_time.min = ((*buf>>4) * 10) + (*buf&0xf);
		buf++;
		ctx->offset_time.sec = ((*buf>>4) * 10) + (*buf&0xf);
		buf++;
		ctx->offset_time.milli = ((*buf>>4) * 100) + ( (*buf&0xf) * 10) + (buf[1]&0xf);
		buf += 2;
		isdb_log("CC MGMT DATA: OTD( h:%d m:%d s:%d millis: %d\n",
				ctx->offset_time.hour, ctx->offset_time.min,
				ctx->offset_time.sec, ctx->offset_time.milli);
	}
	else
	{
		isdb_log("Playback time is in accordance with the time of the clock,"
				"which is calibrated by clock signal (TDT). Playback time is"
				"given by PTS.\n");
	}
	ctx->nb_lang = *buf;
	isdb_log("CC MGMT DATA: nb languages: %d\n", ctx->nb_lang);
	buf++;

	for (i = 0; i < ctx->nb_lang; i++)
	{
		isdb_log("CC MGMT DATA: %d\n", (*buf&0x1F) >> 5);
		ctx->dmf = *buf&0x0F;
		isdb_log("CC MGMT DATA: DMF 0x%X\n", ctx->dmf);
		buf++;
		if (ctx->dmf == 0xC || ctx->dmf == 0xD|| ctx->dmf == 0xE)
		{
			ctx->dc = *buf;
			if (ctx->dc == 0x00)
				isdb_log("Attenuation Due to Rain\n");
		}
		isdb_log("CC MGMT DATA: languages: %c%c%c\n", buf[0], buf[1], buf[2]);
		buf += 3;
		isdb_log("CC MGMT DATA: Format: 0x%X\n", *buf>>4);
		/* 8bit code is not used by Brazilian ARIB they use utf-8*/
		isdb_log("CC MGMT DATA: TCS: 0x%X\n", (*buf>>2)&0x3);
		ctx->current_state.rollup_mode = !!(*buf&0x3);
		isdb_log("CC MGMT DATA: Rollup mode: 0x%X\n", ctx->current_state.rollup_mode);
	}
	return buf - buf_pivot;
}


static int isdb_parse_data_group(void *handle,const uint8_t *buf, struct cc_subtitle *sub)
{
	AM_ISDB_Parser_t *parser = handle;
	ctx_t *ctx = &parser->ctx;

	const uint8_t *buf_pivot = buf;
	int id = (*buf >> 2);
	int version = (*buf & 2);
	int link_number = 0;
	int last_link_number = 0;
	int group_size = 0;
	int ret = 0;

	if ( (id >> 4) == 0 )
	{
		isdb_log("ISDB group A\n");
	}
	else if ( (id >> 4) == 0 )
	{
		isdb_log("ISDB group B\n");
	}

	isdb_log("ISDB (Data group) version %d\n",version);

	buf++;
	link_number = *buf++;
	last_link_number = *buf++;
	isdb_log("ISDB (Data group) link_number %d last_link_number %d\n", link_number, last_link_number);

	group_size = RB16(buf);
	buf += 2;
	isdb_log("ISDB (Data group) group_size %d\n", group_size);

	if (ctx->prev_timestamp > ctx->timestamp)
	{
		ctx->prev_timestamp = ctx->timestamp;
	}
	if ((id & 0x0F) == 0)
	{
		/* Its Caption management */
		isdb_log("ISDB %d management");
		ret = parse_caption_management_data(parser, buf, group_size);
	}
	else if ((id & 0x0F) < 8 )
	{
		/* Caption statement data */
		isdb_log("ISDB language");
		ret = parse_caption_statement_data(parser, id & 0x0F, buf, group_size, sub);
	}
	else
	{
		/* This is not allowed in spec */
	}
	if (ret < 0)
		return -1;
	buf += group_size;

	//TODO check CRC
	buf += 2;

	return buf - buf_pivot;
}


/********************************/
AM_ErrorCode_t AM_ISDB_Create(AM_Isdb_CreatePara_t* para, AM_ISDB_Handle_t* handle)
{
	AM_ISDB_Parser_t *parser = NULL;
	ctx_t *ctx;
	parser = (AM_ISDB_Parser_t*)malloc(sizeof(AM_ISDB_Parser_t));

	if (!parser)
		return AM_FAILURE;

	memset(parser, 0, sizeof(AM_ISDB_Parser_t));
	ctx = &parser->ctx;

	INIT_LIST_HEAD(&ctx->text_list_head);
	INIT_LIST_HEAD(&ctx->buffered_text);
	ctx->prev_timestamp = UINT_MAX;

	ctx->current_state.clut_high_idx = 0;
	ctx->current_state.rollup_mode = 0;
	ctx->json_buffer = para->json_buffer;
	ctx->json_update = para->json_update;
	ctx->user_data = para->user_data;

	if (!ctx->json_buffer)
		return AM_FAILURE;

	init_layout(&ctx->current_state.layout_state);

	*handle = parser;
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_ISDB_Start(AM_Isdb_StartPara_t *para, AM_ISDB_Handle_t handle)
{
	AM_ISDB_Parser_t *parser = handle;
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_ISDB_Stop(AM_ISDB_Handle_t handle)
{
	AM_ISDB_Parser_t *parser = handle;

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_ISDB_Decode(AM_ISDB_Handle_t handle, uint8_t *buf, int size)
{
	int i, pes_packet_length, pes_header_length, scrambling_control, pts_dts_flag;
	AM_ISDB_Parser_t *parser = (AM_ISDB_Parser_t*)handle;
	const uint8_t *header_end = NULL;

	parser->ctx.need_update = 0;
	if (*buf++ != 0x80)
	{
		isdb_log("\nNot a Syncronised PES\n");
		return -1;
	}
	/* private data stream = 0xFF */
	buf++;
	header_end = buf + (*buf & 0x0f);

	buf++;
	while (buf < header_end)
	{
		/* TODO find in spec what is header */
		buf++;
	}
	isdb_parse_data_group(handle, buf, &parser->cc_subtitle);
	if (parser->ctx.need_update)
	{
		isdb_json(&parser->ctx);
	}
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_ISDB_Destroy(AM_ISDB_Handle_t *handle)
{
	free(handle);
	*handle = NULL;
	return AM_SUCCESS;
}


