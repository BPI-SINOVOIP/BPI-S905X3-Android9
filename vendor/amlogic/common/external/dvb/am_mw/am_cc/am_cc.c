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
/**\file am_cc.c
 * \brief 数据库模块
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2013-03-10: create the document
 ***************************************************************************/
#define AM_DEBUG_LEVEL 5

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include "am_debug.h"
#include "am_time.h"
#include "am_userdata.h"
#include "am_misc.h"
#include "am_cc.h"
#include "am_cc_internal.h"
#include "tvin_vbi.h"
#include "am_cond.h"
#include "math.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define CC_POLL_TIMEOUT 16
#define CC_FLASH_PERIOD 1000
#define CC_CLEAR_TIME 	5000
#define LINE_284_TIMEOUT 4000

#define AMSTREAM_USERDATA_PATH "/dev/amstream_userdata"
#define VBI_DEV_FILE "/dev/vbi"
#define VIDEO_WIDTH_FILE "/sys/class/video/frame_width"
#define VIDEO_HEIGHT_FILE "/sys/class/video/frame_height"

#define _TM_T 'V'
struct vout_CCparam_s {
    unsigned int type;
	unsigned char data1;
	unsigned char data2;
};
#define VOUT_IOC_CC_OPEN           _IO(_TM_T, 0x01)
#define VOUT_IOC_CC_CLOSE          _IO(_TM_T, 0x02)
#define VOUT_IOC_CC_DATA           _IOW(_TM_T, 0x03, struct vout_CCparam_s)


#define SAFE_TITLE_AREA_WIDTH (672) /* 16 * 42 */
#define SAFE_TITLE_AREA_HEIGHT (390) /* 26 * 15 */
#define ROW_W (SAFE_TITLE_AREA_WIDTH/42)
#define ROW_H (SAFE_TITLE_AREA_HEIGHT/15)




extern void vbi_decode_caption(vbi_decoder *vbi, int line, const uint8_t *buf);
extern void vbi_refresh_cc(vbi_decoder *vbi);
extern void vbi_caption_reset(vbi_decoder *vbi);

/****************************************************************************
 * Static data
 ***************************************************************************/
static int vout_fd = -1;
static const vbi_opacity opacity_map[AM_CC_OPACITY_MAX] =
{
	VBI_OPAQUE,             /*not used, just take a position*/
	VBI_TRANSPARENT_SPACE,  /*AM_CC_OPACITY_TRANSPARENT*/
	VBI_SEMI_TRANSPARENT,   /*AM_CC_OPACITY_TRANSLUCENT*/
	VBI_OPAQUE,             /*AM_CC_OPACITY_SOLID*/
	VBI_OPAQUE,             /*AM_CC_OPACITY_FLASH*/
};

static const vbi_color color_map[AM_CC_COLOR_MAX] =
{
	VBI_BLACK, /*not used, just take a position*/
	VBI_WHITE,
	VBI_BLACK,
	VBI_RED,
	VBI_GREEN,
	VBI_BLUE,
	VBI_YELLOW,
	VBI_MAGENTA,
	VBI_CYAN,
};

static int chn_send[AM_CC_CAPTION_MAX];

static AM_ErrorCode_t am_cc_calc_caption_size(int *w, int *h)
{
	int rows, vw, vh;
	char wbuf[32];
	char hbuf[32];
	AM_ErrorCode_t ret;

#if 1
	vw = 0;
	vh = 0;
	ret  = AM_FileRead(VIDEO_WIDTH_FILE, wbuf, sizeof(wbuf));
	ret |= AM_FileRead(VIDEO_HEIGHT_FILE, hbuf, sizeof(hbuf));
	if (ret != AM_SUCCESS ||
		sscanf(wbuf, "%d", &vw) != 1 ||
		sscanf(hbuf, "%d", &vh) != 1)
	{
		AM_DEBUG(4, "Get video size failed, default set to 16:9");
		vw = 1920;
		vh = 1080;
	}
#else
	vw = 1920;
	vh = 1080;
#endif
	if (vh == 0)
	{
		vw = 1920;
		vh = 1080;
	}
	rows = (vw * 3 * 32) / (vh * 4);
	if (rows < 32)
		rows = 32;
	else if (rows > 42)
		rows = 42;

	AM_DEBUG(4, "Video size: %d X %d, rows %d", vw, vh, rows);

	*w = rows * ROW_W;
	*h = SAFE_TITLE_AREA_HEIGHT;

	return AM_SUCCESS;
}

uint32_t am_cc_get_video_pts()
{
#define VIDEO_PTS_PATH "/sys/class/tsync/pts_video"
	char buffer[16] = {0};
	AM_FileRead(VIDEO_PTS_PATH,buffer,16);
	return strtoul(buffer, NULL, 16);
}

