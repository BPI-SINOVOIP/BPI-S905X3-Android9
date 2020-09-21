/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 窗口控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_WINDOW_H
#define _AM_GUI_WINDOW_H

#include "am_gui_widget.h"
#include "am_gui_label.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_WINDOW(w)  ((AM_GUI_Window_t*)(w))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 窗口控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;  /**< 控件基本数据*/
	AM_GUI_Label_t    *caption; /**< 窗口标题*/
	AM_GUI_Widget_t   *focus;   /**< 窗口内的焦点控件*/
} AM_GUI_Window_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新窗口控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] win 返回新的窗口控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WindowCreate(AM_GUI_t *gui, AM_GUI_Window_t **win);

/**\brief 初始化一个窗口控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] win 窗口控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WindowInit(AM_GUI_t *gui, AM_GUI_Window_t *win);

/**\brief 释放一个窗口内部相关资源
 * \param[in] win 窗口控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WindowRelease(AM_GUI_Window_t *win);

/**\brief 窗口控件事件回调
 * \param[in] widget 窗口控件
 * \param[in] evt 要处理的事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WindowEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt);

/**\brief 设定窗口标题
 * \param[in] win 窗口控件
 * \param caption 标题文本
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WindowSetCaption(AM_GUI_Window_t *win, const char *caption);

/**\brief 设定窗口中的一个控件为当前焦点
 * \param[in] win 窗口控件
 * \param[in] focus 当前焦点控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WindowSetFocus(AM_GUI_Window_t *win, AM_GUI_Widget_t *focus);

#ifdef __cplusplus
}
#endif

#endif

