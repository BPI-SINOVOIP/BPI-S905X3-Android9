/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 网络管理模块内部头文件
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-29: create the document
 ***************************************************************************/

#ifndef _AM_NET_INTERNAL_H
#define _AM_NET_INTERNAL_H

#include <am_net.h>

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

/**\brief NET设备*/
typedef struct AM_NET_Device AM_NET_Device_t;


/**\brief 网络驱动*/
typedef struct 
{
	AM_ErrorCode_t (*open)(AM_NET_Device_t *dev);
	AM_ErrorCode_t (*close)(AM_NET_Device_t *dev);
	AM_ErrorCode_t (*get_status)(AM_NET_Device_t *dev, AM_NET_Status_t *status);
	AM_ErrorCode_t (*get_hw_addr)(AM_NET_Device_t *dev, AM_NET_HWAddr_t *addr);
	AM_ErrorCode_t (*set_hw_addr)(AM_NET_Device_t *dev, const AM_NET_HWAddr_t *addr);
	AM_ErrorCode_t (*get_para)(AM_NET_Device_t *dev, AM_NET_Para_t *para);
	AM_ErrorCode_t (*set_para)(AM_NET_Device_t *dev, int mask, AM_NET_Para_t *para);
}AM_NET_Driver_t;

/**\brief 网络设备*/
struct AM_NET_Device
{
	int              dev_no;         /**< 设备ID*/
	AM_Bool_t        openned;        /**< 设备是否已经打开*/
	const AM_NET_Driver_t  *drv;     /**< 网络驱动*/
	pthread_mutex_t  lock;           /**< 设备资源保护互斥体*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