static void am_cc_get_page_canvas(AM_CC_Decoder_t *cc, struct vbi_page *pg, int* x_point, int* y_point)
{
	int safe_width, safe_height;

	am_cc_calc_caption_size(&safe_width, &safe_height);
	if (pg->pgno <= 8)
	{
		*x_point = 0;
		*y_point = 0;
	}
	else
	{
		int x, y, r, b;
		struct dtvcc_service *ds = &cc->decoder.dtvcc.service[pg->pgno- 1 - 8];
		struct dtvcc_window *dw = &ds->window[pg->subno];

		if (dw->anchor_relative)
		{
			x = dw->anchor_horizontal * safe_width / 100;
			y = dw->anchor_vertical * SAFE_TITLE_AREA_HEIGHT/ 100;
		}
		else
		{
			x = dw->anchor_horizontal * safe_width / 210;
			y = dw->anchor_vertical * SAFE_TITLE_AREA_HEIGHT/ 75;
		}

		switch (dw->anchor_point)
		{
			case 0:
			default:
				break;
			case 1:
				x -= ROW_W*dw->column_count/2;
				break;
			case 2:
				x -= ROW_W*dw->column_count;
				break;
			case 3:
				y -= ROW_H*dw->row_count/2;
				break;
			case 4:
				x -= ROW_W*dw->column_count/2;
				y -= ROW_H*dw->row_count/2;
				break;
			case 5:
				x -= ROW_W*dw->column_count;
				y -= ROW_H*dw->row_count/2;
				break;
			case 6:
				y -= ROW_H*dw->row_count;
				break;
			case 7:
				x -= ROW_W*dw->column_count/2;
				y -= ROW_H*dw->row_count;
				break;
			case 8:
				x -= ROW_W*dw->column_count;
				y -= ROW_H*dw->row_count;
				break;
		}

		r = x + dw->column_count * ROW_W;
		b = y + dw->row_count * ROW_H;

		AM_DEBUG(2, "x %d, y %d, r %d, b %d", x, y, r, b);

		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		if (r > safe_width)
			r = safe_width;
		if (b > SAFE_TITLE_AREA_HEIGHT)
			b = SAFE_TITLE_AREA_HEIGHT;

		/*calc the real displayed rows & cols*/
		//pg->columns = (r - x) / ROW_W;
		//pg->rows = (b - y) / ROW_H;

		/* AM_DEBUG(2, "window prio(%d), row %d, cols %d ar %d, ah %d, av %d, "
			"ap %d, screen position(%d, %d), displayed rows/cols(%d, %d)",
			dw->priority, dw->row_count, dw->column_count, dw->anchor_relative,
			dw->anchor_horizontal, dw->anchor_vertical, dw->anchor_point,
			x, y, pg->rows, pg->columns);
		*/

		*x_point = x;
		*y_point = y;
	}
}

static void am_cc_override_by_user_options(AM_CC_Decoder_t *cc, struct vbi_page *pg)
{
	int i, j, opacity;
	vbi_char *ac;

#define OVERRIDE_ATTR(_uo, _uon, _text, _attr, _map)\
	AM_MACRO_BEGIN\
		if (_uo > _uon##_DEFAULT && _uo < _uon##_MAX){\
			_text->_attr = _map[_uo];\
		}\
	AM_MACRO_END

	for (i=0; i<pg->rows; i++)
	{
		for (j=0; j<pg->columns; j++)
		{
			ac = &pg->text[i*pg->columns + j];
#if 0//pd-114913
			if (pg->pgno <= 8)
			{
				/*NTSC CC style*/
				if (ac->opacity == VBI_OPAQUE)
					ac->opacity = (VBI_OPAQUE<<4) | VBI_OPAQUE;
				else if (ac->opacity == VBI_SEMI_TRANSPARENT)
					ac->opacity = (VBI_OPAQUE<<4) | VBI_SEMI_TRANSPARENT;
				else if (ac->opacity == VBI_TRANSPARENT_FULL)
					ac->opacity = (VBI_OPAQUE<<4) | VBI_TRANSPARENT_SPACE;
			}
			else
#endif
			if (ac->unicode == 0x20 && ac->opacity == VBI_TRANSPARENT_SPACE)
			{
				ac->opacity = (VBI_TRANSPARENT_SPACE<<4) | VBI_TRANSPARENT_SPACE;
			}
			else
			{
				/*DTV CC style, override by user options*/
				OVERRIDE_ATTR(cc->spara.user_options.fg_color, AM_CC_COLOR, \
					ac, foreground, color_map);
				OVERRIDE_ATTR(cc->spara.user_options.bg_color, AM_CC_COLOR, \
					ac, background, color_map);
				OVERRIDE_ATTR(cc->spara.user_options.fg_opacity, AM_CC_OPACITY, \
					ac, opacity, opacity_map);
				opacity = ac->opacity;
				OVERRIDE_ATTR(cc->spara.user_options.bg_opacity, AM_CC_OPACITY, \
					ac, opacity, opacity_map);
				ac->opacity = ((opacity&0x0F) << 4) | (ac->opacity&0x0F);

				/*flash control*/
				if (cc->spara.user_options.bg_opacity == AM_CC_OPACITY_FLASH &&
					cc->spara.user_options.fg_opacity != AM_CC_OPACITY_FLASH)
				{
					/*only bg flashing*/
					if (cc->flash_stat == FLASH_SHOW)
					{
						ac->opacity &= 0xF0;
						ac->opacity |= VBI_OPAQUE;
					}
					else if (cc->flash_stat == FLASH_HIDE)
					{
						ac->opacity &= 0xF0;
						ac->opacity |= VBI_TRANSPARENT_SPACE;
					}
				}
				else if (cc->spara.user_options.bg_opacity != AM_CC_OPACITY_FLASH &&
					cc->spara.user_options.fg_opacity == AM_CC_OPACITY_FLASH)
				{
					/*only fg flashing*/
					if (cc->flash_stat == FLASH_SHOW)
					{
						ac->opacity &= 0x0F;
						ac->opacity |= (VBI_OPAQUE<<4);
					}
					else if (cc->flash_stat == FLASH_HIDE)
					{
						ac->opacity &= 0x0F;
						ac->opacity |= (ac->opacity<<4);
						ac->foreground = ac->background;
					}
				}
				else if (cc->spara.user_options.bg_opacity == AM_CC_OPACITY_FLASH &&
					cc->spara.user_options.fg_opacity == AM_CC_OPACITY_FLASH)
				{
					/*bg & fg both flashing*/
					if (cc->flash_stat == FLASH_SHOW)
					{
						ac->opacity = (VBI_OPAQUE<<4) | VBI_OPAQUE;
					}
					else if (cc->flash_stat == FLASH_HIDE)
					{
						ac->opacity = (VBI_TRANSPARENT_SPACE<<4) | VBI_TRANSPARENT_SPACE;
					}
				}
			}
		}
	}
}

