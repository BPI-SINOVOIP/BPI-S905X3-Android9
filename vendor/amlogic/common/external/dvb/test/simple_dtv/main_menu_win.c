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
 * \brief 主菜单窗口
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

#define MENU_ITEM_COUNT  2

/****************************************************************************
 * Static data
 ***************************************************************************/

static int main_menu_init (DTV_Window_t *win);
static int main_menu_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int main_menu_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int main_menu_on_timer (DTV_Window_t *win);
static int main_menu_on_show (DTV_Window_t *win);
static int main_menu_on_hide (DTV_Window_t *win);
static int main_menu_quit (DTV_Window_t *win);

static struct {
	DTV_Window_t   win;
	int            focus;
} main_menu = {
{
.init     = main_menu_init,
.update   = main_menu_update,
.on_key   = main_menu_on_key,
.on_timer = main_menu_on_timer,
.on_show  = main_menu_on_show,
.on_hide  = main_menu_on_hide,
.quit     = main_menu_quit
}
};

DTV_Window_t *main_menu_win = (DTV_Window_t*)&main_menu;

/****************************************************************************
 * Functions
 ***************************************************************************/

static int main_menu_init (DTV_Window_t *win)
{
	return 0;
}

static int main_menu_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	AM_OSD_Rect_t sr, dr;
	
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
	sr.w = 0;
	sr.w = gfx.title_setting_img->width;
	sr.h = gfx.title_setting_img->height;
	
	dr.x = 200;
	dr.y = 80;

	dr.w = gfx.title_setting_img->width;
	dr.h = gfx.title_setting_img->height;
	
	AM_OSD_Blit(gfx.title_setting_img, &sr, s, &dr, NULL);

	if(main_menu.focus == 0)
	{

		sr.x = 0;
		sr.w = 0;
		sr.w = gfx.btn_search_focus_img->width;
		sr.h = gfx.btn_search_focus_img->height;
		
		dr.x = 400;
		dr.y = 300;
		dr.w = gfx.btn_search_focus_img->width;
		dr.h = gfx.btn_search_focus_img->height;
		
		AM_OSD_Blit(gfx.btn_search_focus_img, &sr, s, &dr, NULL);

		sr.x = 0;
		sr.w = 0;
		sr.w = gfx.btn_reset_img->width;
		sr.h = gfx.btn_reset_img->height;
		
		dr.x = 400;
		dr.y = 400;
		dr.w = gfx.btn_reset_img->width;
		dr.h = gfx.btn_reset_img->height;
		
		AM_OSD_Blit(gfx.btn_reset_img, &sr, s, &dr, NULL);
	
	}
	else
	{
		sr.x = 0;
		sr.w = 0;
		sr.w = gfx.btn_search_img->width;
		sr.h = gfx.btn_search_img->height;
		
		dr.x = 400;
		dr.y = 300;
		dr.w = gfx.btn_search_img->width;
		dr.h = gfx.btn_search_img->height;
		
		AM_OSD_Blit(gfx.btn_search_img, &sr, s, &dr, NULL);

		sr.x = 0;
		sr.w = 0;
		sr.w = gfx.btn_reset_focus_img->width;
		sr.h = gfx.btn_reset_focus_img->height;
		
		dr.x = 400;
		dr.y = 400;
		dr.w = gfx.btn_reset_focus_img->width;
		dr.h = gfx.btn_reset_focus_img->height;
		
		AM_OSD_Blit(gfx.btn_reset_focus_img, &sr, s, &dr, NULL);
	}
		
	return 0;
}

static int main_menu_on_key (DTV_Window_t *win, AM_INP_Key_t key)
{
	int ret = 0;
	
	switch(key)
	{
		case AM_INP_KEY_CANCEL:
		case AM_INP_KEY_MENU:
			dtv_window_hide(win);
			ret = 1;
		break;
		case AM_INP_KEY_UP:
			main_menu.focus--;
			if(main_menu.focus<0)
				main_menu.focus = MENU_ITEM_COUNT-1;
			ret = 1;
		break;
		case AM_INP_KEY_DOWN:
			main_menu.focus++;
			if(main_menu.focus>=MENU_ITEM_COUNT)
				main_menu.focus = 0;
			ret = 1;
		break;
		case AM_INP_KEY_OK:
			switch(main_menu.focus)
			{
				case 0:
					dtv_window_hide(win);
					extern fe_type_t fe_type;
					if(fe_type!=FE_QAM)
					dtv_window_show(dvbt_param_win);
					else
					dtv_window_show(dvbc_param_win);
					ret = 1;
				break;
				case 1:
					dtv_window_hide(win);
					dtv_window_show(restore_win);
					ret = 1;
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
	
	return ret;
}

static int main_menu_on_timer (DTV_Window_t *win)
{
	return 0;
}

static int main_menu_on_show (DTV_Window_t *win)
{
	return 0;
}

static int main_menu_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int main_menu_quit (DTV_Window_t *win)
{
	return 0;
}

