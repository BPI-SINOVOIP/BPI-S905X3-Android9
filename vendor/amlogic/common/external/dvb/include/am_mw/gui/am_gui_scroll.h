/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 滚动条控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_SCROLL_H
#define _AM_GUI_SCROLL_H

#include "am_gui_widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_SCROLL(s)  ((AM_GUI_Scroll_t*)(s))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 滚动条控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;  /**< 控件基本数据*/
	AM_GUI_Direction_t dir;     /**< 滚动条方向*/
	int                min;     /**< 滚动范围最小值*/
	int                max;     /**< 滚动范围最大值*/
	int                page;    /**< 滚动页值*/
	int                value;   /**< 滚动条当前值*/
} AM_GUI_Scroll_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个滚动条控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param dir 滚动条方向
 * \param[out] scroll 返回新的滚动条控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ScrollCreate(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Scroll_t **scroll);

/**\brief 初始化一个滚动条控件
 * \param[in] gui 控件所属的GUI控制器
 * \param dir 滚动条方向
 * \param[in] scroll 滚动条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ScrollInit(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Scroll_t *scroll);

/**\brief 释放一个滚动条控件内部相关资源
 * \param[in] scroll 滚动条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ScrollRelease(AM_GUI_Scroll_t *scroll);

/**\brief 设定滚动条的取值范围
 * \param[in] scroll 滚动条控件
 * \param min 滚动条的最小值
 * \param max 滚动条的最大值
 * \param page 滚动条的页值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ScrollSetRange(AM_GUI_Scroll_t *scroll, int min, int max, int page);

/**\brief 设定滚动条的当前值
 * \param[in] scroll 滚动条控件
 * \param value 滚动条当前值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ScrollSetValue(AM_GUI_Scroll_t *scroll, int value);

#ifdef __cplusplus
}
#endif

#endif