static void am_cc_vbi_event_handler(vbi_event *ev, void *user_data)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)user_data;
	int pgno, subno, ret;
	char* json_buffer;
	AM_CC_JsonChain_t* node, *json_chain_head;
	static int count = 0;
	json_chain_head = cc->json_chain_head;
	if (ev->type == VBI_EVENT_CAPTION) {
		if (cc->hide)
			return;

		AM_DEBUG(AM_DEBUG_LEVEL, "VBI Caption event: pgno %d, cur_pgno %d",
			ev->ev.caption.pgno, cc->vbi_pgno);

		pgno = ev->ev.caption.pgno;
		if (pgno < AM_CC_CAPTION_MAX) {
			if (cc->process_update_flag == 0)
			{
				cc->curr_data_mask   |= 1 << pgno;
				cc->curr_switch_mask |= 1 << pgno;
			}
		}

		pthread_mutex_lock(&cc->lock);

		if (cc->vbi_pgno == ev->ev.caption.pgno &&
			cc->flash_stat == FLASH_NONE)
		{
			json_buffer = (char*)malloc(JSON_STRING_LENGTH);
			if (!json_buffer)
			{
				AM_DEBUG(0, "json buffer malloc failed");
				pthread_mutex_unlock(&cc->lock);
				return;
			}
			/* Convert to json attributes */
			ret = tvcc_to_json (&cc->decoder, cc->vbi_pgno, json_buffer, JSON_STRING_LENGTH);
			if (ret == -1)
			{
				AM_DEBUG(0, "tvcc_to_json failed");
				if (json_buffer)
					free(json_buffer);
				pthread_mutex_unlock(&cc->lock);
				return;
			}
			//AM_DEBUG(1, "---------json: %s %d", json_buffer, count);
			//count ++;
			/* Create one json node */
			node = (AM_CC_JsonChain_t*)malloc(sizeof(AM_CC_JsonChain_t));
			node->buffer = json_buffer;
			node->pts = cc->decoder_cc_pts;

			AM_DEBUG(AM_DEBUG_LEVEL, "vbi_event node_pts %x decoder_pts %x", node->pts, cc->decoder_cc_pts);
			/* TODO: Add node time */
			AM_TIME_GetTimeSpec(&node->decode_time);

			/* Add to json chain */
			json_chain_head->json_chain_prior->json_chain_next = node;
			json_chain_head->count++;
			node->json_chain_next = json_chain_head;
			node->json_chain_prior = json_chain_head->json_chain_prior;
			json_chain_head->json_chain_prior = node;

			cc->render_flag = AM_TRUE;
			cc->need_clear = AM_FALSE;
			cc->timeout = CC_CLEAR_TIME;
		}

		pthread_mutex_unlock(&cc->lock);
		pthread_cond_signal(&cc->cond);
	} else if ((ev->type == VBI_EVENT_ASPECT) || (ev->type == VBI_EVENT_PROG_INFO)) {
		cc->curr_data_mask |= 1 << AM_CC_CAPTION_XDS;

		if (cc->cpara.pinfo_cb)
			cc->cpara.pinfo_cb(cc, ev->ev.prog_info);
	} else if (ev->type == VBI_EVENT_NETWORK) {
		cc->curr_data_mask |= 1 << AM_CC_CAPTION_XDS;

		if (cc->cpara.network_cb)
			cc->cpara.network_cb(cc, &ev->ev.network);
	} else if (ev->type == VBI_EVENT_RATING) {
		cc->curr_data_mask |= 1 << AM_CC_CAPTION_XDS;

		if (cc->cpara.rating_cb)
			cc->cpara.rating_cb(cc, &ev->ev.prog_info->rating);
	}
}

static void dump_cc_data(uint8_t *buff, int size)
{
	int i;
	char buf[4096];

	if (size > 1024)
		size = 1024;
	for (i=0; i<size; i++)
	{
		sprintf(buf+i*3, "%02x ", buff[i]);
	}

	AM_DEBUG(AM_DEBUG_LEVEL, "dump_cc_data len: %d data:%s", size, buf);
}

static void am_cc_check_flash(AM_CC_Decoder_t *cc)
{
	if (cc->spara.user_options.bg_opacity == AM_CC_OPACITY_FLASH ||
		cc->spara.user_options.fg_opacity == AM_CC_OPACITY_FLASH)
	{
		if (cc->flash_stat == FLASH_NONE)
			cc->flash_stat = FLASH_SHOW;
		else if (cc->flash_stat == FLASH_SHOW)
			cc->flash_stat = FLASH_HIDE;
		else if (cc->flash_stat == FLASH_HIDE)
			cc->flash_stat = FLASH_SHOW;
		cc->timeout = CC_FLASH_PERIOD;
		cc->render_flag = AM_TRUE;
	}
	else if (cc->flash_stat != FLASH_NONE)
	{
		cc->flash_stat = FLASH_NONE;
		cc->timeout = CC_CLEAR_TIME;
	}
}

