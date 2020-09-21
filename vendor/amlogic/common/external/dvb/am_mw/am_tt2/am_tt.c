/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
#define AM_DEBUG_LEVEL 1

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#include <libzvbi.h>
#include <pthread.h>

#include <am_time.h>
#include <am_cond.h>
#include <am_tt2.h>
#include <am_debug.h>
#include <am_util.h>
#include <am_misc.h>
#include <sys/ioctl.h>
#include <signal.h>


#include "tvin_vbi.h"

#define AM_TT2_MAX_SLICES (32)
#define AM_TT2_MAX_CACHED_PAGES (200)
#define AM_TT2_ROWS (25)

#define VBI_DEV_FILE "/dev/vbi"
//#define DEBUG_TT

typedef struct AM_TT2_CachedPage_s
{
	int			          count;
	vbi_page	          page;
	uint8_t               drcs_clut[2 + 2 * 4 + 2 * 16];
	int      	          pts;
	int                   page_type;
	struct AM_TT2_CachedPage_s *next;
	struct AM_TT2_CachedPage_s *sub_next;

}AM_TT2_CachedPage_t;

typedef struct nav_link_s
{
	int			          pgno;
	int                   subno;
}nav_link_t;

typedef struct
{
	vbi_decoder          *dec;
	vbi_search           *search;
	AM_TT2_Para_t         para;
	AM_TT_Input_t         input;
	int                   page_no;
	int                   sub_page_no;
	AM_Bool_t             disp_update;
	AM_Bool_t             running;
	int                   reveal;
	int                   pts;
	int                   lock_subpg;
	int                   lock_subpgno;
	AM_TT2_CachedPage_t    *curr_page;
	pthread_mutex_t       lock;
	pthread_cond_t        cond;
	pthread_t             thread;
	pthread_t             vbi_tid;
	pthread_mutex_t       vbi_lock;
	char                  time[8];
	AM_Bool_t             transparent_background;
	AM_TT2_CachedPage_t   *cached_pages;
	AM_TT2_CachedPage_t   *cached_tail;
	AM_TT2_CachedPage_t   *display_page;
	AM_TT2_CachedPage_t   * subtitle_head;
	AM_TT2_CachedPage_t   last_display_page;
	AM_TT2_DisplayMode_t   display_mode;
	nav_link_t            nav_link[6];
	int region_id;
}AM_TT2_Parser_t;

enum systems {
	SYSTEM_525 = 0,
	SYSTEM_625
};

static int tt2_get_pts(const char *pts_file, int base)
{
	char buf[32];
	AM_ErrorCode_t ret;
	uint32_t v;

	ret = AM_FileRead(pts_file, buf, sizeof(buf));
	if (!ret) {
		v = strtoul(buf, 0, base);
	} else {
		v = 0;
	}

	return v;
}

static void tt2_add_cached_page(AM_TT2_Parser_t *parser, vbi_page *vp, int page_type)
{
	AM_TT2_CachedPage_t *target;

	target = parser->cached_pages;

	while (target)
	{
		if ((vbi_bcd2dec(target->page.pgno) == vbi_bcd2dec(vp->pgno)) && (target->page.subno == vp->subno))
		{
#ifdef DEBUG_TT
			AM_DEBUG(0, "pg %d sub %d found", vbi_bcd2dec(vp->pgno), vp->subno);
#endif
			break;
		}
		else
			target = target->next;
	}

	if (!target)
	{
#ifdef DEBUG_TT
		AM_DEBUG(0, "Alloc cached_pages");
#endif
		target = (AM_TT2_CachedPage_t *)malloc(sizeof(AM_TT2_CachedPage_t));
		if (target == NULL)
		{
			AM_DEBUG(1, "Cannot alloc memory for new cached page");
			return;
		}
		memset(target, 0, sizeof(AM_TT2_CachedPage_t));
		target->next = parser->cached_pages;
		parser->cached_pages = target;
		if (page_type & 0x8000)
		{
#ifdef DEBUG_TT
			AM_DEBUG(0, "Subtitle page added pgno %x", vp->pgno);
#endif

			target->sub_next = parser->subtitle_head;
			parser->subtitle_head = target;
		}
		else
			target->sub_next = NULL;
	}
	else
	{
#ifdef DEBUG_TT
		AM_DEBUG(0, "cached_pages found, reuse pgno %d sub %d", vbi_bcd2dec(vp->pgno), vp->subno);
#endif
		vbi_unref_page(&target->page);
	}

	// memcpy(&tmp->page, vp, sizeof(AM_TT2_CachedPage_t));
	target->page = *vp;
	target->page_type = page_type;
	if (vp->drcs_clut) {
		memcpy(target->drcs_clut, vp->drcs_clut, sizeof(target->drcs_clut));
		target->page.drcs_clut = &target->drcs_clut;
	}

	target->pts = tt2_get_pts("/sys/class/stb/video_pts", 10);
	// AM_DEBUG(2, "Cache page, pts 0x%x, total cache %d pages", tmp->pts,
	// 	parser->cached_tail->count - parser->cached_pages->count);
}

