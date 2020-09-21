/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 按钮控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_BUTTON_H
#define _AM_GUI_BUTTON_H

#include "am_gui_label.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_BUTTON(b) ((AM_GUI_Button_t*)(b))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 按钮控件*/
typedef struct
{
	AM_GUI_Label_t    label;  /**< 基本标签数据*/
} AM_GUI_Button_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新按钮控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] btn 返回新的按钮控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ButtonCreate(AM_GUI_t *gui, AM_GUI_Button_t **btn);

/**\brief 初始化一个按钮控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] btn 按钮控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ButtonInit(AM_GUI_t *gui, AM_GUI_Button_t *btn);

/**\brief 释放一个按钮控件内部相关资源
 * \param[in] btn 按钮控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ButtonRelease(AM_GUI_Button_t *btn);

/**\brief 按钮事件回调
 * \param[in] widget 按钮控件指针
 * \param[in] evt 要处理的控件事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ButtonEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt);

/**\brief 设定按钮控件显示的标题
 * \param[in] btn 按钮控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ButtonSetText(AM_GUI_Button_t *btn, const char *text);

/**\brief 分配一个带文本标题的按钮控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] text 按钮的标题
 * \param[out] btn 返回新的按钮控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TextButtonCreate(AM_GUI_t *gui, const char *text, AM_GUI_Button_t **btn);

#ifdef __cplusplus
}
#endif

#endif