static void am_cc_clear(AM_CC_Decoder_t *cc)
{
	AM_CC_DrawPara_t draw_para;
	AM_CC_JsonChain_t *node, *node_to_clear, *json_chain_head;
	struct vbi_page sub_page;

	/* Clear all node */
	json_chain_head = cc->json_chain_head;
	node = json_chain_head->json_chain_next;
	while (node != json_chain_head)
	{
		node_to_clear = node;
		node = node->json_chain_next;
		free(node_to_clear->buffer);
		free(node_to_clear);
	}
	json_chain_head->json_chain_next = json_chain_head;
	json_chain_head->json_chain_prior = json_chain_head;

	if (cc->cpara.json_buffer)
		tvcc_empty_json(cc->vbi_pgno, cc->cpara.json_buffer, 8096);
	if (cc->cpara.json_update)
		cc->cpara.json_update(cc);
	AM_DEBUG(AM_DEBUG_LEVEL, "am_cc_clear");
}

/*
 * If has cc data to render, return 1
 * else return 0
 */
static int am_cc_render(AM_CC_Decoder_t *cc)
{
	AM_CC_DrawPara_t draw_para;
	struct vbi_page sub_pages[8];
	int sub_pg_cnt, i;
	int status = 0;
	static int count = 0;
	AM_CC_JsonChain_t* node, *node_reset, *json_chain_head;
	struct timespec now;
	int has_data_to_render;
	int32_t decode_time_gap = 0;
	int ret;

	if (cc->hide)
		return 0;
	has_data_to_render = 0;
	/*Flashing?*/
	//am_cc_check_flash(cc);

	if (am_cc_calc_caption_size(&draw_para.caption_width,
		&draw_para.caption_height) != AM_SUCCESS)
		return 0;
	//if (cc->cpara.draw_begin)
	//	cc->cpara.draw_begin(cc, &draw_para);
	json_chain_head = cc->json_chain_head;
	AM_TIME_GetTimeSpec(&now);
	/* Fetch one json from chain */
	node = json_chain_head->json_chain_next;

	while (node != json_chain_head)
	{
		//100 indicates analog which does not need to use pts
		if (cc->vfmt != 100)
		{
			decode_time_gap = (int32_t)(node->pts - cc->video_pts);
#if 0
			if (decode_time_gap<-200000 || decode_time_gap > 400000)
			{
				AM_DEBUG(4, "render_thread pts gap too large, am_cc_clear, node 0x%x video 0x%x diff %d", node->pts,
					cc->video_pts, decode_time_gap);
				has_data_to_render = 0;
				goto NEXT_NODE;
			}
			else
#endif
			//do sync in 5secs time gap
			if (decode_time_gap > 0 && decode_time_gap < 450000)
			{
				AM_DEBUG(AM_DEBUG_LEVEL, "render_thread pts gap large than 0, value %d", decode_time_gap);
				has_data_to_render = 0;
				node = node->json_chain_next;
				continue;
			}
			AM_DEBUG(AM_DEBUG_LEVEL, "render_thread pts in range, node->pts %x videopts %x", node->pts, cc->video_pts);
		}

		if (cc->cpara.json_buffer)
		{
			memcpy(cc->cpara.json_buffer, node->buffer, JSON_STRING_LENGTH);
			has_data_to_render = 1;
		}
		AM_DEBUG(AM_DEBUG_LEVEL, "json--> %s", node->buffer);
		if (cc->cpara.json_update && has_data_to_render)
			cc->cpara.json_update(cc);

		//AM_DEBUG(1, "Render json: %s, %d", node->buffer, count);
		//count++;
CLEAN_NODE:
		node_reset = node;
		node->json_chain_prior->json_chain_next = node->json_chain_next;
		node->json_chain_next->json_chain_prior = node->json_chain_prior;
		node = node->json_chain_next;
		json_chain_head->count--;
		if (node_reset->buffer)
			free(node_reset->buffer);
		if (node_reset)
			free(node_reset);
		//if (has_data_to_render == 1)
		//	break;
	}


	//if (cc->cpara.draw_end)
	//	cc->cpara.draw_end(cc, &draw_para);
	AM_DEBUG(AM_DEBUG_LEVEL, "render_thread render exit, has_data %d chain_left %d", has_data_to_render,json_chain_head->count);
	return has_data_to_render;
}

static void am_cc_handle_event(AM_CC_Decoder_t *cc, int evt)
{
	switch (evt)
	{
		case AM_CC_EVT_SET_USER_OPTIONS:
			/*force redraw*/
			cc->render_flag = AM_TRUE;
			break;
		default:
			break;
	}
}

static void am_cc_set_tv(const uint8_t *buf, unsigned int n_bytes)
{
	int cc_flag;
	int cc_count;
	int i;

	cc_flag = buf[1] & 0x40;
	if (!cc_flag)
	{
		AM_DEBUG(0, "cc_flag is invalid, %d", n_bytes);
		return;
	}
	cc_count = buf[1] & 0x1f;
	for(i = 0; i < cc_count; ++i)
	{
		unsigned int b0;
		unsigned int cc_valid;
		unsigned int cc_type;
		unsigned char cc_data1;
		unsigned char cc_data2;

		b0 = buf[3 + i * 3];
		cc_valid = b0 & 4;
		cc_type = b0 & 3;
		cc_data1 = buf[4 + i * 3];
		cc_data2 = buf[5 + i * 3];

		if (cc_type == 0 || cc_type == 1)//NTSC pair
		{
			struct vout_CCparam_s cc_param;
            cc_param.type = cc_type;
			cc_param.data1 = cc_data1;
			cc_param.data2 = cc_data2;
			//AM_DEBUG(0, "cc_type:%#x, write cc data: %#x, %#x", cc_type, cc_data1, cc_data2);
			if (ioctl(vout_fd, VOUT_IOC_CC_DATA, &cc_param)== -1)
	            AM_DEBUG(1, "ioctl VOUT_IOC_CC_DATA failed, error:%s", strerror(errno));

			if (!cc_valid || i >= 3)
				break;
		}
	}
}

