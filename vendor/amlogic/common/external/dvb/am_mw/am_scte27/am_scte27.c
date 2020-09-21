/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include "list.h"
#include "semaphore.h"
#include "math.h"
#include <am_misc.h>
#include <am_time.h>
#include <am_debug.h>
#include <am_cond.h>
#include <am_scte27.h>

#define scte_log(...) __android_log_print(ANDROID_LOG_INFO, "SCTE" TAG_EXT, __VA_ARGS__)
#define bmp_log(...) __android_log_print(ANDROID_LOG_INFO, "SCTE_BMP" TAG_EXT, __VA_ARGS__)
#define dump_log(...) __android_log_print(ANDROID_LOG_INFO, "SCTE_DUMP" TAG_EXT, __VA_ARGS__)

#define DISPLAY_STD_720_480_30       0
#define DISPLAY_STD_720_576_25       1
#define DISPLAY_STD_1280_720_60      2
#define DISPLAY_STD_1920_1080_60     3
#define DISPLAY_STD_RESERVE         -1

#define SUB_TYPE_SIMPLE_BITMAP       1

#define SUB_NODE_NOT_DISPLAYING      0
#define SUB_NODE_DISPLAYING          1

#define PTS_PER_SEC                  90000

#define BMP_OUTLINE_NONE             0
#define BMP_OUTLINE_OUTLINE          1
#define BMP_OUTLINE_DROPSHADOW       2
#define BMP_OUTLINE_RESERVED         3

#define PIXEL_BG                     0 //Background
#define PIXEL_CH                     1 //Character
#define PIXEL_OL                     2 //Outline
#define PIXEL_DS                     3 //Dropshadow

#define OUTLINE_STYLE_NONE           0
#define OUTLINE_STYLE_OL             1
#define OUTLINE_STYLE_DS             2

#define DISPLAY_STATUS_INIT 0
#define DISPLAY_STATUS_SHOW 1

typedef struct scte_simple_bitmap_s
{
	uint8_t     show_status;
	uint8_t     bg_style;
	uint8_t     outline_style;
	uint8_t     pre_clear;
	uint8_t     immediate;
	uint32_t    character_color;
	uint32_t    character_color_rgba;
	uint32_t    frame_top_h;
	uint32_t    frame_top_v;
	uint32_t    frame_bottom_h;
	uint32_t    frame_bottom_v;
	struct bg_style_s
	{
		uint32_t top_h;
		uint32_t top_v;
		uint32_t bottom_h;
		uint32_t bottom_v;
		uint32_t frame_color;
		uint32_t frame_color_rgba;
	} bg_style_para;
	union{
		struct outlined_s
		{
			uint8_t    outline_thickness;
			uint32_t   outline_color;
			uint32_t   outline_color_rgba;
		} outlined;
		struct drop_shadow_s
		{
			uint8_t    shadow_right;
			uint8_t    shadow_bottom;
			uint32_t   shadow_color;
			uint32_t   shadow_color_rgba;
		} drop_shadow;
	} style_para;
	uint32_t   bitmap_length;
	uint32_t   reveal_pts;
	uint32_t   disapear_pts;
	void*      sub_bmp;
	struct list_head list;
}scte_simple_bitmap_t;

typedef struct scte_subtitle_s
{
	char                         lang[4];
	uint8_t                      display_status;
	uint8_t                      pre_clear;
	uint8_t                      immediate;
	uint8_t                      display_std;
	uint8_t                      sub_type;
	uint32_t                     pts;
	uint32_t                     duration;
	int                        block_len;
	uint32_t                     msg_crc;
	void*                        descriptor; //Not used

	scte_simple_bitmap_t         bitmap_container;
}scte_subtitle_t;

typedef struct scte_segment_s
{
	int                          table_ext;
	int                          curr_no;
	int                          last_no;
	int                          seg_size;
	int has_segment;
	uint8_t*   segment_container;
}scte_segment_t;

typedef struct
{
	AM_Bool_t                    running;
	long                         handle;
	pthread_mutex_t              lock;
	pthread_cond_t               cond;
	pthread_t                    thread;
	AM_SCTE27_Para_t             para;
	struct list_head  simple_bitmap_head;
	struct scte_segment_s        scte_segment;
	uint32_t                     subtitle_count;
} AM_SCTE27_Parser_t;

static int bean_bits;
static int bean_bytes;
static int bean_total;
static uint8_t *bean_buffer;

