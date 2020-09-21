/***************************************************************************
   * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief DTV头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#ifndef _AM_DTV_H
#define _AM_DTV_H

#include <linux/dvb/frontend.h>
#include <am_types.h>
#include <am_inp.h>
#include <am_fend.h>
#include <am_osd.h>
#include <am_dmx.h>
#include <am_av.h>
#include <am_font.h>
#include <am_img.h>
#include <am_aout.h>
#include <am_time.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DTV_INP_DEV_NO   0
#define DTV_AV_DEV_NO    0
#define DTV_FEND_DEV_NO  1
#define DTV_DMX_DEV_NO   0
#define DTV_OSD_DEV_NO   0
#define DTV_AV_DEV_NO    0
#define DTV_AOUT_DEV_NO  0

#define DTV_PROG_COUNT (64)
#define DTV_PROG_NAME_LEN (256)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct
{
	fe_type_t       type;
	int             frequency;
	int             symbol_rate;
	fe_modulation_t modulation;
} DTV_TS_t;

typedef struct
{
	int             video_pid;
	int             audio_pid;
	int				chan_num;
	char			name[DTV_PROG_NAME_LEN];
} DTV_Program_t;

typedef struct DTV_Window DTV_Window_t;

/**\brief 窗口*/
struct DTV_Window
{
	AM_Bool_t       show;               /**< 窗口是否显示*/
	int (*init) (DTV_Window_t *win);    /**< 初始化窗口*/
	int (*update) (DTV_Window_t *win, AM_OSD_Surface_t *s); /**< 绘制窗口*/
	int (*on_key) (DTV_Window_t *win, AM_INP_Key_t key);    /**< 处理按键，如果按键被处理，返回1，否则返回0*/
	int (*on_timer) (DTV_Window_t *win);                    /**< 定时器处理，如果触发了定时器，返回1，否则返回0*/
	int (*on_show) (DTV_Window_t *win); /**< 当窗口显示时被调用*/
	int (*on_hide) (DTV_Window_t *win); /**< 当窗口隐藏时被调用*/
	int (*quit) (DTV_Window_t *win);    /**< 释放窗口资源*/
};

typedef struct
{
	int               def_font;
	AM_OSD_Surface_t *bg_img;
	AM_OSD_Surface_t *title_setting_img;
	AM_OSD_Surface_t *title_search_img;
	AM_OSD_Surface_t *btn_search_img;
	AM_OSD_Surface_t *btn_search_focus_img;
	AM_OSD_Surface_t *btn_reset_img;
	AM_OSD_Surface_t *btn_reset_focus_img;
	AM_OSD_Surface_t *confirm_img;
	AM_OSD_Surface_t *cancel_img;
	AM_OSD_Surface_t *inp_img;
	AM_OSD_Surface_t *inp_focus_img;
	AM_OSD_Surface_t *list_focus_img;
	AM_OSD_Surface_t *progress_img;
	AM_OSD_Surface_t *progress_value_img;
	AM_OSD_Surface_t *volume_img;
	AM_OSD_Surface_t *volume_value_img;
	AM_OSD_Surface_t *search_frame_img;
	AM_OSD_Surface_t *list_frame_img;
	AM_OSD_Surface_t *info_frame_img;
	AM_OSD_Surface_t *pop_win_img;
	AM_OSD_Surface_t *search_prompt;
} DTV_Gfx_t;

/****************************************************************************
 * Static data
 ***************************************************************************/

extern DTV_Gfx_t     gfx;
extern struct dvb_frontend_parameters     ts_param;
extern DTV_Program_t programs[DTV_PROG_COUNT];
extern int           program_count;
extern int           curr_program;

extern DTV_Window_t *main_menu_win;
extern DTV_Window_t *dvbc_param_win;
extern DTV_Window_t *dvbt_param_win;
extern DTV_Window_t *scan_progress_win;
extern DTV_Window_t *scan_result_win;
extern DTV_Window_t *restore_win;
extern DTV_Window_t *prog_info_win;
extern DTV_Window_t *prog_list_win;
extern DTV_Window_t *vol_win;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

extern int gfx_init(void);
extern int gfx_quit(void);
extern int dtv_program_init(void);
extern int dtv_program_quit(void);
extern int dtv_program_get_info_by_chan_num(int chan_num, DTV_Program_t *info);
/**\brief 获取当前播放节目的信息*/
extern int dtv_program_get_curr_info(DTV_Program_t *info);
/**\brief 播放当前节目*/
extern int dtv_program_play(void);

extern int dtv_program_play_by_channel_no(int channel_no);
/**\brief 停止播放当前节目*/
extern int dtv_program_stop(void);
/**\brief 播放下一个频道*/
extern int dtv_program_channel_up(void);
/**\brief 播放上一个频道*/
extern int dtv_program_channel_down(void);
extern int dtv_window_init(DTV_Window_t *win);
extern int dtv_window_show(DTV_Window_t *win);
extern int dtv_window_hide(DTV_Window_t *win);
extern int dtv_window_update(DTV_Window_t *win, AM_OSD_Surface_t *surface);
extern int dtv_window_on_key(DTV_Window_t *win, AM_INP_Key_t key);
extern int dtv_window_on_timer(DTV_Window_t *win);
extern int dtv_window_quit(DTV_Window_t *win);

#ifdef __cplusplus
}
#endif

#endif