static void tt2_step_cached_page(AM_TT2_Parser_t *parser)
{
	if (parser->cached_pages != NULL)
	{
		AM_TT2_CachedPage_t *tmp = parser->cached_pages;
		parser->cached_pages = tmp->next;
		vbi_unref_page(&tmp->page);
		free(tmp);
	}

	if (parser->cached_pages == NULL)
		parser->cached_tail = NULL;
}

static AM_TT2_CachedPage_t* find_cached_page(AM_TT2_Parser_t *parser, int pgno, int subno, int sub_mask)
{
	AM_TT2_CachedPage_t *target_page;

	if (!parser)
		return NULL;
	if (!parser->cached_pages)
		return NULL;

	target_page = parser->cached_pages;
	while (target_page)
	{
		if (target_page->page.pgno == pgno &&
				(subno & sub_mask) == (target_page->page.subno & sub_mask))
			break;
		target_page = target_page->next;
	}

	return target_page;
}

static void tt_lofp_to_line(unsigned int *field, unsigned int *field_line, unsigned int *frame_line, unsigned int lofp, enum systems system)
{
	unsigned int line_offset;

	*field = !(lofp & (1 << 5));
	line_offset = lofp & 31;

	if (line_offset > 0)
	{
		static const unsigned int field_start [2][2] = {
			{ 0, 263 },
			{ 0, 313 },
		};
		*field_line = line_offset;
		*frame_line = field_start[system][*field] + line_offset;
	}
	else
	{
		*field_line = 0;
		*frame_line = 0;
	}
}

static int get_subs(AM_TT2_Parser_t *parser, int pgno, char* subs)
{
	AM_TT2_CachedPage_t     *target_page;
	int                      sub_cnt = 0;
	int i, j, tmp;

	if (!subs)
		return 0;

	target_page = parser->cached_pages;
	while (target_page)
	{
		if (target_page->page.pgno == pgno)
		{
			subs[sub_cnt++] = target_page->page.subno;
		}
		target_page = target_page->next;
	}
	for (i=0; i<sub_cnt; i++)
	{
		for (j=0; j<sub_cnt-i; j++)
		{
			if (subs[j] > subs[j+1])
			{
				tmp = subs[j];
				subs[j] = subs[j+1];
				subs[j+1] = tmp;
			}
		}
	}
	return sub_cnt;
}

static AM_TT2_CachedPage_t* tt2_check(AM_TT2_Parser_t *parser)
{
	AM_TT2_CachedPage_t     *target_page;
	int                                   target_pgno = 0;
	int                                   target_subno = 0;

	//Get pgno and subno
	target_pgno = parser->page_no;

	if (parser->lock_subpg)
		target_subno = parser->lock_subpgno;
	else
		target_subno = parser->sub_page_no;

	target_page = parser->cached_pages;

	if (target_page != NULL)
	{
		while (target_page != NULL)
		{
			if (vbi_bcd2dec(target_page->page.pgno) == target_pgno &&
				(target_subno == target_page->page.subno  || target_subno == AM_TT2_ANY_SUBNO))
			{
#if 0
				AM_DEBUG(0, "tt2_check found page");
#endif
				break;
			}
			else
			{
#if 0
				AM_DEBUG(0, "tt2_check finding curr_pg %d curr_sub %d; pg %d, sub %d",
						 vbi_bcd2dec(target_page->page.pgno),
						 target_page->page.subno,
						 target_pgno,
						 target_subno);
#endif
				target_page = target_page->next;
			}
		}

		if (target_page == NULL)
		{
			AM_DEBUG(0, "Cannot find target page %d subno %d", target_pgno, target_subno);
			target_page = &parser->last_display_page;
		}
		else
		{
			parser->last_display_page = *target_page;
		}
	}
	return target_page;
}

int tt2_check_pgno(AM_TT2_Parser_t *parser, int pgno)
{
	AM_TT2_CachedPage_t* target_page;
	int found_pg = 0;

	target_page = parser->cached_pages;
	while (target_page)
	{
		if (target_page->page.pgno == pgno)
		{
			found_pg = 1;
			break;
		}
		target_page = target_page->next;
	}
	if (found_pg)
		return pgno;
	else
		return 0;
}

static void tt2_draw(AM_TT2_Parser_t *parser, vbi_page* page, char* subs, int sub_cnt, int page_type,
int flash_switch, int nav0, int nav1, int nav2, int nav3)
{
	int is_subtitle;

	if (parser->para.draw_begin)
		parser->para.draw_begin(parser);

	is_subtitle = page_type & 0xC000;

	if (*(parser->para.bitmap))
	{
		vbi_draw_vt_page_region(page,
							VBI_PIXFMT_RGBA32_LE,
							*(parser->para.bitmap),
							parser->para.pitch,
							0,
							0,
							page->columns,
							page->rows,
							parser->reveal, //Reveal
							flash_switch,
							is_subtitle,
							parser->display_mode,
							parser->transparent_background,
							parser->lock_subpg,
							parser->time,
							0);
	}

	if (parser->para.draw_end)
		parser->para.draw_end(parser,
							  page_type,
							  vbi_bcd2dec(page->pgno),
							  subs,
							  sub_cnt,
							  vbi_bcd2dec(nav0),
							  vbi_bcd2dec(nav1),
							  vbi_bcd2dec(nav2),
							  vbi_bcd2dec(nav3),
							  page->subno);
}

