/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief GUI模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-22: create the document
 ***************************************************************************/

#ifndef _AM_GUI_H
#define _AM_GUI_H

#include "gui/am_gui_theme.h"
#include "gui/am_gui_widget.h"
#include "gui/am_gui_timer.h"
#include "gui/am_gui_flow.h"
#include "gui/am_gui_label.h"
#include "gui/am_gui_button.h"
#include "gui/am_gui_input.h"
#include "gui/am_gui_list.h"
#include "gui/am_gui_progress.h"
#include "gui/am_gui_scroll.h"
#include "gui/am_gui_select.h"
#include "gui/am_gui_desktop.h"
#include "gui/am_gui_box.h"
#include "gui/am_gui_table.h"
#include "gui/am_gui_window.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief GUI模块错误代码*/
enum AM_GUI_ErrorCode
{
	AM_GUI_ERROR_BASE=AM_ERROR_BASE(AM_MOD_GUI),
	AM_GUI_ERR_NO_MEM,          /**< 内存不足*/
	AM_GUI_ERR_UNKNOWN_EVENT,   /**< 控件不处理此事件*/
	AM_GUI_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief GUI初始化参数*/
typedef struct
{
	int                osd_dev_no;  /**< GUI对应的OSD设备ID*/
	int                inp_dev_no;  /**< GUI对应的输入设备ID*/
} AM_GUI_CreatePara_t;

/**\brief GUI管理器*/
struct AM_GUI
{
	AM_GUI_Timer_t    *timers;      /**< 已经开始运行的定时器链表*/
	AM_GUI_Widget_t   *desktop;     /**< 桌面控件*/
	AM_OSD_Surface_t  *surface;     /**< GUI绘图表面*/
	AM_OSD_Rect_t      update_rect; /**< 需要更新的区域*/
	int                osd_dev_no;  /**< OSD设备ID*/
	int                inp_dev_no;  /**< 输入设备ID*/
	AM_Bool_t          loop;        /**< 主循环正在运行*/
	void              *theme_data;  /**< 主题相关数据*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 创建一个GUI管理器
 * \param[in] para GUI创建参数
 * \param[out] gui 返回新的GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_Create(const AM_GUI_CreatePara_t *para, AM_GUI_t **gui);

/**\brief 释放一个GUI管理器
 * \param[in] gui GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_Destroy(AM_GUI_t *gui);

/**\brief 进入GUI管理器主循环
 * \param[in] gui GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_Main(AM_GUI_t *gui);

/**\brief 退出GUI管理器主循环
 * \param[in] gui GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_Exit(AM_GUI_t *gui);

/**\brief 取得GUI中的桌面控件
 * \param[in] gui GUI管理器
 * \param[out] desktop 返回桌面控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_GetDesktop(AM_GUI_t *gui, AM_GUI_Widget_t **desktop);

#ifdef __cplusplus
}
#endif

#endif

