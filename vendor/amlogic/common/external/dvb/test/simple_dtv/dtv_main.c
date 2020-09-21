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
 * \brief DTV Demo主程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "dtv.h"
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define INP_TIMEOUT 20

/****************************************************************************
 * Static functions
 ***************************************************************************/

static int dtv_keymap(void)
{
	static AM_INP_KeyMap_t map;
	
	memset(&map, 0, sizeof(map));
#if 0
	map.codes[AM_INP_KEY_UP] = 'w';
	map.codes[AM_INP_KEY_DOWN] = 's';
	map.codes[AM_INP_KEY_LEFT] = 'a';
	map.codes[AM_INP_KEY_RIGHT] = 'd';
	map.codes[AM_INP_KEY_OK] = ' ';
	map.codes[AM_INP_KEY_CANCEL] = 'c';
	map.codes[AM_INP_KEY_0] = '0';
	map.codes[AM_INP_KEY_1] = '1';
	map.codes[AM_INP_KEY_2] = '2';
	map.codes[AM_INP_KEY_3] = '3';
	map.codes[AM_INP_KEY_4] = '4';
	map.codes[AM_INP_KEY_5] = '5';
	map.codes[AM_INP_KEY_6] = '6';
	map.codes[AM_INP_KEY_7] = '7';
	map.codes[AM_INP_KEY_8] = '8';
	map.codes[AM_INP_KEY_9] = '9';
	map.codes[AM_INP_KEY_MENU] = 'm';
	map.codes[AM_INP_KEY_POWER] = 'p';
#else
	map.codes[AM_INP_KEY_UP] = 0xca;
	map.codes[AM_INP_KEY_DOWN] = 0xd2;
	map.codes[AM_INP_KEY_LEFT] = 0x99;
	map.codes[AM_INP_KEY_RIGHT] = 0xc1;
	map.codes[AM_INP_KEY_OK] = 0xce;
	map.codes[AM_INP_KEY_CANCEL] = 0xc5;
	map.codes[AM_INP_KEY_0] = 0x87;
	map.codes[AM_INP_KEY_1] = 0x92;
	map.codes[AM_INP_KEY_2] = 0x93;
	map.codes[AM_INP_KEY_3] = 0xcc;
	map.codes[AM_INP_KEY_4] = 0x8e;
	map.codes[AM_INP_KEY_5] = 0x8f;
	map.codes[AM_INP_KEY_6] = 0xc8;
	map.codes[AM_INP_KEY_7] = 0x8a;
	map.codes[AM_INP_KEY_8] = 0x8b;
	map.codes[AM_INP_KEY_9] = 0xc4;
	map.codes[AM_INP_KEY_MENU] = 0xd6;
	map.codes[AM_INP_KEY_POWER] = 0xdc;
#endif
	AM_INP_SetKeyMap(DTV_INP_DEV_NO, &map);
	
	return 0;
}

static int dtv_init(void)
{
	AM_INP_OpenPara_t inp_para;
	AM_FEND_OpenPara_t fend_para;
	AM_DMX_OpenPara_t dmx_para;
	AM_AV_OpenPara_t av_para;
	AM_OSD_OpenPara_t osd_para;
	AM_AOUT_OpenPara_t aout_para;
	
	memset(&inp_para, 0, sizeof(inp_para));
	AM_INP_Open(DTV_INP_DEV_NO, &inp_para);
	dtv_keymap();
	
	memset(&fend_para, 0, sizeof(fend_para));
	AM_FEND_Open(DTV_FEND_DEV_NO, &fend_para);
	
	memset(&dmx_para, 0, sizeof(dmx_para));
	AM_DMX_Open(DTV_DMX_DEV_NO, &dmx_para);
	
	memset(&av_para, 0, sizeof(av_para));
	AM_AV_Open(DTV_AV_DEV_NO, &av_para);

	AM_DMX_SetSource(DTV_DMX_DEV_NO, AM_DMX_SRC_TS2);
	AM_AV_SetTSSource(DTV_AV_DEV_NO, AM_AV_TS_SRC_TS2);
	
	memset(&osd_para, 0, sizeof(osd_para));
	osd_para.format = AM_OSD_FMT_COLOR_8888;
	osd_para.width  = 1280;
	osd_para.height = 720;
	osd_para.output_width  = 1280;
	osd_para.output_height = 720;
	osd_para.enable_double_buffer = AM_TRUE;
	AM_OSD_Open(DTV_OSD_DEV_NO, &osd_para);

	memset(&aout_para, 0, sizeof(aout_para));
	AM_AOUT_Open(DTV_AOUT_DEV_NO, &aout_para);
	
	AM_FONT_Init();
	
	dtv_program_init();
	
	gfx_init();
	
	dtv_window_init(restore_win);
	dtv_window_init(scan_result_win);
	dtv_window_init(scan_progress_win);
	dtv_window_init(dvbt_param_win);
	dtv_window_init(dvbc_param_win);
	dtv_window_init(main_menu_win);
	dtv_window_init(vol_win);
	dtv_window_init(prog_list_win);
	dtv_window_init(prog_info_win);

	dtv_program_play();	
	
	return 0;
}