static void tt2_cache_page(AM_TT2_Parser_t *parser, int pgno, int sub_pgno)
{
	vbi_page page;
	AM_Bool_t cached;
	int page_type;

	cached = vbi_fetch_vt_page(parser->dec, &page,
		pgno, sub_pgno,
		VBI_WST_LEVEL_2p5, AM_TT2_ROWS, AM_TRUE, &page_type);
	if(!cached)
	{
		AM_DEBUG(0, "vbi_fetch_vt_page cache null pg: %x %d", pgno, sub_pgno);
		return;
	}
	if (vbi_dec2bcd(parser->page_no) == pgno && parser->lock_subpg == 0)
		parser->sub_page_no      = page.subno;

	tt2_add_cached_page(parser, &page, page_type);
}

static void* tt2_thread(void *arg)
{
	AM_TT2_Parser_t               *parser = (AM_TT2_Parser_t*)arg;
	AM_TT2_CachedPage_t        *target_page;
	AM_TT2_CachedPage_t        draw_page = {0};
	char subs[16];
	int                                     sub_cnt;
	int                                     timeout = 60;
	struct timespec                 ts;
	int                                   clock_for_flash = 0;
	int                                   curr_clock;
	int                                    flash_switch = 0;
	int                                    new_page_to_draw = 0;
	vbi_page                            page;
	int                                     cached;
	int                                     page_type;
	int nav0, nav1, nav2, nav3;

	while (parser->running)
	{
		//For flash
		AM_TIME_GetClock(&curr_clock);
		if (curr_clock - clock_for_flash > 800)
		{
			clock_for_flash = curr_clock;
			flash_switch = !flash_switch;
		}

		pthread_mutex_lock(&parser->lock);
		if (parser->running)
		{
			AM_TIME_GetTimeSpecTimeout(timeout, &ts);
			pthread_cond_timedwait(&parser->cond, &parser->lock, &ts);
		}

		if (parser->disp_update) {
			target_page = tt2_check(parser);

			parser->disp_update = AM_FALSE;
			if (target_page)
			{
				vbi_teletext_set_default_region(parser->dec, parser->region_id);
				cached = vbi_fetch_vt_page(parser->dec, &page,
										   target_page->page.pgno, target_page->page.subno,
										   VBI_WST_LEVEL_3p5, AM_TT2_ROWS, AM_TRUE, NULL);
				if (cached)
				{
					page = target_page->page;
					new_page_to_draw = 1;
					sub_cnt = get_subs(parser, target_page->page.pgno, subs);
					page_type = target_page->page_type;
					draw_page = *target_page;
					parser->curr_page = target_page;
				}
				else
				{
					AM_DEBUG(0, "vbi_fetch_vt_page failed");
				}
			}
		}

		if (target_page)
		{
			if (new_page_to_draw)
			{
				nav0 = tt2_check_pgno(parser, page.nav_link[0].pgno);
				nav1 = tt2_check_pgno(parser, page.nav_link[1].pgno);
				nav2 = tt2_check_pgno(parser, page.nav_link[2].pgno);
				nav3 = tt2_check_pgno(parser, page.nav_link[3].pgno);
			}

			pthread_mutex_unlock(&parser->lock);

			if ((parser->input == AM_TT_INPUT_VBI) ||
					((target_page->pts - tt2_get_pts("/sys/class/tsync/pts_pcrscr", 16) <= 0) && parser->input != AM_TT_INPUT_VBI))
			{
				if (new_page_to_draw)
				{
					if (*(parser->para.bitmap))
					{
						AM_DEBUG(0, "tt2_draw page 0x%x type 0x%x", page.pgno, page_type);
						tt2_draw(parser, &page, subs, sub_cnt, page_type, flash_switch, nav0, nav1, nav2, nav3);
					}
					new_page_to_draw = 0;
				}
			}
		}
		else
			pthread_mutex_unlock(&parser->lock);
	}
	AM_DEBUG(0, "Exit tt2_thread");
	return NULL;
}

static void tt2_time_update(vbi_event *ev, void *user_data)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)user_data;

	if (ev->type == VBI_EVENT_TIME)
	{
		memcpy(parser->time, ev->time, 8);
		parser->disp_update = AM_TRUE;

		pthread_cond_signal(&parser->cond);
	}

}

static void tt2_event_handler(vbi_event *ev, void *user_data)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)user_data;
	int pgno, subno;

	if(ev->type != VBI_EVENT_TTX_PAGE)
		return;

	pgno  = ev->ev.ttx_page.pgno;
	subno = ev->ev.ttx_page.subno;

	if (parser->para.notify_contain_data)
		parser->para.notify_contain_data((AM_TT2_Handle_t)parser, vbi_bcd2dec(pgno));

