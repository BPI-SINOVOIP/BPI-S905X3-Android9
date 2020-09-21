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
 * \brief 窗口
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
 * API functions
 ***************************************************************************/

int dtv_window_init(DTV_Window_t *win)
{
	if(win->init)
		win->init(win);
	
	return 0;
}

int dtv_window_show(DTV_Window_t *win)
{
	win->show = AM_TRUE;
	
	if(win->on_show)
		win->on_show(win);
	
	return 0;
}

int dtv_window_hide(DTV_Window_t *win)
{
	if(!win->show)
		return 0;
	
	win->show = AM_FALSE;
	
	if(win->on_hide)
		win->on_hide(win);
	
	return 0;
}

int dtv_window_update(DTV_Window_t *win, AM_OSD_Surface_t *surface)
{
	if(!win->show)
		return 0;
	
	if(win->update)
		win->update(win, surface);
	
	return 0;
}

int dtv_window_on_key(DTV_Window_t *win, AM_INP_Key_t key)
{
	if(!win->show)
		return 0;
	
	if(win->on_key)
		return win->on_key(win, key);
	
	return 0;
}

int dtv_window_on_timer(DTV_Window_t *win)
{
	if(!win->show)
		return 0;
	
	if(win->on_timer)
		return win->on_timer(win);
	
	return 0;
}

int dtv_window_quit(DTV_Window_t *win)
{
	if(win->quit)
		win->quit(win);
	
	return 0;
}