static void solve_vbi_data (AM_CC_Decoder_t *cc, struct vbi_data_s *vbi)
{
	int line = vbi->line_num;

	if (cc == NULL || vbi == NULL)
		return;

	/*if (line != 21)
		return;

	if (vbi->field_id == VBI_FIELD_2)
		line = 284;*/

	AM_DEBUG(AM_DEBUG_LEVEL, "VBI solve_vbi_data line %d: %02x %02x", line, vbi->b[0], vbi->b[1]);
	vbi_decode_caption(cc->decoder.vbi, line, vbi->b);
}

/**\brief VBI data thread.*/
static void *am_vbi_data_thread(void *arg)
{

	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)arg;
	struct vbi_data_s  vbi[50];
	int fd;
	int type = VBI_TYPE_USCC;
	int now, last;
	int last_data_mask = 0;
	int timeout = cc->cpara.data_timeout * 2;

	fd = open(VBI_DEV_FILE, O_RDWR);
	if (fd == -1) {
		AM_DEBUG(1, "cannot open \"%s\"", VBI_DEV_FILE);
		return NULL;
	}

	if (ioctl(fd, VBI_IOC_SET_TYPE, &type )== -1)
		AM_DEBUG(0, "VBI_IOC_SET_TYPE error:%s", strerror(errno));

	if (ioctl(fd, VBI_IOC_START) == -1)
		AM_DEBUG(0, "VBI_IOC_START error:%s", strerror(errno));

	AM_TIME_GetClock(&last);

	while (cc->running) {
		struct pollfd pfd;
		int           ret;

		pfd.fd     = fd;
		pfd.events = POLLIN;

		ret = poll(&pfd, 1, CC_POLL_TIMEOUT);

		if (cc->running && (ret > 0) && (pfd.events & POLLIN)) {
			struct vbi_data_s *pd;

			ret = read(fd, vbi, sizeof(vbi));
			pd  = vbi;
			//AM_DEBUG(0, "am_vbi_data_thread running read data == %d",ret);

			if (ret >= (int)sizeof(struct vbi_data_s)) {
				while (ret >= (int)sizeof(struct vbi_data_s)) {
					solve_vbi_data(cc, pd);

					pd ++;
					ret -= sizeof(struct vbi_data_s);
				}
			} else {
				cc->process_update_flag = 1;
				vbi_refresh_cc(cc->decoder.vbi);
				cc->process_update_flag = 0;
			}
		} else {
			cc->process_update_flag = 1;
			vbi_refresh_cc(cc->decoder.vbi);
			cc->process_update_flag = 0;
		}

		AM_TIME_GetClock(&now);

		if (cc->curr_data_mask) {
			cc->curr_data_mask |= (1 << AM_CC_CAPTION_DATA);
		}

		if (cc->curr_data_mask & (1 << AM_CC_CAPTION_DATA)) {
			if (!(last_data_mask & (1 << AM_CC_CAPTION_DATA))) {
				last_data_mask |= (1 << AM_CC_CAPTION_DATA);
				if (cc->cpara.data_cb) {
					cc->cpara.data_cb(cc, last_data_mask);
				}
			}
		}

		if (now - last >= timeout) {
			if (last_data_mask != cc->curr_data_mask) {
				if (cc->cpara.data_cb) {
					cc->cpara.data_cb(cc, cc->curr_data_mask);
				}
				//AM_DEBUG(0, "CC data mask 0x%x -> 0x%x", last_data_mask, cc->curr_data_mask);
				last_data_mask = cc->curr_data_mask;
			}
			cc->curr_data_mask = 0;
			last = now;
			timeout = cc->cpara.data_timeout;
		}
		cc->process_update_flag = 1;
		vbi_refresh_cc(cc->decoder.vbi);
		cc->process_update_flag = 0;
	}

	AM_DEBUG(0, "am_vbi_data_thread exit");
	ioctl(fd, VBI_IOC_STOP);
	close(fd);
	return NULL;
}