#ifdef DEBUG_TT
	AM_DEBUG(0, "TT event handler: pgno %d, subno %d, parser->page_no %d, parser->sub_page_no %d",
		vbi_bcd2dec(pgno), subno, parser->page_no, parser->sub_page_no	);
	AM_DEBUG(0, "header_update %d, clock_update %d, roll_header %d",
		ev->ev.ttx_page.header_update, ev->ev.ttx_page.clock_update, ev->ev.ttx_page.roll_header);
#endif
	parser->disp_update = AM_TRUE;

	tt2_cache_page(parser, pgno, subno);

	if (parser->para.new_page)
		parser->para.new_page((AM_TT2_Handle_t)parser, pgno, subno);
}

static void *tt2_vbi_data_thread(void *arg)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)arg;
	struct vbi_data_s vbi;
	int fd;
	int type = VBI_TYPE_TT_625B;
	vbi_sliced sliced[AM_TT2_MAX_SLICES];
	vbi_sliced *s = sliced;
	int i;
	int notify_data = 0;
//#define FILE_DEBUG
#ifndef FILE_DEBUG
	AM_DEBUG(0, "no filedebug");
	fd = open(VBI_DEV_FILE, O_RDONLY);
	if (fd == -1) {
		AM_DEBUG(1, "cannot open \"%s\"", VBI_DEV_FILE);
		return NULL;
	}

	if (ioctl(fd, VBI_IOC_SET_TYPE, &type )== -1)
		AM_DEBUG(4, "VBI_IOC_SET_TYPE error:%s", strerror(errno));

	if (ioctl(fd, VBI_IOC_START) == -1)
		AM_DEBUG(4, "VBI_IOC_START error:%s", strerror(errno));
	AM_DEBUG(1, "vbi_thread start");

	while (parser->running) {
		struct pollfd pfd;
		int           ret;

		pfd.fd     = fd;
		pfd.events = POLLIN;

		ret = poll(&pfd, 1, 15);
		// AM_DEBUG(1, "vbi_thread poll! %d", ret);
		if (parser->running && (ret > 0) && (pfd.events & POLLIN)) {
			struct vbi_data_s *pd;

			ret = read(fd, &vbi, sizeof(vbi));
			pd  = &vbi;
			AM_DEBUG(4, "am_vbi_data_thread running read data == %d",ret);

			if (ret >= (int)sizeof(struct vbi_data_s)) {
				while (ret >= (int)sizeof(struct vbi_data_s)) {
					//TODO
					s->line = pd->line_num;
					s->id = VBI_SLICED_TELETEXT_B;
					memcpy(s->data, pd->b, 42);

					pthread_mutex_lock(&parser->lock);
					if (parser->dec)
						vbi_decode(parser->dec, s, 1, 0);

					pthread_mutex_unlock(&parser->lock);

					pd ++;
					ret -= sizeof(struct vbi_data_s);
				}
			}
		}
	}
#else
	fd = open("/data/vbi_data", O_RDONLY);
	AM_DEBUG(0, "open data/vbi_data fd %d %s", fd, strerror(errno));
	if (fd < 0)
		return NULL;
	while (parser->running) {
		int           ret;

		if (parser->running) {
			ret = read(fd, &vbi, sizeof(struct vbi_data_s));
#if 0
			AM_DEBUG(0, "type %d fid %d nb %d ln %d pk1 %x pk2 %x", vbi.vbi_type, vbi.field_id, vbi.nbytes,
				vbi.line_num, vbi.b[0], vbi.b[1]);
#endif
			if (ret >= sizeof(struct vbi_data_s))
			{
				if (vbi.vbi_type != 3)
					continue;
			}
			else
			{
				lseek(fd, 0, SEEK_SET);
				continue;
			}
#if 0
			AM_DEBUG(0, "am_vbi_data_thread running read data == %d",ret);
#endif
			if (ret >= (int)sizeof(struct vbi_data_s)) {
				while (ret >= (int)sizeof(struct vbi_data_s)) {
					s->line = vbi.line_num;
					s->id = VBI_SLICED_TELETEXT_B;
					memcpy(s->data, vbi.b, 42);
					pthread_mutex_lock(&parser->lock);
					vbi_decode(parser->dec, s, 1, 0);
					pthread_mutex_unlock(&parser->lock);
					ret -= sizeof(struct vbi_data_s);
				}
			}
			usleep(10000);
		}
	}
#endif
	AM_DEBUG(0, "am_vbi_data_thread exit");

#ifndef FILE_DEBUG
	ioctl(fd, VBI_IOC_STOP);
#endif
	AM_DEBUG(0, "tt2 vbi data thread exit");
	close(fd);
	return NULL;
}