uint32_t subtitle_color_to_bitmap_color(uint32_t yCbCr)
{
	uint8_t R,G,B,A,Y,Cr,Cb,opaque_enable;
	uint32_t bitmap_color;
	char subtitle_color[2];

	subtitle_color[0] = (yCbCr >> 8) & 0xFF;
	subtitle_color[1] = yCbCr & 0xFF;

	Y = (subtitle_color[0] >> 3) & 0x1F;
	opaque_enable = (subtitle_color[0] >> 2) & 1;
	Cr = ((subtitle_color[0] & 0x3) << 3) | ((subtitle_color[1] >> 5) & 0x7);
	Cb = subtitle_color[1] & 0x1F;

	if (opaque_enable)
		A = 0xFF;
	else
		A = 0x80;

	Y*=8;
	Cr*=8;
	Cb*=8;

	R = 1.164*(Y-16)+1.596*(Cr-128);
	G = 1.164*(Y-16)-0.392*(Cb-128)-0.813*(Cr-128);
	B = 1.164*(Y-16)+2.017*(Cb-128);

	bitmap_color = (R << 24) | (G << 16) | (B << 8) | A;

	return bitmap_color;
}

void init_beans_separator(uint8_t* buffer, int size)
{
	bean_total = size;
	bean_buffer = (uint8_t*)buffer;
	bean_bytes = 0;
	bean_bits = 0;
}

int get_beans(int* value, int bits)
{
	int beans;
	int a, b;

	if (bits > 8)
		return -1;

	if (bits + bean_bits < 8)
	{
		beans = (bean_buffer[bean_bytes] >> (8 - bits -bean_bits)) & ((1<< bits) -1);
		bean_bits += bits;
		*value = beans;
	}
	else
	{
		a = bits + bean_bits - 8;
		b = bits - a;
		beans = ((bean_buffer[bean_bytes] & ((1<<b) -1)) << a)|
			((bean_buffer[bean_bytes+1] >> (8 - a)) & ((1<<a) -1));

		bean_bytes++;
		bean_bits = bits + bean_bits - 8;
		if ((bean_bytes > bean_total) || (bean_bytes == bean_total && bean_bits > 0))
		{
			bmp_log("buffer overflow, analyze error");
			return -1;
		}
		*value = beans;
	}

	return 1;
}

static uint32_t am_scte_get_video_pts()
{
#define VIDEO_PTS_PATH "/sys/class/tsync/pts_video"
	char buffer[16] = {0};

	AM_FileRead(VIDEO_PTS_PATH,buffer,16);
	return strtoul(buffer, NULL, 16);
}

static void clear_bitmap(AM_SCTE27_Parser_t *parser)
{
	uint8_t* matrix;

	if (!parser)
		return;
	if (!parser->para.bitmap)
		return;

	matrix = *parser->para.bitmap;

	memset(matrix, 0, parser->para.pitch * parser->para.height);
}

static int pts_bigger_than(uint32_t pts1, uint32_t pts2)
{
	int ret = 0;

	if (pts1 >= pts2)
	{
		if ((pts1 - pts2) > (1<<31))
			ret = 0;
		else
			ret = 1;
	}
	else if (pts1 < pts2)
	{
		if ((pts2 - pts1) > (1<<31))
			ret = 1;
		else
			ret = 0;
	}

	return ret;
}

