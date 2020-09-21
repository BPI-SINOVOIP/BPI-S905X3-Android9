/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 布局框
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#ifndef _AM_GUI_BOX_H
#define _AM_GUI_BOX_H

#include "am_gui_widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_BOX(b)  ((AM_GUI_Box_t*)(b))

/**\brief 子控件扩展*/
#define AM_GUI_BOX_FL_EXPAND   1

/**\brief 子控件以百分比表示大小*/
#define AM_GUI_BOX_FL_PERCENT  2

/**\brief 设定控件大小*/
#define AM_GUI_BOX_FL_SIZE     4

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 布局框*/
typedef struct
{
	AM_GUI_Widget_t    widget;  /**< 基本控件数据*/
	AM_GUI_Direction_t dir;     /**< 布局方向*/
} AM_GUI_Box_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新布局框控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] box 返回新的布局框控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_BoxCreate(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Box_t **box);

/**\brief 初始化一个布局框控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] box 布局框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_BoxInit(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Box_t *box);

/**\brief 释放一个布局框控件内部相关资源
 * \param[in] box 布局框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_BoxRelease(AM_GUI_Box_t *box);

/**\brief 向布局框中添加一个控件
 * \param[in] box 布局框控件指针
 * \param[in] widget 添加的控件
 * \param flags 控件宽度标志
 * \param size 子控件的大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_BoxAppend(AM_GUI_Box_t *box, AM_GUI_Widget_t *widget, int flags, int size);

#ifdef __cplusplus
}
#endif

#endif