/**\brief 创建teletext解析句柄
 * \param[out] handle 返回创建的新句柄
 * \param[in] para teletext解析参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_Create(AM_TT2_Handle_t *handle, AM_TT2_Para_t *para)
{
	AM_TT2_Parser_t *parser;

	if (!para || !handle)
	{
		return AM_TT2_ERR_INVALID_PARAM;
	}

	parser = (AM_TT2_Parser_t*)malloc(sizeof(AM_TT2_Parser_t));
	if (!parser)
	{
		return AM_TT2_ERR_NO_MEM;
	}

	memset(parser, 0, sizeof(AM_TT2_Parser_t));

	parser->input = para->input;

	AM_DEBUG(0, "TT2_create input %d", para->input);

	pthread_mutex_init(&parser->lock, NULL);
	pthread_cond_init(&parser->cond, NULL);

	parser->para    = *para;

	*handle = parser;

	AM_SigHandlerInit();

	return AM_SUCCESS;
}

/**\brief 释放teletext解析句柄
 * \param handle 要释放的句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_Destroy(AM_TT2_Handle_t handle)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	if (parser->running != AM_FALSE)
		AM_TT2_Stop(handle);


	/* Free all cached pages */
#if 0
	while (parser->cached_pages != NULL)
	{
		tt2_step_cached_page(parser);
	}
#endif

	pthread_cond_destroy(&parser->cond);
	pthread_mutex_destroy(&parser->lock);

	free(parser);

	return AM_SUCCESS;
}

/**\brief 设定是否为字幕
 * \param handle 要释放的句柄
 * \param subtitle 是否为字幕
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t
AM_TT2_SetSubtitleMode(AM_TT2_Handle_t handle, AM_Bool_t subtitle)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	parser->para.is_subtitle = subtitle;
	parser->disp_update = AM_TRUE;

	pthread_mutex_unlock(&parser->lock);

	pthread_cond_signal(&parser->cond);


	return AM_SUCCESS;
}

#define TT_DISP_NORMAL           0
#define TT_DISP_ONLY_PGNO        1
#define TT_DISP_HIDE_CLOCK       2
#define TT_DISP_MIX_TRANSPARENT  3

AM_ErrorCode_t
AM_TT2_SetRevealMode(AM_TT2_Handle_t handle, int mode)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	if (!parser)
			return AM_TT2_ERR_INVALID_HANDLE;

	pthread_mutex_lock(&parser->lock);
	parser->disp_update = AM_TRUE;
	parser->reveal = (mode > 0) ? 1 : 0;

	pthread_mutex_unlock(&parser->lock);
	pthread_cond_signal(&parser->cond);
	return AM_SUCCESS;
}

AM_ErrorCode_t
AM_TT2_LockSubpg(AM_TT2_Handle_t handle, int lock)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	if (!parser)
			return AM_TT2_ERR_INVALID_HANDLE;

	pthread_mutex_lock(&parser->lock);
	parser->disp_update = AM_TRUE;
	if (lock > 0)
	{
		parser->lock_subpg = lock;
		parser->lock_subpgno = parser->sub_page_no;
	}
	else
	{
		parser->lock_subpg = 0;
		parser->lock_subpgno = 0;
	}
	pthread_mutex_unlock(&parser->lock);
	return AM_SUCCESS;
}

AM_ErrorCode_t
AM_TT2_SetRegion(AM_TT2_Handle_t handle, int region_id)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	AM_DEBUG(0, "AM_TT2_SetRegion region_id %d", region_id);

	if (!parser)
		return AM_TT2_ERR_INVALID_HANDLE;
	else if (!parser->dec)
		return AM_TT2_ERR_INVALID_HANDLE;

	if (parser->running != AM_TRUE)
		return AM_TT2_ERR_NOT_RUN;

	pthread_mutex_lock(&parser->lock);
	//vbi_teletext_set_default_region(parser->dec, region_id);
	parser->region_id = region_id;
	pthread_mutex_unlock(&parser->lock);
	return AM_SUCCESS;
}

AM_ErrorCode_t
AM_TT2_SetTTDisplayMode(AM_TT2_Handle_t handle, int mode)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
		return AM_TT2_ERR_INVALID_HANDLE;

	pthread_mutex_lock(&parser->lock);

	if (mode == TT_DISP_MIX_TRANSPARENT)
	{
		mode = 0;
		parser->transparent_background = 1;
	}
	else
		parser->transparent_background = 0;

	parser->display_mode = mode;
	parser->disp_update = AM_TRUE;

	pthread_mutex_unlock(&parser->lock);
	pthread_cond_signal(&parser->cond);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TT2_GotoSubtitle(AM_TT2_Handle_t handle)
{
	AM_TT2_Parser_t      *parser = (AM_TT2_Parser_t*)handle;
	AM_TT2_CachedPage_t  *target_page;

	if (!parser->subtitle_head)
		return AM_FAILURE;

	pthread_mutex_lock(&parser->lock);
	target_page = find_cached_page(parser, vbi_dec2bcd(parser->page_no), parser->sub_page_no, 0xFF);

	if (!target_page)
		target_page = parser->subtitle_head;
	else if (target_page->page_type & 0x8000)
	{
		target_page = target_page->sub_next;
		if (!target_page)
			target_page = parser->subtitle_head;
	} else {
		target_page = parser->subtitle_head;
	}

	parser->page_no = vbi_bcd2dec(target_page->page.pgno);
	parser->sub_page_no = target_page->page.subno;
	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

/**\brief 取得用户定义数据
 * \param handle 句柄
 * \return 用户定义数据
 */