static void generate_bitmap(AM_SCTE27_Parser_t *parser, scte_simple_bitmap_t *simple_bitmap, uint8_t* buf, int len)
{

#define assert_bits(x, y) \
{ \
	int z; \
	z = get_beans(&x, y); \
	if (z <= 0) \
	{ \
		bmp_log("left bits < 0, total %d byte %d bits %d", bean_total, bean_bytes, bean_bits); \
		break; \
	} \
}

#define set_rgba(x, y) \
{ \
	(x)[0] = (y >> 24) & 0xFF; \
	(x)[1] = (y >> 16) & 0xFF; \
	(x)[2] = (y >> 8) & 0xFF; \
	(x)[3] = y & 0xFF; \
}

	int size, on_bits, off_bits, row_length, row_pixels, total_rows;;
	uint8_t *row_start, *row_cursor, *matrix;
	int value;
	int row_count = 0;
	int i, j;

	size = simple_bitmap->bitmap_length;
	row_pixels = simple_bitmap->frame_bottom_h - simple_bitmap->frame_top_h;
	total_rows = simple_bitmap->frame_bottom_v - simple_bitmap->frame_top_v;
	matrix = (uint8_t*)malloc(row_pixels*total_rows);
	simple_bitmap->sub_bmp = matrix;

	init_beans_separator(buf, len);

	row_start = matrix;
	row_cursor = row_start;
	row_length = 0;

	while (1)
	{
		assert_bits(value, 1);
		if (value != 0)
		{
			//1xxxYYYYY
			assert_bits(on_bits, 3);
			if (on_bits == 0)
				on_bits = 8;
			for (i=0; i<on_bits; i++)
				*(row_cursor++) = PIXEL_CH;

			assert_bits(off_bits, 5);
			if (off_bits == 0)
				off_bits = 32;
			for (i=0; i<off_bits; i++)
				*(row_cursor++) = PIXEL_BG;
			row_length += (off_bits + on_bits);
		}
		else
		{
			assert_bits(value, 1);
			if (value != 0)
			{
				//01XXXXXX
				assert_bits(off_bits, 6);
				if (off_bits == 0)
					off_bits = 64;
				for (i=0; i<off_bits; i++)
					*(row_cursor++) = PIXEL_BG;
				row_length += off_bits;
			}
			else
			{
				assert_bits(value, 1);
				if (value != 0)
				{
					//001XXXX
					assert_bits(on_bits, 4);
					if (on_bits == 0)
						on_bits = 16;
					for (i=0; i<on_bits; i++)
						*(row_cursor++) = PIXEL_CH;
					row_length += on_bits;
				}
				else
				{
					assert_bits(value, 2);
					switch (value)
					{
						case 0: //Stuff
							bmp_log("stuffing code");
							break;
						case 1: //EOL
							if (row_length < row_pixels)
							{
								for (i=0; i<row_pixels - row_length; i++)
									*(row_cursor++) = PIXEL_BG;
							}

							row_start += row_pixels;
							row_cursor = row_start;
							row_length = 0;
							row_count++;

							break;
						case 2: //Reserved
						case 3: //Reserved
							bmp_log("reserved %d", value);
							break;
						default:
							bmp_log("can not be here");
							break;
					};
					if (row_count >= total_rows)
						break;
				}
			}
		}
	}
	//Outline

	if (simple_bitmap->outline_style == OUTLINE_STYLE_OL)
	{
		int thickness = simple_bitmap->style_para.outlined.outline_thickness;
		for (i=0; i<total_rows; i++)
		{
			for (j=0; j<row_pixels; j++)
			{
				if (matrix[i*row_pixels + j] == PIXEL_CH)
				{
					int outline_i, outline_j;
					for (outline_i = i - thickness; outline_i <= i + thickness; outline_i++)
					{
						for (outline_j = j -thickness; outline_j <= j + thickness; outline_j++)
						{
							int w = abs(outline_i - i);
							int h = abs(outline_j - j);
							if (sqrt(w*w + h*h) <= thickness)
							{
								if (matrix[outline_i*row_pixels + outline_j] == PIXEL_BG)
									matrix[outline_i*row_pixels + outline_j] = PIXEL_OL;
							}
						}
					}
				}
			}
		}
	}
	else if (simple_bitmap->outline_style == OUTLINE_STYLE_DS)
	{
		int offset_right = simple_bitmap->style_para.drop_shadow.shadow_right;
		int offset_below = simple_bitmap->style_para.drop_shadow.shadow_bottom;
		for (i=0; i<total_rows; i++)
		{
			for (j=0; j<row_pixels; j++)
			{
				if (matrix[i*row_pixels + j] == PIXEL_CH)
				{
					int target_ds = (i + offset_below) * row_pixels + j + offset_right;
					if (matrix[target_ds] == PIXEL_BG)
						matrix[target_ds] = PIXEL_DS;
				}
			}
		}
	}

	list_add_tail(&simple_bitmap->list, &parser->simple_bitmap_head);
	parser->subtitle_count ++;
	return;
}

