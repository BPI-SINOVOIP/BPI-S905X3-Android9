/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 输入模块内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-20: create the document
 ***************************************************************************/

#ifndef _AM_INP_INTERNAL_H
#define _AM_INP_INTERNAL_H

#include <am_inp.h>
#include <am_thread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define INP_FL_RUN_CB        (1)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 输入设备数据*/
typedef struct AM_INP_Device AM_INP_Device_t;

/**\brief 输入设备驱动接口*/
typedef struct
{
	AM_ErrorCode_t (*open) (AM_INP_Device_t *dev, const AM_INP_OpenPara_t *para); /**< 打开一个设备*/
	AM_ErrorCode_t (*close) (AM_INP_Device_t *dev);   /**< 关闭一个设备*/
	AM_ErrorCode_t (*wait) (AM_INP_Device_t *dev, struct input_event *event, int timeout); /**< 等待输入事件*/
} AM_INP_Driver_t;

/**\brief 输入设备数据*/
struct AM_INP_Device
{
	int              dev_no;        /**< 输入设备号*/
	const AM_INP_Driver_t *drv;     /**< 驱动指针*/
	void            *drv_data;      /**< 驱动私有数据*/
	AM_Bool_t        openned;       /**< 设备已经打开*/
	AM_Bool_t        enable;        /**< 使能/禁用标志*/
	AM_Bool_t        enable_thread; /**< 是否创建线程*/
	AM_Bool_t        enable_map;    /**< 是否使用键码映射表*/
	pthread_t        event_thread;  /**< 事件线程*/
	AM_INP_EventCb_t event_cb;      /**< 事件线程回调函数*/
	void            *user_data;     /**< 回调用户参数*/
	int              flags;         /**< 事件检测线程标志*/
	pthread_mutex_t  lock;          /**< 设备资源互斥体*/
	pthread_cond_t   cond;          /**< 条件变量*/
	AM_INP_KeyMap_t  map;           /**< 键码映射表*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

