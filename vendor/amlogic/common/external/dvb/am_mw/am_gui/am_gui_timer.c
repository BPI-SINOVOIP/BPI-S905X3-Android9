#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief GUI定时器
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <string.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 初始化定时器
 * \param[in] gui GUI管理器
 * \param[in] timer 定时器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TimerInit(AM_GUI_t *gui, AM_GUI_Timer_t *timer)
{
	assert(gui && timer);
	
	timer->prev = NULL;
	timer->next = NULL;
	timer->gui  = gui;
	
	return AM_SUCCESS;
}

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
AM_ErrorCode_t AM_GUI_TimerStart(AM_GUI_Timer_t *timer, int delay,
		int interval, int times, AM_GUI_TimerCallback_t cb, void *data)
{
	assert(timer);
	
	AM_GUI_TimerStop(timer);
	
	timer->delay    = delay;
	timer->interval = interval;
	timer->times    = times;
	timer->cb       = cb;
	timer->data     = data;
	
	timer->next = timer->gui->timers;
	timer->prev = NULL;
	if(timer->gui->timers)
		timer->gui->timers->prev = timer;
	timer->gui->timers = timer;
	
	return AM_SUCCESS;
}

/**\brief 停止一个定时器
 * \param[in] timer 定时器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TimerStop(AM_GUI_Timer_t *timer)
{
	assert(timer);
	
	if(timer->prev && timer->next)
	{
		if(timer->prev)
			timer->prev->next = timer->next;
		else
			timer->gui->timers = timer->next;
		if(timer->next)
			timer->next->prev = timer->prev;
		timer->prev = NULL;
		timer->next = NULL;
	}
	
	return AM_SUCCESS;
}

