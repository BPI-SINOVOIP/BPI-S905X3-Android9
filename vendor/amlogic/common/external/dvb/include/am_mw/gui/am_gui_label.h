/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 标签控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_LABEL_H
#define _AM_GUI_LABEL_H

#include "am_gui_widget.h"
#include "am_gui_flow.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_LABEL(w) ((AM_GUI_Label_t*)(w))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 标签控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;  /**< 基本控件信息*/
	AM_GUI_Flow_t      flow;    /**< 控件内容流对象*/
} AM_GUI_Label_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 标签控件事件回调
 * \param[in] widget 标签控件
 * \param[in] evt 事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_LabelEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt);

/**\brief 分配一个新标签控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] lab 返回新的标签控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_LabelCreate(AM_GUI_t *gui, AM_GUI_Label_t **lab);

/**\brief 初始化一个标签控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] lab 标签控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_LabelInit(AM_GUI_t *gui, AM_GUI_Label_t *lab);

/**\brief 释放一个标签控件内部相关资源
 * \param[in] lab 标签控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_LabelRelease(AM_GUI_Label_t *lab);

/**\brief 设定标签控件的文本
 * \param[in] lab 标签控件
 * \param[in] text 文本字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_LabelSetText(AM_GUI_Label_t *lab, const char *text);

/**\brief 设定标签控件的图片
 * \param[in] lab 标签控件
 * \param[in] image 图片数据
 * \param need_free 控件释放时是否释放图片
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_LabelSetImage(AM_GUI_Label_t *lab, AM_OSD_Surface_t *image, AM_Bool_t need_free);

/**\brief 设定标签的多行显示模式
 * \param[in] lab 标签控件
 * \param multiline 是否使用多行显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_LabelSetMultiline(AM_GUI_Label_t *lab, AM_Bool_t multiline);

/**\brief 分配一个文字标签控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] text 文字数据
 * \param[out] lab 返回新的标签控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TextLabelCreate(AM_GUI_t *gui, const char *text, AM_GUI_Label_t **lab);

/**\brief 分配一个图片标签控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] image 图片
 * \param[out] lab 返回新的标签控件
 * \param need_free 释放控件时是否释放图片
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ImageLabelCreate(AM_GUI_t *gui, AM_OSD_Surface_t *image, AM_GUI_Label_t **lab, AM_Bool_t need_free);

#ifdef __cplusplus
}
#endif

#endif