static void decode_bitmap(AM_SCTE27_Parser_t *parser, scte_subtitle_t *sub_info, uint8_t* buffer, int size)
{
	/*uint32_t frame_time;
	char frame_rate_buf[256];
	int vframe_rate = 0;
	int frame_rate;*/
	uint32_t video_pts, frame_dur;
	//char* frame_rate_tmp;

	scte_simple_bitmap_t *simple_bitmap = malloc(sizeof(scte_simple_bitmap_t));

	if (!buffer || !simple_bitmap)
		return;
	/*
	AM_FileRead("/sys/class/video/frame_rate", frame_rate_buf, sizeof(frame_rate_buf));
	frame_rate_tmp = strstr(frame_rate_buf, "panel");
	if (frame_rate_tmp)
	{
		frame_rate_tmp = strstr(frame_rate_tmp, "fps");
		scte_log("fps %s", frame_rate_tmp);
		if (frame_rate_tmp)
		{
			vframe_rate = strtoul(frame_rate_tmp + 4, NULL, 10);
			scte_log("framerate %d", vframe_rate);
		}
	}*/

	memset(simple_bitmap, 0, sizeof(scte_simple_bitmap_t));
	//Get reveal and hide pts
	switch (sub_info->display_std)
	{
		case DISPLAY_STD_720_480_30:
			frame_dur = 3000;
			break;
		case DISPLAY_STD_720_576_25:
			//frame_rate = 25; //Protocol says 25
			frame_dur = 3600;
			break;
		case DISPLAY_STD_1280_720_60:
			frame_dur = 1500;
			break;
		case DISPLAY_STD_1920_1080_60:
			frame_dur = 1500;
			break;
		default:
			frame_dur = 3600;
			break;
	}
	simple_bitmap->pre_clear = sub_info->pre_clear;
	simple_bitmap->immediate = sub_info->immediate;
	simple_bitmap->show_status = DISPLAY_STATUS_INIT;
	simple_bitmap->reveal_pts = sub_info->pts;
	simple_bitmap->disapear_pts = sub_info->pts + frame_dur * sub_info->duration;
	video_pts = am_scte_get_video_pts();
	if (pts_bigger_than(video_pts, simple_bitmap->disapear_pts))
	{
		if (parser->para.report)
			parser->para.report(parser, AM_SCTE27_Decoder_Error_TimeError);
		scte_log("video pts larger than current data, vpts %x disappear_pts %x", video_pts, simple_bitmap->disapear_pts);
	}
#if 0
	scte_log("display_std %d frame_rate %d dur %d rev_pts %x hide_pts %x",
		sub_info->display_std,
		frame_rate,
		sub_info->duration,
		simple_bitmap->reveal_pts,
		simple_bitmap->disapear_pts);
#endif
	simple_bitmap->bg_style = buffer[0] & (1<<2);
	simple_bitmap->outline_style = buffer[0] & 0x3;
	simple_bitmap->character_color = (buffer[1] << 8) | buffer[2];
	simple_bitmap->character_color_rgba = subtitle_color_to_bitmap_color(simple_bitmap->character_color);
	simple_bitmap->frame_top_h = (buffer[3] << 4) | ((buffer[4] >> 4) & 0xF);
	simple_bitmap->frame_top_v = ((buffer[4] & 0xF) << 8) | buffer[5];
	simple_bitmap->frame_bottom_h = (buffer[6] << 4) | ((buffer[7] >> 4) & 0xF);
	simple_bitmap->frame_bottom_v = ((buffer[7] & 0xF) << 8) | buffer[8];

	buffer = &buffer[9];

	if (simple_bitmap->bg_style)
	{
		simple_bitmap->bg_style_para.top_h = (buffer[0] << 4) | ((buffer[1] >> 4) & 0xF);
		simple_bitmap->bg_style_para.top_v = ((buffer[1] & 0xF) << 8) | buffer[2];
		simple_bitmap->bg_style_para.bottom_h = (buffer[3] << 4) | ((buffer[4] >> 4) & 0xF);
		simple_bitmap->bg_style_para.bottom_v = ((buffer[4] & 0xF) << 8) | buffer[5];
		simple_bitmap->bg_style_para.frame_color = (buffer[6] << 8) | buffer[7];
		simple_bitmap->bg_style_para.frame_color_rgba = subtitle_color_to_bitmap_color(simple_bitmap->bg_style_para.frame_color);

		buffer = &buffer[8];
	}

	switch (simple_bitmap->outline_style)
	{
		case BMP_OUTLINE_OUTLINE:
			simple_bitmap->style_para.outlined.outline_thickness = buffer[0] & 0xF;
			simple_bitmap->style_para.outlined.outline_color = (buffer[1] << 8) | buffer[2];
			simple_bitmap->style_para.outlined.outline_color_rgba = subtitle_color_to_bitmap_color(simple_bitmap->style_para.outlined.outline_color);
			buffer = &buffer[3];
			break;
		case BMP_OUTLINE_DROPSHADOW:
			simple_bitmap->style_para.drop_shadow.shadow_right = (buffer[0] >> 4) & 0xF;
			simple_bitmap->style_para.drop_shadow.shadow_bottom = buffer[0] & 0xF;
			simple_bitmap->style_para.drop_shadow.shadow_color = (buffer[1] << 8) | buffer[2];
			simple_bitmap->style_para.drop_shadow.shadow_color_rgba = subtitle_color_to_bitmap_color(simple_bitmap->style_para.drop_shadow.shadow_color);
			buffer = &buffer[3];
			break;
		case BMP_OUTLINE_RESERVED:
			buffer = &buffer[4];
			break;
		case BMP_OUTLINE_NONE:
		default:
			break;
	}
	simple_bitmap->bitmap_length = (buffer[0] << 8) | buffer[1];
	//scte_log("size %d bmp_size %d", size, simple_bitmap->bitmap_length);
	generate_bitmap(parser, simple_bitmap, &buffer[2], simple_bitmap->bitmap_length);
}

