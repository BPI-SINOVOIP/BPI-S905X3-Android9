/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 视频输出模块内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#ifndef _AM_VOUT_INTERNAL_H
#define _AM_VOUT_INTERNAL_H

#include <am_vout.h>
#include <am_thread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define VOUT_FL_RUN_CB   1

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct AM_VOUT_Driver AM_VOUT_Driver_t;
typedef struct AM_VOUT_Device AM_VOUT_Device_t;

/**\brief 视频输出驱动*/
struct AM_VOUT_Driver
{
	AM_ErrorCode_t (*open)(AM_VOUT_Device_t *dev, const AM_VOUT_OpenPara_t *para);
	AM_ErrorCode_t (*set_format)(AM_VOUT_Device_t *dev, AM_VOUT_Format_t fmt);
	AM_ErrorCode_t (*enable)(AM_VOUT_Device_t *dev, AM_Bool_t enable);
	AM_ErrorCode_t (*close)(AM_VOUT_Device_t *dev);
};

/**\brief 视频输出设备*/
struct AM_VOUT_Device
{
	int                      dev_no;   /**< 设备号*/
	const AM_VOUT_Driver_t  *drv;      /**< 设备驱动*/
	void                    *drv_data; /**< 驱动私有数据*/
	AM_VOUT_Format_t         format;   /**< 视频输出格式*/
	pthread_mutex_t          lock;     /**< 设备数据保护互斥体*/
	AM_Bool_t                openned;  /**< 设备是否已经打开*/
	AM_Bool_t                enable;   /**< 是否输出视频信号*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

