/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 选择框控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-03: create the document
 ***************************************************************************/

#ifndef _AM_GUI_SELECT_H
#define _AM_GUI_SELECT_H

#include "am_gui_widget.h"
#include "am_gui_list.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_SELECT(s)    ((AM_GUI_Select_t*)(s))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 选择框控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;  /**< 控件基本数据*/
} AM_GUI_Select_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新选择框控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] sel 返回新的选择框控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_SelectCreate(AM_GUI_t *gui, AM_GUI_Select_t **sel);

/**\brief 初始化一个选择框控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] sel 选择框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_SelectInit(AM_GUI_t *gui, AM_GUI_Select_t *sel);

/**\brief 释放一个选择框控件内部相关资源
 * \param[in] sel 选择框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_SelectRelease(AM_GUI_Select_t *sel);

/**\brief 释放一个选择框控件内部资源并释放内存
 * \param[in] sel 要释放的选择框控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_SelectDestroy(AM_GUI_Select_t *sel);

#ifdef __cplusplus
}
#endif

#endif

