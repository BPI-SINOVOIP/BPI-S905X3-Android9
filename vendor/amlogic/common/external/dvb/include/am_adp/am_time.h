/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 时钟、时间相关函数
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#ifndef _AM_TIME_H
#define _AM_TIME_H

#include "am_types.h"
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#ifdef ANDROID
/*extern int  __pthread_cond_timedwait(pthread_cond_t*, 
                                     pthread_mutex_t*,
                                     const struct timespec*, 
                                     clockid_t);
#define pthread_cond_timedwait(c, m, a) __pthread_cond_timedwait(c, m, a, CLOCK_MONOTONIC);
*/
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 得到开机到当前系统运行的时间，单位为毫秒
 * \param[out] clock 返回系统运行时间
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_time.h)
 */
extern AM_ErrorCode_t AM_TIME_GetClock(int *clock);

/**\brief 得到开机到当前系统运行的时间，格式为struct timespec
 * \param[out] ts 返回当前timespec值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_time.h)
 */
extern AM_ErrorCode_t AM_TIME_GetTimeSpec(struct timespec *ts);

/**\brief 得到若干毫秒后的timespec值
 * 此函数主要用于pthread_cond_timedwait, sem_timedwait等函数计算超时时间。
 * \param timeout 以毫秒为单位的超时时间
 * \param[out] ts 返回timespec值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_time.h)
 */
extern AM_ErrorCode_t AM_TIME_GetTimeSpecTimeout(int timeout, struct timespec *ts);

#ifdef __cplusplus
}
#endif

#endif

