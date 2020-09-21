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
 * \brief 音量控制窗口
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

static int vol_init (DTV_Window_t *win);
static int vol_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int vol_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int vol_on_timer (DTV_Window_t *win);
static int vol_on_show (DTV_Window_t *win);
static int vol_on_hide (DTV_Window_t *win);
static int vol_quit (DTV_Window_t *win);

#define VOL_VALUE_STEP  5
static int now,end;

static struct {
	DTV_Window_t win;
	int          focus;
	int 		 cur_vol_value;
} vol = {
{
.init     = vol_init,
.update   = vol_update,
.on_key   = vol_on_key,
.on_timer = vol_on_timer,
.on_show  = vol_on_show,
.on_hide  = vol_on_hide,
.quit     = vol_quit
}
};

DTV_Window_t *vol_win = (DTV_Window_t*)&vol;

/****************************************************************************
 * Functions
 ***************************************************************************/

static int vol_init (DTV_Window_t *win)
{
		return 0;
}

static int vol_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_OSD_Rect_t sr, dr;
	int i =0 ,tmp;

	ret = AM_AOUT_GetVolume(DTV_AOUT_DEV_NO, &(vol.cur_vol_value));
	if(ret!=AM_SUCCESS)
	{
		printf("AM_AOUT_SetVolume fail!\n");
	}
	else
	{	
		AM_DEBUG(1, "vol_value = %d",vol.cur_vol_value);
	}

	//draw vol bar
	
	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.volume_img->width;
	sr.h = gfx.volume_img->height;
	
	dr.x = 300;
	dr.y = 450;
	dr.w = gfx.volume_img->width;
	dr.h = gfx.volume_img->height;

	
	AM_OSD_Blit(gfx.volume_img, &sr, s, &dr, NULL);

	if(tmp = vol.cur_vol_value / VOL_VALUE_STEP)

		AM_DEBUG(1, "%d\n",tmp);
	
	for(i=0;i<tmp;i++)
	{
		sr.x = 0;
		sr.w = 0;
		sr.w = gfx.volume_value_img->width;
		sr.h = gfx.volume_value_img->height;
		
		dr.x = 376 + ( i * (gfx.volume_value_img->width + 4));
		dr.y = 459;
		dr.w = gfx.volume_value_img->width;
		dr.h = gfx.volume_value_img->height;
		
		AM_OSD_Blit(gfx.volume_value_img, &sr, s, &dr, NULL);
	
	}

	AM_TIME_GetClock(&now);

	
	return 0;
}

static int vol_on_key (DTV_Window_t *win, AM_INP_Key_t key)
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
			if(vol.cur_vol_value>=VOL_VALUE_STEP)
				vol.cur_vol_value =vol.cur_vol_value - VOL_VALUE_STEP;
			else
				ret = 0;
			AM_AOUT_SetVolume(0,vol.cur_vol_value);
			ret = 1;
		break;
		
		case AM_INP_KEY_RIGHT:
			if(vol.cur_vol_value <= 100 - VOL_VALUE_STEP)
				vol.cur_vol_value = vol.cur_vol_value + VOL_VALUE_STEP;	
			else
				ret = 0;
			AM_AOUT_SetVolume(0,vol.cur_vol_value);
			ret = 1;
		break;
		case AM_INP_KEY_OK:
		break;
		default:
		break;
	}
	return ret;
}

static int vol_on_timer (DTV_Window_t *win)
{
	AM_TIME_GetClock(&end);

	if(end > now )
	{
		if(end - now > 5000)
		{
			dtv_window_hide(win);
			return 1;
		}	
	}
			
	return 0;
}

static int vol_on_show (DTV_Window_t *win)
{
	AM_TIME_GetClock(&now);
	return 0;
}

static int vol_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int vol_quit (DTV_Window_t *win)
{
	return 0;
}

