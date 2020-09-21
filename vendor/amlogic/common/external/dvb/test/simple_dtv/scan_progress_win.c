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
 * \brief 搜索进度窗口
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

static int scan_progress_init (DTV_Window_t *win);
static int scan_progress_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int scan_progress_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int scan_progress_on_timer (DTV_Window_t *win);
static int scan_progress_on_show (DTV_Window_t *win);
static int scan_progress_on_hide (DTV_Window_t *win);
static int scan_progress_quit (DTV_Window_t *win);

static struct {
	DTV_Window_t win;
	int          focus;
} scan_progress = {
{
.init     = scan_progress_init,
.update   = scan_progress_update,
.on_key   = scan_progress_on_key,
.on_timer = scan_progress_on_timer,
.on_show  = scan_progress_on_show,
.on_hide  = scan_progress_on_hide,
.quit     = scan_progress_quit
}
};

DTV_Window_t *scan_progress_win = (DTV_Window_t*)&scan_progress;

/****************************************************************************
 * Functions
 ***************************************************************************/

static int scan_progress_init (DTV_Window_t *win)
{
	return 0;
}

static int scan_progress_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	return 0;
}

static int scan_progress_on_key (DTV_Window_t *win, AM_INP_Key_t key)
{
	return 0;
}

static int scan_progress_on_timer (DTV_Window_t *win)
{
	return 0;
}

static int scan_progress_on_show (DTV_Window_t *win)
{
	return 0;
}

static int scan_progress_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int scan_progress_quit (DTV_Window_t *win)
{
	return 0;
}