void* AM_TT2_GetUserData(AM_TT2_Handle_t handle)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
	{
		return NULL;
	}

	return parser->para.user_data;
}

/**\brief 分析teletext数据
 * \param handle 句柄
 * \param[in] buf PES数据缓冲区
 * \param size 缓冲区内数据大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_Decode(AM_TT2_Handle_t handle, uint8_t *buf, int size)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	int scrambling_control;
	int pts_dts_flag;
	int pes_header_length;
	uint64_t pts = 0ll;
	vbi_sliced sliced[AM_TT2_MAX_SLICES];
	vbi_sliced *s = sliced;
	int s_count = 0;
	int packet_head;
	uint8_t *ptr;
	int left;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	if (size < 9)
		goto end;

	scrambling_control = (buf[6] >> 4) & 0x03;
	pts_dts_flag = (buf[7] >> 6) & 0x03;
	pes_header_length = buf[8];
	if (((pts_dts_flag == 2) || (pts_dts_flag == 3)) && (size > 13))
	{
		pts |= (uint64_t)((buf[9] >> 1) & 0x07) << 30;
		pts |= (uint64_t)((((buf[10] << 8) | buf[11]) >> 1) & 0x7fff) << 15;
		pts |= (uint64_t)((((buf[12] << 8) | buf[13]) >> 1) & 0x7fff);
	}

	parser->pts = pts;

	packet_head = buf[8] + 9;
	ptr  = buf + packet_head + 1;
	left = size - packet_head - 1;

	pthread_mutex_lock(&parser->lock);

	while (left >= 2)
	{
		unsigned int field;
		unsigned int field_line;
		unsigned int frame_line;
		int data_unit_length;
		int data_unit_id;
		int i;

		data_unit_id = ptr[0];
		data_unit_length = ptr[1];
		if ((data_unit_id != 0x02) && (data_unit_id != 0x03))
			goto next_packet;
		if (data_unit_length > left)
			break;
		if (data_unit_length < 44)
			goto next_packet;
		if (ptr[3] != 0xE4)
			goto next_packet;

		tt_lofp_to_line(&field, &field_line, &frame_line, ptr[2], SYSTEM_625);
		if (0 != frame_line)
		{
			s->line = frame_line;
		}
		else
		{
			s->line = 0;
		}

		s->id = VBI_SLICED_TELETEXT_B;
		for (i = 0; i < 42; ++i)
			s->data[i] = vbi_rev8 (ptr[4 + i]);

		s++;
		s_count++;

		if (s_count == AM_TT2_MAX_SLICES)
		{
			vbi_decode(parser->dec, sliced, s_count, pts/90000.);
			s = sliced;
			s_count = 0;
		}
next_packet:
		ptr += data_unit_length + 2;
		left -= data_unit_length + 2;
	}

	if (s_count)
	{
		vbi_decode(parser->dec, sliced, s_count, pts/90000.);
	}

	pthread_mutex_unlock(&parser->lock);

end:
	return AM_SUCCESS;
}

/**\brief 开始teletext显示
 * \param handle 句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_Start(AM_TT2_Handle_t handle, int region_id)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	parser->dec = vbi_decoder_new();
	if (!parser->dec)
	{
		free(parser);
		parser->dec = NULL;
		return AM_TT2_ERR_CREATE_DECODE;
	}

	vbi_event_handler_register(parser->dec, VBI_EVENT_TTX_PAGE, tt2_event_handler, parser);
	vbi_event_handler_register(parser->dec, VBI_EVENT_TIME, tt2_time_update, parser);

	parser->page_no = 100;
	parser->sub_page_no = AM_TT2_ANY_SUBNO;
	parser->display_mode = 0;
	parser->transparent_background = AM_FALSE;
	parser->lock_subpg = 0;
	parser->lock_subpgno = 0;
	parser->cached_pages = NULL;
	parser->cached_tail = NULL;
	parser->display_page = NULL;
	parser->subtitle_head = NULL;
	parser->region_id = region_id;
	parser->thread = 0;
	parser->vbi_tid = 0;
	memset(&parser->last_display_page, 0, sizeof(AM_TT2_CachedPage_t));
	memset(&parser->nav_link, 0, sizeof(nav_link_t));

	/* Set teletext default region, See libzvbi/src/lang.c */
	vbi_teletext_set_default_region(parser->dec, region_id);

	if (!parser->running)
	{
		parser->running = AM_TRUE;
		if (pthread_create(&parser->thread, NULL, tt2_thread, parser))
		{
			parser->running = AM_FALSE;
			return AM_TT2_ERR_CANNOT_CREATE_THREAD;
		}
	}

	if (parser->input == AM_TT_INPUT_VBI)
	{
		if (pthread_create(&parser->vbi_tid, NULL, tt2_vbi_data_thread, parser))
		{
			parser->running = AM_FALSE;
			return AM_TT2_ERR_CANNOT_CREATE_THREAD;
		}
	}
	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

