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
 * \brief 搜索结果窗口
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "dtv.h"
#include "am_scan.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/

static int scan_result_init (DTV_Window_t *win);
static int scan_result_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int scan_result_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int scan_result_on_timer (DTV_Window_t *win);
static int scan_result_on_show (DTV_Window_t *win);
static int scan_result_on_hide (DTV_Window_t *win);
static int scan_result_quit (DTV_Window_t *win);

static struct {
	DTV_Window_t win;
	int          focus;
} scan_result = {
{
.init     = scan_result_init,
.update   = scan_result_update,
.on_key   = scan_result_on_key,
.on_timer = scan_result_on_timer,
.on_show  = scan_result_on_show,
.on_hide  = scan_result_on_hide,
.quit     = scan_result_quit
}
};

enum 
{
	SEARCHING_IDLE=0,
	SEARCHING_LOCKING,
	SEARCHING_LOCKED,
	SEARCHING_WAITING_PAT,
	SEARCHING_WAITING_PMT,
	SEARCHING_WAITING_SDT,
	SEARCHING_WAITING_FINISHED
	
};

#define SEARCH_PROMPT_1	"正在搜索"
#define SEARCH_PROMPT_2 "请您稍等..."

DTV_Window_t *scan_result_win = (DTV_Window_t*)&scan_result;

static int progress=10;

static int searching_state=SEARCHING_IDLE;

static AM_SCAN_Handle_t search_handle = NULL;

/****************************************************************************
 * Functions
 ***************************************************************************/

static int scan_result_init (DTV_Window_t *win)
{
	return 0;
}

static int scan_result_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	AM_OSD_Rect_t sr, dr;
	AM_FONT_TextPos_t pos;
	char buffer[64];
	int i=0;
	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.bg_img->width;
	sr.h = gfx.bg_img->height;
	
	dr.x = 0;
	dr.y = 0;
	dr.w = s->width;
	dr.h = s->height;

	
	AM_OSD_Blit(gfx.bg_img, &sr, s, &dr, NULL);

	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.title_search_img->width;
	sr.h = gfx.title_search_img->height;
	
	dr.x = 72;
	dr.y = 72;
	dr.w = gfx.title_search_img->width;
	dr.h = gfx.title_search_img->height;

	AM_OSD_Blit(gfx.title_search_img, &sr, s, &dr, NULL);


	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.search_frame_img->width;
	sr.h = gfx.search_frame_img->height;
	
	dr.x = 100;
	dr.y = 170;
	dr.w = gfx.search_frame_img->width;
	dr.h = gfx.search_frame_img->height;

	AM_OSD_Blit(gfx.search_frame_img, &sr, s, &dr, NULL);

	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.progress_img->width;
	sr.h = gfx.progress_img->height;
	
	dr.x = 100;
	dr.y = 615;
	dr.w = gfx.progress_img->width;
	dr.h = gfx.progress_img->height;

	AM_OSD_Blit(gfx.progress_img, &sr, s, &dr, NULL);


	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.progress_value_img->width;
	sr.h = gfx.progress_value_img->height;
	
	while(i<progress)
	{
		dr.x = 104+i*25;
		dr.y = 615;
		dr.w = gfx.progress_value_img->width;
		dr.h = gfx.progress_value_img->height;

		AM_OSD_Blit(gfx.progress_value_img, &sr, s, &dr, NULL);


		i++;	

	}

	if(searching_state<SEARCHING_WAITING_FINISHED)
	{
		pos.rect.x=490;
		pos.rect.y=320;
		pos.rect.w=250;
		pos.rect.h=40;
		AM_FONT_Draw(gfx.def_font,s,SEARCH_PROMPT_1,strlen(SEARCH_PROMPT_1),&pos,0xffffffff);
		pos.rect.x=480;
		pos.rect.y=365;
		pos.rect.w=250;
		pos.rect.h=40;
		AM_FONT_Draw(gfx.def_font,s,SEARCH_PROMPT_2,strlen(SEARCH_PROMPT_2),&pos,0xffffffff);



	}
	else
	{
		extern int           program_count;
		extern int           curr_program;
		pos.rect.x=360;
		pos.rect.y=220;
		pos.rect.w=320;
		pos.rect.h=40;
		sprintf(buffer,"搜索完毕，共搜到 %d 个节目",program_count);
		
		AM_FONT_Draw(gfx.def_font,s,buffer,strlen(buffer),&pos,0xffffffff);
	
		

		for(i=0;i<program_count&&i<10;i+=2)	
		{
			pos.rect.x=340;
			pos.rect.y=280+i*21;
			pos.rect.w=320;
			pos.rect.h=40;
			AM_FONT_Draw(gfx.def_font,s,programs[i].name,strlen(programs[i].name),&pos,0xffffffff);
			pos.rect.x=660;
			pos.rect.y=280+i*21;
			pos.rect.w=320;
			pos.rect.h=40;
			AM_FONT_Draw(gfx.def_font,s,programs[i+1].name,strlen(programs[i].name),&pos,0xffffffff);

		
		}
	}

	return 0;
}

