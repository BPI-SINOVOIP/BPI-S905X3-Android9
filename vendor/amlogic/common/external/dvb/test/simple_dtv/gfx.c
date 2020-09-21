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
 * \brief 图形相关资源
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "dtv.h"

/****************************************************************************
 * Data
 ***************************************************************************/

DTV_Gfx_t     gfx;

/****************************************************************************
 * API functions
 ***************************************************************************/

int gfx_init(void)
{
	AM_FONT_Attr_t fattr;
	AM_IMG_DecodePara_t dec;
	
	fattr.name  = "hei";
	fattr.flags = 0;
	fattr.encoding = AM_FONT_ENC_UTF16;
	fattr.charset  = AM_FONT_CHARSET_UNICODE;
	AM_FONT_LoadFile("res/wqy-microhei.ttc", "freetype", &fattr);
	
	fattr.width  = 40;
	fattr.height = 40;
	fattr.encoding = AM_FONT_ENC_UTF8;
	AM_FONT_Create(&fattr, &gfx.def_font);
	
	dec.enable_hw = AM_FALSE;
	dec.hw_dev_no = DTV_AV_DEV_NO;
	dec.surface_flags = AM_OSD_SURFACE_FL_HW;
	
	AM_IMG_Load("res/bg.png", &dec, &gfx.bg_img);
	gfx.bg_img->flags &=~AM_OSD_SURFACE_FL_ALPHA;
	AM_IMG_Load("res/btn_search_focus.png", &dec, &gfx.btn_search_focus_img);
	AM_IMG_Load("res/btn_search_unfocused.png", &dec, &gfx.btn_search_img);
	AM_IMG_Load("res/btn_reset_focus.png", &dec, &gfx.btn_reset_focus_img);
	AM_IMG_Load("res/btn_reset_unfocused.png", &dec, &gfx.btn_reset_img);
	AM_IMG_Load("res/setting.png", &dec, &gfx.title_setting_img);
	AM_IMG_Load("res/search.png", &dec, &gfx.title_search_img);
	AM_IMG_Load("res/confirm.png", &dec, &gfx.confirm_img);
	AM_IMG_Load("res/cancel.png", &dec, &gfx.cancel_img);
	AM_IMG_Load("res/textarea_unfocused.png", &dec, &gfx.inp_img);
	AM_IMG_Load("res/textarea_focus.png", &dec, &gfx.inp_focus_img);
	AM_IMG_Load("res/list_focus.png", &dec, &gfx.list_focus_img);
	AM_IMG_Load("res/process01.png", &dec, &gfx.progress_img);
	AM_IMG_Load("res/process02.png", &dec, &gfx.progress_value_img);
	AM_IMG_Load("res/volum01.png", &dec, &gfx.volume_img);
	AM_IMG_Load("res/volum02.png", &dec, &gfx.volume_value_img);
	AM_IMG_Load("res/search_frame.png", &dec, &gfx.search_frame_img);
	AM_IMG_Load("res/list_frame.png", &dec, &gfx.list_frame_img);
	AM_IMG_Load("res/item_frame.png", &dec, &gfx.info_frame_img);
	AM_IMG_Load("res/pop_window.png", &dec, &gfx.pop_win_img);
	AM_IMG_Load("res/search_prompt.png", &dec, &gfx.search_prompt);

	
	return 0;
}

int gfx_quit(void)
{
	AM_FONT_Destroy(gfx.def_font);
	AM_OSD_DestroySurface(gfx.bg_img);
	AM_OSD_DestroySurface(gfx.btn_search_focus_img);
	AM_OSD_DestroySurface(gfx.btn_search_img);
	AM_OSD_DestroySurface(gfx.btn_reset_focus_img);
	AM_OSD_DestroySurface(gfx.btn_reset_img);
	AM_OSD_DestroySurface(gfx.title_setting_img);
	AM_OSD_DestroySurface(gfx.title_search_img);
	AM_OSD_DestroySurface(gfx.confirm_img);
	AM_OSD_DestroySurface(gfx.cancel_img);
	AM_OSD_DestroySurface(gfx.inp_img);
	AM_OSD_DestroySurface(gfx.inp_focus_img);
	AM_OSD_DestroySurface(gfx.list_focus_img);
	AM_OSD_DestroySurface(gfx.progress_img);
	AM_OSD_DestroySurface(gfx.progress_value_img);
	AM_OSD_DestroySurface(gfx.volume_img);
	AM_OSD_DestroySurface(gfx.volume_value_img);
	AM_OSD_DestroySurface(gfx.search_frame_img);
	AM_OSD_DestroySurface(gfx.list_frame_img);
	AM_OSD_DestroySurface(gfx.info_frame_img);
	AM_OSD_DestroySurface(gfx.pop_win_img);
	
	return 0;
}