static int dtv_quit(void)
{
	dtv_window_quit(restore_win);
	dtv_window_quit(scan_result_win);
	dtv_window_quit(scan_progress_win);
	dtv_window_quit(dvbt_param_win);
	dtv_window_quit(dvbc_param_win);
	dtv_window_quit(main_menu_win);
	dtv_window_quit(vol_win);
	dtv_window_quit(prog_list_win);
	dtv_window_quit(prog_info_win);
	
	gfx_quit();
	
	AM_FONT_Quit();
	
	dtv_program_quit();
	AM_OSD_Close(DTV_OSD_DEV_NO);
	AM_AOUT_Close(DTV_AOUT_DEV_NO);
	AM_AV_Close(DTV_AV_DEV_NO);
	AM_DMX_Close(DTV_DMX_DEV_NO);
	AM_FEND_Close(DTV_FEND_DEV_NO);
	AM_INP_Close(DTV_INP_DEV_NO);
	return 0;
}

int dtv_update()
{
	AM_OSD_Surface_t *s;
	AM_OSD_Color_t col;
	uint32_t pixel;
	AM_OSD_Rect_t r;
	
	AM_OSD_GetSurface(DTV_OSD_DEV_NO, &s);
	
	col.r = 0;
	col.g = 0;
	col.b = 0;
	col.a = 0;
	AM_OSD_MapColor(s->format, &col, &pixel);
	r.x = 0;
	r.y = 0;
	r.w = s->width;
	r.h = s->height;
	AM_OSD_DrawFilledRect(s, &r, pixel);
	
	dtv_window_update(prog_info_win, s);
	dtv_window_update(prog_list_win, s);
	dtv_window_update(vol_win, s);
	dtv_window_update(main_menu_win, s);
	dtv_window_update(dvbc_param_win, s);
	dtv_window_update(dvbt_param_win, s);
	dtv_window_update(scan_progress_win, s);
	dtv_window_update(scan_result_win, s);
	dtv_window_update(restore_win, s);
	
	AM_OSD_Update(DTV_OSD_DEV_NO, NULL);
	return 0;
}

static int dtv_on_key(AM_INP_Key_t key)
{
	int ret = 0;
	
	ret = dtv_window_on_key(restore_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(scan_result_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(scan_progress_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(dvbt_param_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(dvbc_param_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(main_menu_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(vol_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(prog_list_win, key);
	
	if(ret==0)
		ret = dtv_window_on_key(prog_info_win, key);
	
	return ret;
}

static int dtv_on_timer(AM_INP_Key_t key)
{
	int ret = 0;
	
	ret |= dtv_window_on_timer(restore_win);
	ret |= dtv_window_on_timer(scan_result_win);
	ret |= dtv_window_on_timer(scan_progress_win);
	ret |= dtv_window_on_timer(dvbt_param_win);
	ret |= dtv_window_on_timer(dvbc_param_win);
	ret |= dtv_window_on_timer(main_menu_win);
	ret |= dtv_window_on_timer(vol_win);
	ret |= dtv_window_on_timer(prog_list_win);
	ret |= dtv_window_on_timer(prog_info_win);
	
	return ret;
}

static int dtv_desktop_on_key(AM_INP_Key_t key)
{
	int ret = 0;
	
	switch(key)
	{
		case AM_INP_KEY_MENU:
			dtv_window_show(main_menu_win);
			ret = 1;
		break;

		case AM_INP_KEY_LEFT:
		case AM_INP_KEY_RIGHT:
			dtv_window_show(vol_win);
			ret = 1;
		break;
		case AM_INP_KEY_OK:
			dtv_window_show(prog_list_win);
			ret = 1;
		break;
	
		case AM_INP_KEY_UP:	
			dtv_program_channel_up();
			ret = 1;	
		break;

		case AM_INP_KEY_DOWN:	
			dtv_program_channel_down();
			ret = 1;	
		break;

		default:
		break;
	}
	
	return ret;
}

static int dtv_main(void)
{
	struct input_event evt;
	
	dtv_update();
	
	while (1)
	{
		int update = 0;

		if(AM_INP_WaitEvent(DTV_INP_DEV_NO, &evt, INP_TIMEOUT)!=AM_SUCCESS)
			goto timer;
		if((evt.type!=EV_KEY) || (evt.value!=1))
			goto timer;
		
		if(evt.code==AM_INP_KEY_POWER)
			break;
		
		if(dtv_on_key(evt.code))
			update = 1;
		else if(dtv_desktop_on_key(evt.code))
			update = 1;
timer:
		if(dtv_on_timer(evt.code))
			update = 1;
			
		if(update)
			dtv_update();
	}
	
	return 0;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

int main(int argc, char **argv)
{
	dtv_init();
	
	dtv_main();
	
	dtv_quit();
	return 0;
}

