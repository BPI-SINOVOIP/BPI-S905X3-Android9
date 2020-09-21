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
 * \brief 节目信息窗口
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "dtv.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/

static int prog_info_init (DTV_Window_t *win);
static int prog_info_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int prog_info_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int prog_info_on_timer (DTV_Window_t *win);
static int prog_info_on_show (DTV_Window_t *win);
static int prog_info_on_hide (DTV_Window_t *win);
static int prog_info_quit (DTV_Window_t *win);

static struct {
	DTV_Window_t win;
	int          focus;
} prog_info = {
{
.init     = prog_info_init,
.update   = prog_info_update,
.on_key   = prog_info_on_key,
.on_timer = prog_info_on_timer,
.on_show  = prog_info_on_show,
.on_hide  = prog_info_on_hide,
.quit     = prog_info_quit
}
};

DTV_Window_t *prog_info_win = (DTV_Window_t*)&prog_info;


static int now_prog_info, end_prog_info;

/****************************************************************************
 * Functions
 ***************************************************************************/

static int prog_info_init (DTV_Window_t *win)
{
	return 0;
}

static int prog_info_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	AM_OSD_Rect_t sr, dr;


	
	if(program_count > 0 )
	{
		sr.x = 0;
		sr.y = 0;
		sr.w = gfx.info_frame_img->width;
		sr.h = gfx.info_frame_img->height;
		
		dr.x = 150;
		dr.y = 520;
		dr.w = gfx.info_frame_img->width;
		dr.h = gfx.info_frame_img->height;

		
		AM_OSD_Blit(gfx.info_frame_img, &sr, s, &dr, NULL);

		DTV_Program_t info;
		
		if (dtv_program_get_info_by_chan_num(curr_program+1,&info) < 0)
		{
			AM_DEBUG(1, "Cannot get curr program, cannot play");
			return -1;
		}


		uint32_t pixel;
		AM_OSD_Color_t col;
		AM_FONT_TextPos_t pos;	

		col.r = 255;
		col.g = 255;
		col.b = 255;
		col.a = 255;
		AM_OSD_MapColor(s->format, &col, &pixel);

		
		pos.rect.x = dr.x+80;
		pos.rect.w = dr.w;
		pos.rect.y = dr.y + 50;
		pos.rect.h = dr.h;
		pos.base   = pos.rect.h;
	
		char str[6];

		sprintf(str,"%d",curr_program+1);
		AM_FONT_Draw(gfx.def_font,s,str,strlen(str),&pos,pixel);
		pos.rect.x = dr.x+350;
		AM_FONT_Draw(gfx.def_font,s,info.name,strlen(info.name),&pos,pixel);
	}

	AM_TIME_GetClock(&now_prog_info);
	
	return 0;
}

static int prog_info_on_key (DTV_Window_t *win, AM_INP_Key_t key)
{
	int ret = 0;
	
	switch(key)
	{
		case AM_INP_KEY_CANCEL:
		case AM_INP_KEY_MENU:
			dtv_window_hide(win);
			ret = 1;
		break;
		case AM_INP_KEY_LEFT:
		break;
		case AM_INP_KEY_RIGHT:
		break;
		case AM_INP_KEY_OK:
		break;
		default:
		break;
	}
	return ret;
}

static int prog_info_on_timer (DTV_Window_t *win)
{
	AM_TIME_GetClock(&end_prog_info);

	if(end_prog_info > now_prog_info )
	{
		if(end_prog_info - now_prog_info > 5000)
		{
			dtv_window_hide(win);
			return 1;
		}	
	}
			
	return 0;
}

static int prog_info_on_show (DTV_Window_t *win)
{
	AM_TIME_GetClock(&now_prog_info);
	return 0;
}

static int prog_info_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int prog_info_quit (DTV_Window_t *win)
{
	return 0;
}