static void render_bitmap(AM_SCTE27_Parser_t *parser, scte_simple_bitmap_t *simple_bitmap)
{
	int pitch, row_pixels, total_rows;
	uint8_t *bmp, *target_bmp_start, *cursor, *matrix;
	int i, j;

	if (!parser || !simple_bitmap)
		return;

	row_pixels = simple_bitmap->frame_bottom_h - simple_bitmap->frame_top_h;
	total_rows = simple_bitmap->frame_bottom_v - simple_bitmap->frame_top_v;
	matrix = simple_bitmap->sub_bmp;
	bmp = *(parser->para.bitmap);
	pitch = parser->para.pitch; //RGBA
	target_bmp_start = bmp + simple_bitmap->frame_top_v * pitch + simple_bitmap->frame_top_h * 4;

	for (i=0; i<total_rows; i++)
	{
		cursor = target_bmp_start + i * pitch;
		for (j=0; j<row_pixels; j++)
		{
			switch (matrix[i*row_pixels + j])
			{
				case PIXEL_BG: //Bg
					if (simple_bitmap->bg_style)
						set_rgba(cursor + j * 4, simple_bitmap->bg_style_para.frame_color_rgba);
					break;
				case PIXEL_CH: //Font
					set_rgba(cursor + j * 4, simple_bitmap->character_color_rgba);
					break;
				case PIXEL_OL: // Outline
					set_rgba(cursor + j * 4, simple_bitmap->style_para.outlined.outline_color_rgba);
					break;
				case PIXEL_DS: // Drop shadow
					set_rgba(cursor + j * 4, simple_bitmap->style_para.drop_shadow.shadow_color_rgba);
					break;
				default:
					//scte_log("render should not be here");
					break;
			}
		}
	}
#if 0
	scte_log("render_bitmap: th %d tv %d bh %d bv %d bg_style %d ol_style %d bitmap_len 0x%x bgc %x chc %x",
		simple_bitmap->frame_top_h,
		simple_bitmap->frame_top_v,
		simple_bitmap->frame_bottom_h,
		simple_bitmap->frame_bottom_v,
		simple_bitmap->bg_style,
		simple_bitmap->outline_style,
		simple_bitmap->bitmap_length,
		simple_bitmap->bg_style_para.frame_color,
		simple_bitmap->character_color);
#endif
}

static void node_check(AM_SCTE27_Parser_t *parser)
{
	scte_simple_bitmap_t *simple_bitmap, *tmp, *pre_clear_bmp = NULL;
	int has_sub = 0;
	int need_redraw = 0;
	uint32_t video_pts;

	if (!parser)
		return;

	if (list_empty(&parser->simple_bitmap_head))
		return;

	video_pts = am_scte_get_video_pts();

	/* Checkj if some bitmap should be displayed.*/
	list_for_each_entry_safe(simple_bitmap, tmp, &parser->simple_bitmap_head, list, scte_simple_bitmap_t)
	{
		if (pts_bigger_than(video_pts, simple_bitmap->reveal_pts)
				&& pts_bigger_than(simple_bitmap->disapear_pts, video_pts)) {
			has_sub = 1;
			if (simple_bitmap->show_status == DISPLAY_STATUS_INIT)
				need_redraw = 1;
		}
	}

	//scte_log("has sub: %d", has_sub);

	/*Clear the old bitmap only when has somethings to be displayed*/
	list_for_each_entry_safe(simple_bitmap, tmp, &parser->simple_bitmap_head, list, scte_simple_bitmap_t)
	{
		int diff = simple_bitmap->disapear_pts - video_pts;
		int del  = 0;

		if (diff < 0) {
			if (diff > -90000 * 5)
				del = 1;
			else if (has_sub)
				del = 1;
		}

		if (del)
		{
			need_redraw = 1;
			scte_log("remove pts %x - %x", simple_bitmap->reveal_pts, simple_bitmap->disapear_pts);
			list_del(&simple_bitmap->list);
			if (simple_bitmap->sub_bmp)
				free(simple_bitmap->sub_bmp);
			free(simple_bitmap);
			parser->subtitle_count --;
			continue;
		}

		if (pts_bigger_than(video_pts, simple_bitmap->reveal_pts) ||
			simple_bitmap->immediate)
		{

			if (simple_bitmap->pre_clear)
				pre_clear_bmp = simple_bitmap;
		}
	}

	if (pre_clear_bmp) {
		list_for_each_entry_safe(simple_bitmap, tmp, &parser->simple_bitmap_head, list, scte_simple_bitmap_t)
		{
			if (simple_bitmap == pre_clear_bmp)
				break;

			scte_log("remove imm pts %x - %x", simple_bitmap->reveal_pts, simple_bitmap->disapear_pts);
			need_redraw = 1;
			list_del(&simple_bitmap->list);
			if (simple_bitmap->sub_bmp)
				free(simple_bitmap->sub_bmp);
			free(simple_bitmap);
			parser->subtitle_count --;
		}

	}

	if (need_redraw)
	{
		parser->para.draw_begin(parser);

		clear_bitmap(parser);

		list_for_each_entry_safe(simple_bitmap, tmp, &parser->simple_bitmap_head, list, scte_simple_bitmap_t)
		{
			scte_log("node_check vpts %x bmp_pts %x outdata_pts %x",video_pts,simple_bitmap->reveal_pts,simple_bitmap->disapear_pts);

			//Draw node
			if (pts_bigger_than(video_pts, simple_bitmap->reveal_pts) ||
				simple_bitmap->immediate)
			{

				if (simple_bitmap->show_status == DISPLAY_STATUS_INIT)
				{
					//scte_log("node render imm: %d", simple_bitmap->immediate);
					if (parser->para.report_available) {
						//scte_log("scte:report_available");
						parser->para.report_available(parser);
					}
					simple_bitmap->show_status = DISPLAY_STATUS_SHOW;
				}

				render_bitmap(parser, simple_bitmap);
			}
		}

		parser->para.draw_end(parser);
		scte_log("Need redraw");
	}
}

