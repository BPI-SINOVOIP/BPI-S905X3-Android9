/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 进度条控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_PROGRESS_H
#define _AM_GUI_PROGRESS_H

#include "am_gui_widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_PROGRESS(i)  ((AM_GUI_Progress_t*)(i))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 进度条控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;  /**< 控件基本数据*/
	AM_GUI_Direction_t dir;     /**< 进度条显示方向*/
	int                min;     /**< 进度条最小值*/
	int                max;     /**< 进度条最大值*/
	int                value;   /**< 进度条当前值*/
} AM_GUI_Progress_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新进度条控件并初始化
 * \param[in] gui 进度条控件所属的GUI控制器
 * \param dir 进度条显示方向
 * \param[out] prog 返回新的进度条控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ProgressCreate(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Progress_t **prog);

/**\brief 初始化一个进度条控件
 * \param[in] gui 控件所属的GUI控制器
 * \param dir 进度条显示方向
 * \param[in] prog 进度条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ProgressInit(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Progress_t *prog);

/**\brief 释放一个进度条控件内部相关资源
 * \param[in] prog 进度条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ProgressRelease(AM_GUI_Progress_t *prog);

/**\brief 设定进度条的取值范围
 * \param[in] prog 要释放的进度条控件
 * \param min 进度条最小值
 * \param max 进度条最大值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ProgressSetRange(AM_GUI_Progress_t *prog, int min, int max);

/**\brief 设定进度条的当前值
 * \param[in] prog 要释放的进度条控件
 * \param value 进度条值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ProgressSetValue(AM_GUI_Progress_t *prog, int value);

#ifdef __cplusplus
}
#endif

#endif