/**\brief CC data thread*/
static void *am_cc_data_thread(void *arg)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)arg;
	uint8_t cc_buffer[5*1024];/*In fact, 99 is enough*/
	uint8_t *cc_data;
	int cnt, left, fd, cc_data_cnt;
	struct pollfd fds;
	int last_pkg_idx = -1, pkg_idx;
	int ud_dev_no = 0;
	AM_USERDATA_OpenPara_t para;
	int now, last, last_switch;
	int last_data_mask = 0;
	int last_switch_mask = 0;
	uint32_t *cc_pts;

	char display_buffer[8192];
	int i;

	AM_DEBUG(1, "CC data thread start.");

	memset(&para, 0, sizeof(para));
	para.vfmt = cc->vfmt;
	/* Start the cc data */
	if (AM_USERDATA_Open(ud_dev_no, &para) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Cannot open userdata device %d", ud_dev_no);
		return NULL;
	}

	AM_TIME_GetClock(&last);
	last_switch = last;

	while (cc->running)
	{
		cc_data_cnt = AM_USERDATA_Read(ud_dev_no, cc_buffer, sizeof(cc_buffer), CC_POLL_TIMEOUT);
		/* There is a poll action in userdata read */
		if (!cc->running)
			break;
		cc_data = cc_buffer + 4;
		cc_data_cnt -= 4;
		dump_cc_data(cc_buffer, cc_data_cnt);
		if (cc_data_cnt > 8 &&
			cc_data[0] == 0x47 &&
			cc_data[1] == 0x41 &&
			cc_data[2] == 0x39 &&
			cc_data[3] == 0x34)
		{
			cc_pts = cc_buffer;
			cc->decoder_cc_pts = *cc_pts;
			AM_DEBUG(AM_DEBUG_LEVEL, "cc_data_thread mpeg cc_count %d pts %x", cc_data_cnt,*cc_pts);
			//dump_cc_data(cc_data, cc_data_cnt);

			if (cc_data[4] != 0x03 /* 0x03 indicates cc_data */)
			{
				AM_DEBUG(AM_DEBUG_LEVEL, "Unprocessed user_data_type_code 0x%02x, we only expect 0x03", cc_data[4]);
				continue;
			}
			if (vout_fd != -1)
				am_cc_set_tv(cc_data+4, cc_data_cnt-4);
			/*decode this cc data*/
			tvcc_decode_data(&cc->decoder, 0, cc_data+4, cc_data_cnt-4);
		}
		else if (cc_data_cnt > 4 &&
			cc_data[0] == 0xb5 &&
			cc_data[1] == 0x00 &&
			cc_data[2] == 0x2f )
		{
			//dump_cc_data(cc_data+4, cc_data_cnt-4);
			//directv format
			if (cc_data[3] != 0x03 /* 0x03 indicates cc_data */)
			{
				AM_DEBUG(1, "Unprocessed user_data_type_code 0x%02x, we only expect 0x03", cc_data[3]);
				continue;
			}
			cc_data[4] = cc_data[3];// use user_data_type_code in place of user_data_code_length  for extract code
			if (vout_fd != -1)
				am_cc_set_tv(cc_data+4, cc_data_cnt-4);

			/*decode this cc data*/
			tvcc_decode_data(&cc->decoder, 0, cc_data+4, cc_data_cnt-4);
		}
		{
			cc->process_update_flag = 1;
			update_service_status(&cc->decoder);
			vbi_refresh_cc(cc->decoder.vbi);
			cc->process_update_flag = 0;
		}

		AM_TIME_GetClock(&now);

		if (cc->curr_data_mask) {
			cc->curr_data_mask |= (1 << AM_CC_CAPTION_DATA);
		}

		if (cc->curr_data_mask & (1 << AM_CC_CAPTION_DATA)) {
			if (!(last_data_mask & (1 << AM_CC_CAPTION_DATA))) {
				last_data_mask |= (1 << AM_CC_CAPTION_DATA);
				if (cc->cpara.data_cb) {
					cc->cpara.data_cb(cc, last_data_mask);
				}
			}
		}

		if (now - last >= cc->cpara.data_timeout) {
			if (last_data_mask != cc->curr_data_mask) {
				if (cc->cpara.data_cb) {
					cc->cpara.data_cb(cc, cc->curr_data_mask);
				}
				AM_DEBUG(AM_DEBUG_LEVEL, "CC data mask 0x%x -> 0x%x", last_data_mask, cc->curr_data_mask);
				last_data_mask = cc->curr_data_mask;
			}
			cc->curr_data_mask = 0;
			last = now;
		}

		if (now - last_switch >= cc->cpara.switch_timeout) {
			if (last_switch_mask != cc->curr_switch_mask) {
				if (!(cc->curr_switch_mask & (1 << cc->vbi_pgno))) {
					if ((cc->vbi_pgno == cc->spara.caption1) && (cc->spara.caption2 != AM_CC_CAPTION_NONE)) {
						AM_DEBUG(0, "CC switch %d -> %d", cc->vbi_pgno, cc->spara.caption2);
						cc->vbi_pgno = cc->spara.caption2;
					} else if ((cc->vbi_pgno == cc->spara.caption2) && (cc->spara.caption1 != AM_CC_CAPTION_NONE)) {
						AM_DEBUG(0, "CC switch %d -> %d", cc->vbi_pgno, cc->spara.caption1);
						cc->vbi_pgno = cc->spara.caption1;
					}
				}
				last_switch_mask = cc->curr_switch_mask;
			}
			cc->curr_switch_mask = 0;
			last_switch = now;
		}
	}

	/*Stop the cc data*/
	AM_USERDATA_Close(ud_dev_no);

	AM_DEBUG(1, "CC data thread exit now");
	return NULL;
}

/**\brief CC rendering thread*/
static void *am_cc_render_thread(void *arg)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)arg;
	struct timespec ts;
	int cnt, timeout, ret;
	int last, now;
	int nodata = 0;
	char vpts_buf[16];
	uint32_t vpts;
	int32_t pts_gap_cc_video;
	int32_t vpts_gap;
	AM_CC_JsonChain_t *node;

	/* Get fps of video */
	int fps;
#if 0
	int fd = open("/sys/class/video/fps_info", O_RDONLY);
	char fps_info[32];
	char* fps_start, *fps_end;
	read(fd, fps_info, 32);
	fps_start = strstr(fps_info, ":") + 1;
	fps_end = strstr(fps_info, " ");
	fps_end[0] = 0;
	fps = strtoul(fps_start, NULL, 16);
#endif
	AM_TIME_GetClock(&last);

	pthread_mutex_lock(&cc->lock);

	//am_cc_check_flash(cc);

	/* timeout must be calculate from fps, ie 60fps for 16ms */
#if 0
	if (fps != 0)
		timeout = 1000/fps;
	else
		timeout = 16;
	if (timeout == 0)
		timeout = 16;