/**\brief 停止teletext显示
 * \param handle 句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_Stop(AM_TT2_Handle_t handle)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	AM_Bool_t wait = AM_FALSE;
	int count = 0;
	AM_TT2_CachedPage_t *cache_page;
	AM_TT2_CachedPage_t *free_cache_page;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	if (parser->running)
	{
		parser->running = AM_FALSE;
		wait = AM_TRUE;
	}

	pthread_mutex_unlock(&parser->lock);
	pthread_cond_broadcast(&parser->cond);

	if (parser->input == AM_TT_INPUT_VBI && (parser->vbi_tid != 0))
		pthread_kill(parser->vbi_tid, SIGALRM);

	if (wait && (parser->thread != 0))
	{
		pthread_join(parser->thread, NULL);
	}

	if (parser->input == AM_TT_INPUT_VBI && wait && (parser->vbi_tid != 0))
	{
		pthread_join(parser->vbi_tid, NULL);
	}

	cache_page = parser->cached_pages;

	while (cache_page)
	{
		vbi_unref_page(&cache_page->page);
		free_cache_page = cache_page;
		cache_page = cache_page->next;
		memset(free_cache_page, 0, sizeof(AM_TT2_CachedPage_t));
		free(free_cache_page);
	}
	parser->cached_pages = NULL;
	parser->vbi_tid = 0;
	parser->thread = 0;

	if (parser->search)
		vbi_search_delete(parser->search);

	if (parser->dec)
		vbi_decoder_delete(parser->dec);
	parser->dec = NULL;

	return AM_SUCCESS;
}

/**\brief 跳转到指定页
 * \param handle 句柄
 * \param page_no 页号
 * \param sub_page_no 子页号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_GotoPage(AM_TT2_Handle_t handle, int page_no, int sub_page_no)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	if (page_no<100 || page_no>899)
	{
		return AM_TT2_ERR_INVALID_PARAM;
	}

	if (sub_page_no > 0xFF && sub_page_no != AM_TT2_ANY_SUBNO)
	{
		return AM_TT2_ERR_INVALID_PARAM;
	}
	AM_DEBUG(0, "AM tt2 GotoPage p %d sub %d", page_no, sub_page_no);

	pthread_mutex_lock(&parser->lock);

	parser->page_no = page_no;
	if (parser->lock_subpg)
		parser->lock_subpgno = sub_page_no;
	else
		parser->sub_page_no = sub_page_no;

	parser->sub_page_no = sub_page_no;
	parser->disp_update = AM_TRUE;

	pthread_mutex_unlock(&parser->lock);

	pthread_cond_signal(&parser->cond);

	return AM_SUCCESS;
}

/**\brief 跳转到home页
 * \param handle 句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_GoHome(AM_TT2_Handle_t handle)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	vbi_page page;
	vbi_link link;
	int page_type;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	if (parser->curr_page)
	{
		vbi_resolve_home(&parser->curr_page->page, &link);
		if (link.type == VBI_LINK_PAGE) {
			parser->page_no = vbi_bcd2dec(link.pgno);
			parser->sub_page_no = AM_TT2_ANY_SUBNO;
		} else if (link.type == VBI_LINK_SUBPAGE) {
			parser->sub_page_no = link.subno;
		}

		parser->disp_update = AM_TRUE;
		pthread_cond_signal(&parser->cond);
	}

	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

/**\brief 跳转到下一页
 * \param handle 句柄
 * \param dir 搜索方向，+1为正向，-1为反向
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_NextPage(AM_TT2_Handle_t handle, int dir)
{
	AM_TT2_Parser_t         *parser = (AM_TT2_Parser_t*)handle;
	int                      pgno, subno;
	AM_TT2_CachedPage_t     *target_page = NULL;
	int                      next_page_no;
	int                      max_page_no = 0;
	int                      min_page_no = 0x8FF;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	pgno  = vbi_dec2bcd(parser->page_no);
	subno = parser->sub_page_no;
#if 0
	if (vbi_get_next_pgno(parser->dec, dir, &pgno, &subno))
	{
		parser->page_no = vbi_bcd2dec(pgno);
		parser->sub_page_no = subno;

		parser->disp_update = AM_TRUE;
		pthread_cond_signal(&parser->cond);
	}
#endif

	target_page = parser->cached_pages;

	if (dir > 0)
	{
		next_page_no = 0x900;
		while (target_page)
		{
			if (target_page->page.pgno > pgno && target_page->page.pgno < next_page_no)
			{
				next_page_no = target_page->page.pgno;
				subno = target_page->page.subno;
#ifdef DEBUG_TT
				AM_DEBUG(0, "pgno %x tpgno %x npgno %x", pgno, target_page->page.pgno, next_page_no);
#endif
			}
			max_page_no = max_page_no > target_page->page.pgno ? max_page_no : target_page->page.pgno;
			min_page_no = min_page_no < target_page->page.pgno ? min_page_no : target_page->page.pgno;
			target_page = target_page->next;
		}
		if (next_page_no == 0x900)
			next_page_no = min_page_no;
	}
	else
	{
		next_page_no = 0;
		while (target_page)
		{
			if (target_page->page.pgno < pgno && target_page->page.pgno > next_page_no)
			{
				next_page_no = target_page->page.pgno;
				subno = target_page->page.subno;
#ifdef DEBUG_TT
				AM_DEBUG(0, "pgno %x tpgno %x npgno %x", pgno, target_page->page.pgno, next_page_no);
#endif
			}
			max_page_no = max_page_no > target_page->page.pgno ? max_page_no : target_page->page.pgno;
			min_page_no = min_page_no < target_page->page.pgno ? min_page_no : target_page->page.pgno;
			target_page = target_page->next;
		}
		if (next_page_no == 0)
			next_page_no = max_page_no;
	}

	parser->page_no = vbi_bcd2dec(next_page_no);
	parser->sub_page_no = AM_TT2_ANY_SUBNO;
	parser->disp_update = AM_TRUE;

	pthread_mutex_unlock(&parser->lock);
	pthread_cond_signal(&parser->cond);

	return AM_SUCCESS;
}

/**\brief 跳转到下一子页
 * \param handle 句柄
 * \param dir 搜索方向，+1为正向，-1为反向
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_NextSubPage(AM_TT2_Handle_t handle, int dir)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	int pgno, subno;
	AM_DEBUG(0, "AM tt2 nextSubPage");

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}
	if (!parser->curr_page)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	pgno  = parser->curr_page->page.pgno;
	subno = parser->curr_page->page.subno;

	if (vbi_get_next_sub_pgno(parser->dec, dir, &pgno, &subno))
	{
		parser->page_no = vbi_bcd2dec(pgno);
		parser->sub_page_no = subno;

		parser->disp_update = AM_TRUE;
		pthread_cond_signal(&parser->cond);
	}

	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

/**\brief 获取子页信息
 * \param handle 句柄
 * \param pgno 页号
 * \param[out] subs 返回子页号
 * \param[inout] len 输入数组subs长度，返回实际子页数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t
AM_TT2_GetSubPageInfo(AM_TT2_Handle_t handle, int pgno, int *subs, int *len)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	if (!subs || !len)
		return AM_TT2_ERR_INVALID_PARAM;

	if (*len <= 0)
		return AM_TT2_ERR_INVALID_PARAM;

	pthread_mutex_lock(&parser->lock);

	pgno  = vbi_dec2bcd(pgno);
	if (!vbi_get_sub_info(parser->dec, pgno, subs, len))
	{
		*len = 0;
	}

	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

/**\brief 根据颜色跳转到指定链接
 * \param handle 句柄
 * \param color 颜色
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_ColorLink(AM_TT2_Handle_t handle, AM_TT2_Color_t color)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;
	AM_Bool_t cached;
	vbi_page page;
	int page_type;
	int page_no;
	int subno;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	if (color >= 4)
	{
		return AM_TT2_ERR_INVALID_PARAM;
	}

	pthread_mutex_lock(&parser->lock);

	if (parser->curr_page)
	{
		page_no = parser->curr_page->page.nav_link[color].pgno;
		subno = parser->curr_page->page.nav_link[color].subno;
	}
	else
	{
		pthread_mutex_unlock(&parser->lock);
		return AM_FAILURE;
	}

	parser->page_no = vbi_bcd2dec(page_no);
	parser->sub_page_no = AM_TT2_ANY_SUBNO;
	parser->disp_update = AM_TRUE;

	AM_DEBUG(0, "color %d pageno %d", color, parser->page_no);
	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;

}

/**\brief 设定搜索字符串
 * \param handle 句柄
 * \param pattern 搜索字符串
 * \param casefold 是否区分大小写
 * \param regex 是否用正则表达式匹配
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_SetSearchPattern(AM_TT2_Handle_t handle, const char *pattern, AM_Bool_t casefold, AM_Bool_t regex)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	if (parser->search)
	{
		vbi_search_delete(parser->search);
		parser->search = NULL;
	}

	parser->search = vbi_search_new(parser->dec, vbi_dec2bcd(parser->page_no), parser->sub_page_no,
			(uint16_t*)pattern, casefold, regex, NULL);

	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

/**\brief 搜索指定页
 * \param handle 句柄
 * \param dir 搜索方向，+1为正向，-1为反向
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt2.h)
 */
AM_ErrorCode_t AM_TT2_Search(AM_TT2_Handle_t handle, int dir)
{
	AM_TT2_Parser_t *parser = (AM_TT2_Parser_t*)handle;

	if (!parser)
	{
		return AM_TT2_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	if (parser->search)
	{
		vbi_page *page;
		int status;

		status = vbi_search_next(parser->search, &page, dir);
		if (status == VBI_SEARCH_SUCCESS) {
			parser->page_no = vbi_bcd2dec(page->pgno);
			parser->sub_page_no = page->subno;

			parser->disp_update = AM_TRUE;
			pthread_cond_signal(&parser->cond);
		}
	}

	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