static void* scte27_thread(void *arg)
{
	AM_SCTE27_Parser_t *parser = (AM_SCTE27_Parser_t*)arg;

	AM_DEBUG(0, "scte27 thread start");

	pthread_mutex_lock(&parser->lock);

	while (parser->running)
	{
		if (parser->running) {
			struct timespec ts;
			int timeout = 20;

			AM_TIME_GetTimeSpecTimeout(timeout, &ts);
			pthread_cond_timedwait(&parser->cond, &parser->lock, &ts);
		}

		node_check(parser);
	}

	pthread_mutex_unlock(&parser->lock);
	AM_DEBUG(0, "scte27 thread end");
	return NULL;
}

AM_ErrorCode_t AM_SCTE27_Create(AM_SCTE27_Handle_t *handle, AM_SCTE27_Para_t *para)
{
	AM_SCTE27_Parser_t *parser;

	if (!handle || !para)
	{
		return AM_SCTE27_ERR_INVALID_PARAM;
	}

	parser = (AM_SCTE27_Parser_t*)malloc(sizeof(AM_SCTE27_Parser_t));
	if (!parser)
	{
		return AM_SCTE27_ERR_NO_MEM;
	}

	memset(parser, 0, sizeof(AM_SCTE27_Parser_t));

	pthread_mutex_init(&parser->lock, NULL);
	pthread_cond_init(&parser->cond, NULL);
	parser->para = *para;

	*handle = parser;

	AM_SigHandlerInit();

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_SCTE27_Destroy(AM_SCTE27_Handle_t handle)
{
	AM_SCTE27_Parser_t *parser;

	if (!handle)
	{
		AM_DEBUG(0, "scte27 destroy with invalid handle");
		return AM_SCTE27_ERR_INVALID_HANDLE;
	}

	AM_DEBUG(0, "scte27 destroy");
	parser = (AM_SCTE27_Parser_t*)handle;

	AM_SCTE27_Stop(handle);

	pthread_mutex_destroy(&parser->lock);
	pthread_cond_destroy(&parser->cond);

	free(parser);

	return AM_SUCCESS;
}

void* AM_SCTE27_GetUserData(AM_SCTE27_Handle_t handle)
{
	AM_SCTE27_Parser_t *parser;

	parser = (AM_SCTE27_Parser_t*)handle;
	if (!parser)
	{
		return NULL;
	}

	return parser->para.user_data;
}

void clean_parser_segment(AM_SCTE27_Parser_t *parser)
{
	if (parser->scte_segment.segment_container)
	{
		free(parser->scte_segment.segment_container);
		parser->scte_segment.segment_container = NULL;
		memset(&parser->scte_segment, 0, sizeof(scte_segment_t));
	}
	parser->scte_segment.table_ext = 0;
	parser->scte_segment.curr_no = 0;
	parser->scte_segment.last_no = 0;
	parser->scte_segment.seg_size = 0;
	parser->scte_segment.has_segment = 0;
}

int decode_message_body(AM_SCTE27_Parser_t *parser, const uint8_t *buf, int size)
{
	scte_subtitle_t sub_node = {0};
	uint8_t *bitmap = NULL;
	int width;
	int height;
	int video_width;
	int video_height;
	char read_buff[64];
	int lang_valid = -1;
	int i;

	memcpy(sub_node.lang, buf, 3);
	for (i=0; i<3; i++)
	{
		if ((sub_node.lang[i] >= 'a' && sub_node.lang[i] <= 'z') ||
			(sub_node.lang[i] >= 'A' && sub_node.lang[i] <= 'Z'))
		{
			lang_valid = 0;
		}
		else
		{
			lang_valid = -1;
			break;
		}
	}
	if (parser->para.lang_cb && lang_valid == 0)
		parser->para.lang_cb(parser, sub_node.lang, 4);
	sub_node.pre_clear = buf[3] & (1<<7);
	sub_node.immediate = buf[3] & (1<<6);
	sub_node.display_std = buf[3] & 0x1F;
	sub_node.pts = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
	sub_node.sub_type = (buf[8] >> 4) & 0xF;
	sub_node.duration = ((buf[8] & 0x7) << 8) | buf[9];
	sub_node.block_len = (buf[10] << 8) | buf[11];

	scte_log("pre_clear %d imm %d", sub_node.pre_clear, sub_node.immediate);

	if (sub_node.block_len < 12 || sub_node.block_len > size)
	{
		scte_log("sub_node->block_len invalid blen %x size %x", sub_node.block_len, size);
		return -1;
	}

	bitmap = &buf[12];
	AM_FileRead("/sys/class/video/frame_height", read_buff, sizeof(read_buff));
	video_height = strtoul(read_buff, NULL, 10);
	AM_FileRead("/sys/class/video/frame_width", read_buff, sizeof(read_buff));
	video_width = strtoul(read_buff, NULL, 10);

	switch (sub_node.display_std)
	{
		case DISPLAY_STD_720_480_30:
			width = 720;
			height = 480;
			break;
		case DISPLAY_STD_720_576_25:
			//frame_rate = 25; //Protocol says 25
			width = 720;
			height = 576;
			break;
		case DISPLAY_STD_1280_720_60:
			width = 1280;
			height = 720;
			break;
		case DISPLAY_STD_1920_1080_60:
			width = 1920;
			height = 1080;
			break;
		default:
			width = video_width;
			height = video_height;
			scte_log("display standard error");
			break;
	}

	//Special treatment.
	/*if (video_height == 480)
	{
		width = 720;
		height = 480;
	}*/
#if 0
	scte_log("msg_w %d msg_h %d video width %d height %d std %d",
	width, height, video_width, video_height, sub_node.display_std);
	scte_log("!!msg_len %d lang: %c%c%c clear %d imm %d std %d pts 0x%x sub_t %d duration 0x%x b_len 0x%x",
		size,
		buf[0], buf[1], buf[2],
		sub_node.pre_clear,
		sub_node.immediate,
		sub_node.display_std,
		sub_node.pts,
		sub_node.sub_type,
		sub_node.duration,
		sub_node.block_len);
#endif
	parser->para.update_size(parser, width, height);
	if (sub_node.sub_type == SUB_TYPE_SIMPLE_BITMAP)
		decode_bitmap(parser, &sub_node, bitmap, sub_node.block_len);

	return 0;
}

AM_ErrorCode_t AM_SCTE27_Decode(AM_SCTE27_Handle_t handle, const uint8_t *buf, int size)
{
	AM_SCTE27_Parser_t *parser;
	int segmentation_overlay_included;
	int section_length;
	int protocol_version;
	int segment_size;
	int seg_errno;
	int ret = 0;
	enum AM_SCTE27_Decoder_Error error_flag = AM_SCTE27_Decoder_Error_END;

	if(!handle)
	{
		return AM_SCTE27_ERR_INVALID_HANDLE;
	}

	if(!buf || !size)
	{
		return AM_SCTE27_ERR_INVALID_PARAM;
	}

	parser = (AM_SCTE27_Parser_t*)handle;

	pthread_mutex_lock(&parser->lock);

	section_length = ((buf[1] & 0xF) << 8) | buf[2];
	protocol_version = buf[3] & 0x3F;
	if (protocol_version != 0)
		scte_log("Unsupport scte version: %d only support 0", protocol_version);

	// 1. segmentation_overlay_included
	segmentation_overlay_included = (buf[3] >> 6) & 1;

	// if segmentation_overlay_included combine segment.
	if (segmentation_overlay_included)
	{
		int table_ext = (buf[4] << 8) | buf[5];
		int last_no = (buf[6] << 4) | ((buf[7] >> 4) & 0xF);
		int curr_no = ((buf[7] & 0xF)) << 8 | buf[8];

		if (curr_no > last_no)
		{
			seg_errno = AM_SCTE27_PACKET_INVALID;
			error_flag = AM_SCTE27_Decoder_Error_LoseData;
			goto SEG_ERROR;
		}

		if (curr_no == 0)
		{
			scte_log("scte overlay");
			clean_parser_segment(parser);
			parser->scte_segment.seg_size = 0;
			parser->scte_segment.table_ext = table_ext;
			parser->scte_segment.last_no = last_no;
			parser->scte_segment.curr_no = -1;
		}

		if (table_ext != parser->scte_segment.table_ext &&
			parser->scte_segment.has_segment)
		{
			clean_parser_segment(parser);
			scte_log("segment ext does not match");
			seg_errno = AM_SCTE27_PACKET_INVALID;
			error_flag = AM_SCTE27_Decoder_Error_InvalidData;
			goto SEG_ERROR;
		}

		if ((curr_no != (parser->scte_segment.curr_no + 1)) &&
			parser->scte_segment.has_segment)
		{
			clean_parser_segment(parser);
			scte_log("segment curr_no does not match");
			seg_errno = AM_SCTE27_PACKET_INVALID;
			error_flag = AM_SCTE27_Decoder_Error_LoseData;
			goto SEG_ERROR;
		}

		parser->scte_segment.curr_no = curr_no;
		parser->scte_segment.has_segment = 1;
		segment_size = section_length - 1 - 5 - 4;
		parser->scte_segment.segment_container = realloc(parser->scte_segment.segment_container, parser->scte_segment.seg_size + segment_size);
		memcpy(&parser->scte_segment.segment_container[parser->scte_segment.seg_size],
			&buf[9],
			segment_size);
		parser->scte_segment.seg_size += segment_size;

		if (curr_no == last_no)
		{
			if (parser->scte_segment.seg_size < 12)
			{
				seg_errno = AM_SCTE27_PACKET_INVALID;
				error_flag = AM_SCTE27_Decoder_Error_InvalidData;
				goto SEG_ERROR;
			}

			ret = decode_message_body(parser,
				parser->scte_segment.segment_container,
				parser->scte_segment.seg_size);
			clean_parser_segment(parser);
		}
	}
	// decode message body
	else
	{
		if (parser->scte_segment.has_segment)
			clean_parser_segment(parser);
		if ((section_length - 1 - 4) < 12)
			return AM_SCTE27_PACKET_INVALID;
		ret = decode_message_body(parser, &buf[4], section_length - 1 - 4);
	}
	if (ret)
		scte_log("decode_message_body failed");

	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
SEG_ERROR:
	if (parser->para.report)
		parser->para.report(parser, error_flag);
	pthread_mutex_unlock(&parser->lock);
	return seg_errno;
}

AM_ErrorCode_t AM_SCTE27_Start(AM_SCTE27_Handle_t handle)
{
	AM_SCTE27_Parser_t *parser;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if(!handle)
	{
		return AM_SCTE27_ERR_INVALID_HANDLE;
	}

	parser = (AM_SCTE27_Parser_t*)handle;

	pthread_mutex_lock(&parser->lock);

	if(!parser->running)
	{
		parser->running = AM_TRUE;
		if(pthread_create(&parser->thread, NULL, scte27_thread, parser))
		{
			parser->running = AM_FALSE;
			ret = AM_SCTE27_ERR_CANNOT_CREATE_THREAD;
		}
	}
	INIT_LIST_HEAD(&parser->simple_bitmap_head);

	pthread_mutex_unlock(&parser->lock);

	return ret;
}

AM_ErrorCode_t AM_SCTE27_Stop(AM_SCTE27_Handle_t handle)
{
	AM_SCTE27_Parser_t *parser;
	pthread_t th;
	AM_Bool_t wait = AM_FALSE;

	if(!handle)
	{
		return AM_SCTE27_ERR_INVALID_HANDLE;
	}

	parser = (AM_SCTE27_Parser_t*)handle;

	pthread_mutex_lock(&parser->lock);

	if(parser->running)
	{
		AM_DEBUG(0, "scte27 stop thread");
		parser->running = AM_FALSE;
		pthread_cond_signal(&parser->cond);
		th = parser->thread;
		wait = AM_TRUE;
	}

	pthread_mutex_unlock(&parser->lock);
	pthread_cond_broadcast(&parser->cond);
	//pthread_kill(parser->thread, SIGALRM);
	pthread_join(parser->thread, NULL);

	return AM_SUCCESS;
}