#endif
	timeout = 10;

	while (cc->running)
	{
		// Update video pts
		node = cc->json_chain_head->json_chain_next;
		if (node == cc->json_chain_head)
			node = NULL;

		vpts = am_cc_get_video_pts();

		// If has cc data in chain, set gap time to timeout
		if (node)
		{
			pts_gap_cc_video = (int32_t)(node->pts - vpts);
			if (pts_gap_cc_video <= 0)
			{
				AM_DEBUG(AM_DEBUG_LEVEL, "pts gap less than 0, node pts %x vpts %x, gap %x", node->pts, vpts, pts_gap_cc_video);
				timeout = 0;
			}
			else
			{
				timeout = pts_gap_cc_video*1000/90000;
				AM_DEBUG(AM_DEBUG_LEVEL, "render_thread timeout node_pts %x vpts %x gap %d calculate %d", node->pts, vpts, pts_gap_cc_video, timeout);
				// Set timeout to 1 second If pts gap is more than 1 second.
				// We need to judge if video is continuous
				timeout = (timeout>10)?10:timeout;
			}
		}
		else
		{
			AM_DEBUG(AM_DEBUG_LEVEL, "render_thread no node in chain, timeout set to 1000");
			timeout = 10;
		}

		AM_TIME_GetTimeSpecTimeout(timeout, &ts);
		if (cc->running)
			pthread_cond_timedwait(&cc->cond, &cc->lock, &ts);
		else
			break;

		vpts = am_cc_get_video_pts();
		cc->video_pts = vpts;

		// Judge if video pts gap is in valid range
		if (node)
		{
			vpts_gap = (int32_t)(node->pts - cc->video_pts);
			AM_DEBUG(AM_DEBUG_LEVEL, "render_thread after timeout node_pts %x vpts %x gap %d calculate %d",
				node->pts, vpts, vpts_gap, timeout);
		}
		//If gap time is large than 5 secs, clean cc
#if 0
		if (abs(vpts_gap) > 500000)
		{
			AM_DEBUG(0, "Video pts gap too large, clean cc, gap: %d", vpts_gap);
			am_cc_clear(cc);
			continue;
		}
#endif

#if 0
		if (cc->need_clear && cc->flash_stat == FLASH_NONE)
		{
			am_cc_clear(cc);
			cc->need_clear = AM_FALSE;
		}
#endif
		if (cc->evt >= 0)
		{
			am_cc_handle_event(cc, cc->evt);
			cc->evt = 0;
		}

		//if (!nodata)
		am_cc_render(cc);

		AM_TIME_GetClock(&now);
		if (cc->curr_data_mask & (1 << cc->vbi_pgno)) {
			nodata = 0;
			last   = now;
		} else if ((now - last) > 14000) {
			last = now;
			AM_DEBUG(0, "cc render thread: No data now.");
			if ((cc->vbi_pgno < AM_CC_CAPTION_TEXT1) || (cc->vbi_pgno > AM_CC_CAPTION_TEXT4)) {
				am_cc_clear(cc);
				//if (cc->vbi_pgno < AM_CC_CAPTION_TEXT1)
				//	vbi_caption_reset(cc->decoder.vbi);
				nodata = 1;
			}
		}
	}
	am_cc_clear(cc);

	pthread_mutex_unlock(&cc->lock);

	//close(fd);
	AM_DEBUG(1, "CC rendering thread exit now");
	return NULL;
}

 /****************************************************************************
 * API functions
 ****************************************************************************/
/**\brief 创建CC
 * \param [in] para 创建参数
 * \param [out] handle 返回句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cc.h)
 */
AM_ErrorCode_t AM_CC_Create(AM_CC_CreatePara_t *para, AM_CC_Handle_t *handle)
{
	AM_CC_Decoder_t *cc;

	if (para == NULL || handle == NULL)
		return AM_CC_ERR_INVALID_PARAM;

	cc = (AM_CC_Decoder_t*)malloc(sizeof(AM_CC_Decoder_t));
	if (cc == NULL)
		return AM_CC_ERR_NO_MEM;
	if (para->bypass_cc_enable)
	{
		vout_fd= open("/dev/tv", O_RDWR);
		if (vout_fd == -1)
		{
			AM_DEBUG(0, "open vdin error");
		}
	}

	memset(cc, 0, sizeof(AM_CC_Decoder_t));
	cc->json_chain_head = (AM_CC_JsonChain_t*) calloc (sizeof(AM_CC_JsonChain_t), 1);
	cc->json_chain_head->json_chain_next = cc->json_chain_head;
	cc->json_chain_head->json_chain_prior = cc->json_chain_head;
	cc->json_chain_head->count = 0;

	cc->decoder_param = para->decoder_param;
	strncpy(cc->lang, para->lang, 10);

	/* init the tv cc decoder */
	tvcc_init(&cc->decoder, para->lang, 10, para->decoder_param);

	if (cc->decoder.vbi == NULL)
		return AM_CC_ERR_LIBZVBI;

	vbi_event_handler_register(cc->decoder.vbi,
			VBI_EVENT_CAPTION|VBI_EVENT_ASPECT
			|VBI_EVENT_PROG_INFO|VBI_EVENT_NETWORK
			|VBI_EVENT_RATING,
			am_cc_vbi_event_handler,
			cc);

	pthread_mutex_init(&cc->lock, NULL);
	pthread_cond_init(&cc->cond, NULL);

	cc->cpara = *para;

	*handle = cc;
	AM_SigHandlerInit();

	return AM_SUCCESS;
}

/**\brief 销毁CC
 * \param [out] handle CC句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cc.h)
 */
