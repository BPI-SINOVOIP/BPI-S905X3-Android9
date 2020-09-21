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
 * \brief 恢复出厂设置窗口
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

static int restore_init (DTV_Window_t *win);
static int restore_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int restore_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int restore_on_timer (DTV_Window_t *win);
static int restore_on_show (DTV_Window_t *win);
static int restore_on_hide (DTV_Window_t *win);
static int restore_quit (DTV_Window_t *win);

static struct {
	DTV_Window_t win;
	int          focus;
} restore = {
{
.init     = restore_init,
.update   = restore_update,
.on_key   = restore_on_key,
.on_timer = restore_on_timer,
.on_show  = restore_on_show,
.on_hide  = restore_on_hide,
.quit     = restore_quit
}
};

DTV_Window_t *restore_win = (DTV_Window_t*)&restore;

/****************************************************************************
 * Functions
 ***************************************************************************/

static int restore_init (DTV_Window_t *win)
{
	return 0;
}

static int restore_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	return 0;
}

static int restore_on_key (DTV_Window_t *win, AM_INP_Key_t key)
{
	return 0;
}

static int restore_on_timer (DTV_Window_t *win)
{
	return 0;
}

static int restore_on_show (DTV_Window_t *win)
{
	return 0;
}

static int restore_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int restore_quit (DTV_Window_t *win)
{
	return 0;
}

