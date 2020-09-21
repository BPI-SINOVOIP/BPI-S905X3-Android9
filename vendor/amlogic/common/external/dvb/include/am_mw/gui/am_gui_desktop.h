/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 桌面控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#ifndef _AM_GUI_DESKTOP_H
#define _AM_GUI_DESKTOP_H

#include "am_gui_widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_DESKTOP(d)    ((AM_GUI_Desktop_t*)(d))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 桌面控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;   /**< 基本控件数据*/
	uint32_t           bg_pixel; /**< 背景填充像素值*/
} AM_GUI_Desktop_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新桌面控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] d 返回新的桌面控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_DesktopCreate(AM_GUI_t *gui, AM_GUI_Desktop_t **d);

/**\brief 初始化一个桌面控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] d 桌面控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_DesktopInit(AM_GUI_t *gui, AM_GUI_Desktop_t *d);

/**\brief 释放一个桌面控件内部相关资源
 * \param[in] d 桌面控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_DesktopRelease(AM_GUI_Desktop_t *d);

#ifdef __cplusplus
}
#endif

#endif