AM_ErrorCode_t AM_CC_Destroy(AM_CC_Handle_t handle)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;

	if (vout_fd != -1)
		close(vout_fd);

	if (cc == NULL)
		return AM_CC_ERR_INVALID_PARAM;

	AM_CC_Stop(handle);

	tvcc_destroy(&cc->decoder);
	pthread_mutex_destroy(&cc->lock);
	pthread_cond_destroy(&cc->cond);

	if (cc->json_chain_head)
		free(cc->json_chain_head);
	free(cc);

	AM_DEBUG(0, "am_cc_destroy ok");
	return AM_SUCCESS;
}

/**
 * \brief Show close caption.
 * \param handle Close caption parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CC_Show(AM_CC_Handle_t handle)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;

	if (cc == NULL)
		return AM_CC_ERR_INVALID_PARAM;

	cc->hide = AM_FALSE;

	return AM_SUCCESS;
}

/**
 * \brief Hide close caption.
 * \param handle Close caption parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CC_Hide(AM_CC_Handle_t handle)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;

	if (cc == NULL)
		return AM_CC_ERR_INVALID_PARAM;

	cc->hide = AM_TRUE;

	return AM_SUCCESS;
}

/**\brief 开始CC数据接收处理
 * \param handle CC handle
  * \param [in] para 启动参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cc.h)
 */
AM_ErrorCode_t AM_CC_Start(AM_CC_Handle_t handle, AM_CC_StartPara_t *para)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;
	int rc, ret = AM_SUCCESS;

	if (cc == NULL || para == NULL)
		return AM_CC_ERR_INVALID_PARAM;

	pthread_mutex_lock(&cc->lock);
	if (cc->running)
	{
		ret = AM_CC_ERR_BUSY;
		goto start_done;
	}

	if (para->caption1 <= AM_CC_CAPTION_DEFAULT ||
		para->caption1 >= AM_CC_CAPTION_MAX)
		para->caption1 = AM_CC_CAPTION_CC1;

	AM_DEBUG(1, "AM_CC_Start vfmt %d para->caption1=%d para->caption2=%d",
		para->vfmt, para->caption1, para->caption2);

	cc->vfmt = para->vfmt;
	cc->evt = -1;
	cc->spara = *para;
	cc->vbi_pgno = para->caption1;
	cc->running = AM_TRUE;

	cc->curr_switch_mask = 0;
	cc->curr_data_mask   = 0;
	cc->video_pts = 0;

	/* start the rendering thread */
	rc = pthread_create(&cc->render_thread, NULL, am_cc_render_thread, (void*)cc);
	if (rc)
	{
		cc->running = AM_FALSE;
		AM_DEBUG(0, "%s:%s", __func__, strerror(rc));
		ret = AM_CC_ERR_SYS;
	}
	else
	{
		/* start the data source thread */
		if (cc->cpara.input == AM_CC_INPUT_VBI) {
			rc = pthread_create(&cc->data_thread, NULL, am_vbi_data_thread, (void*)cc);
		} else {
			rc = pthread_create(&cc->data_thread, NULL, am_cc_data_thread, (void*)cc);
		}

		if (rc)
		{
			cc->running = AM_FALSE;
			pthread_join(cc->render_thread, NULL);
			AM_DEBUG(0, "%s:%s", __func__, strerror(rc));
			ret = AM_CC_ERR_SYS;
		}
	}

start_done:
	pthread_mutex_unlock(&cc->lock);
	return ret;
}

/**\brief 停止CC处理
 * \param handle CC handle
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cc.h)
 */
AM_ErrorCode_t AM_CC_Stop(AM_CC_Handle_t handle)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;
	int ret = AM_SUCCESS;
	AM_Bool_t join = AM_FALSE;

	if (cc == NULL)
		return AM_CC_ERR_INVALID_PARAM;

	AM_DEBUG(0, "am_cc_stop wait lock");
	pthread_mutex_lock(&cc->lock);
	AM_DEBUG(0, "am_cc_stop enter lock");
	if (cc->running)
	{
		cc->running = AM_FALSE;
		join = AM_TRUE;
	}
	pthread_mutex_unlock(&cc->lock);

	pthread_cond_broadcast(&cc->cond);
	pthread_kill(cc->data_thread, SIGALRM);

	if (join)
	{
		pthread_join(cc->data_thread, NULL);
		pthread_join(cc->render_thread, NULL);
	}
	AM_DEBUG(0, "am_cc_stop ok");
	return ret;
}

/**\brief 设置CC用户选项，用户选项可以覆盖运营商的设置,这些options由应用保存管理
 * \param handle CC handle
 * \param [in] options 选项集
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cc.h)
 */
AM_ErrorCode_t AM_CC_SetUserOptions(AM_CC_Handle_t handle, AM_CC_UserOptions_t *options)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;

	if (cc == NULL)
		return AM_CC_ERR_INVALID_PARAM;

	pthread_mutex_lock(&cc->lock);

	cc->spara.user_options = *options;
	cc->evt = AM_CC_EVT_SET_USER_OPTIONS;

	pthread_mutex_unlock(&cc->lock);
	//pthread_cond_signal(&cc->cond);

	return AM_SUCCESS;
}

/**\brief 获取用户数据
 * \param handle CC 句柄
 * \return [out] 用户数据
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cc.h)
 */
void *AM_CC_GetUserData(AM_CC_Handle_t handle)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;

	if (cc == NULL)
		return NULL;

	return cc->cpara.user_data;
}

void *AM_Isdb_GetUserData(AM_ISDB_Handle_t handle)
{
	AM_CC_Decoder_t *cc = (AM_CC_Decoder_t*)handle;

	if (cc == NULL)
		return NULL;

	return cc->cpara.user_data;
}