static int scan_result_on_key (DTV_Window_t *win, AM_INP_Key_t key)
{
	int ret=0;
	switch(key)
	{
		case AM_INP_KEY_UP:

		ret=1;
		break;
		case AM_INP_KEY_DOWN:
		break;
		case  AM_INP_KEY_OK:
		case AM_INP_KEY_CANCEL:

		ret=1;	
	
		if(searching_state>=SEARCHING_WAITING_FINISHED)
		{
			dtv_window_hide(scan_result_win);
			if(program_count>0)
			{
				dtv_program_play();
			}
		}

		break;
		default:
		break;

	}

	return ret;
}

static int scan_result_on_timer (DTV_Window_t *win)
{
	return 0;
}

static int scan_result_on_show (DTV_Window_t *win)
{
	return 0;
}

static int scan_result_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int scan_result_quit (DTV_Window_t *win)
{
	return 0;
}

static void dtv_program_progress_handler(long dev_no, int event_type, void *param, void *user_data)
{
	extern int  dtv_update(); 
	
	if (event_type == AM_SCAN_EVT_PROGRESS)
	{
		AM_SCAN_Progress_t *prog = (AM_SCAN_Progress_t*)param;

		if (prog->evt == AM_SCAN_PROGRESS_PAT_DONE || 
			prog->evt == AM_SCAN_PROGRESS_SDT_DONE)
		{
			/*更新进度条*/
			AM_DEBUG(1,"Scan progress update");
			progress+=10;
			if(progress>43)
			{
				progress=43;
			}
		}
	}
	dtv_update();
	

}

static void  search_store_cb(AM_SCAN_Result_t *result)
{
	extern void dtv_program_store(AM_SCAN_Result_t *result);
	dtv_program_store(result);
	AM_EVT_Unsubscribe(search_handle, AM_SCAN_EVT_PROGRESS, dtv_program_progress_handler, NULL);
	AM_SCAN_Destroy(search_handle);
	searching_state=SEARCHING_WAITING_FINISHED;	
	progress=43;
	dtv_update();

}

int enter_searching_progress(int freq,int sym,int qam)
{
	struct dvb_frontend_parameters p;
	dtv_window_show(scan_result_win);
	if(sym>0)
	{
		p.frequency = freq;
		p.u.qam.symbol_rate = sym;
		p.u.qam.fec_inner = FEC_AUTO;
		p.u.qam.modulation =qam;
	}
	else
	{
		p.frequency = freq;
		p.u.ofdm.constellation=qam;
	}
	searching_state=0;
	progress=10;
	AM_AV_StopTS(DTV_AV_DEV_NO);
	AM_SCAN_Create(DTV_FEND_DEV_NO, DTV_DMX_DEV_NO, 0, NULL, &p, 1, 
				   AM_SCAN_MODE_MANUAL, search_store_cb, &search_handle);
	/*注册搜索进度通知事件*/
	AM_EVT_Subscribe(search_handle, AM_SCAN_EVT_PROGRESS, dtv_program_progress_handler, NULL);
	AM_SCAN_Start(search_handle);
}


