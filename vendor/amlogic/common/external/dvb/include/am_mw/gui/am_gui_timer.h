/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief GUI 定时器
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_TIMER_H
#define _AM_GUI_TIMER_H

#include <am_types.h>
#include "am_gui_widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief GUI 定时器*/
typedef struct AM_GUI_Timer AM_GUI_Timer_t;

/**\brief 定时器回调函数*/
typedef AM_ErrorCode_t (*AM_GUI_TimerCallback_t)(AM_GUI_Timer_t *timer, void *data);

/**\brief GUI 定时器*/
struct AM_GUI_Timer
{
	AM_GUI_t          *gui;         /**< 定时器所属的GUI管理器*/
	AM_GUI_Timer_t    *prev;        /**< 已运行定时器链表中的前一个定时器*/
	AM_GUI_Timer_t    *next;        /**< 已运行定时器链表中的后一个定时器*/
	int                start_time;  /**< 定时器开始运行时间*/
	int                delay;       /**< 首次触发的延时时间(毫秒)*/
	int                interval;    /**< 定时器触发时间间隔(毫秒)*/
	int                times;       /**< 触发的次数，<=0表示一直触发*/
	AM_GUI_TimerCallback_t cb;      /**< 回调函数*/
	void              *data;        /**< 用户注册参数*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 初始化定时器
 * \param[in] gui GUI管理器
 * \param[in] timer 定时器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TimerInit(AM_GUI_t *gui, AM_GUI_Timer_t *timer);

/**\brief 开始运行定时器
 * \param[in] timer 定时器
 * \param delay 首次触发的延时时间(毫秒)
 * \param interval 定时器触发时间间隔(毫秒)
 * \param times 触发的次数，<=0表示一直触发
 * \param cb 触发时的回调函数
 * \param data 传递给回调的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TimerStart(AM_GUI_Timer_t *timer, int delay,
		int interval, int times, AM_GUI_TimerCallback_t cb, void *data);

/**\brief 停止一个定时器
 * \param[in] timer 定时器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TimerStop(AM_GUI_Timer_t *timer);

#ifdef __cplusplus
}
#endif

#endif

