/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 解扰器设备内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#ifndef _AM_DSC_INTERNAL_H
#define _AM_DSC_INTERNAL_H

#include <am_types.h>
#include <am_dsc.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DSC_CHANNEL_COUNT    (8)


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 解扰器设备*/
typedef struct AM_DSC_Device  AM_DSC_Device_t;

/**\brief 解扰器驱动*/
typedef struct AM_DSC_Driver  AM_DSC_Driver_t;

/**\brief 解扰器通道*/
typedef struct AM_DSC_Channel AM_DSC_Channel_t;

/**\brief 解扰器驱动*/
struct AM_DSC_Driver
{
	AM_ErrorCode_t (*open) (AM_DSC_Device_t *dev, const AM_DSC_OpenPara_t *para);
	AM_ErrorCode_t (*alloc_chan) (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan);
	AM_ErrorCode_t (*free_chan) (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan);
	AM_ErrorCode_t (*set_pid) (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan, uint16_t pid);
	AM_ErrorCode_t (*set_key) (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan, AM_DSC_KeyType_t type, const uint8_t *key);
	AM_ErrorCode_t (*set_source) (AM_DSC_Device_t *dev, AM_DSC_Source_t src);
	AM_ErrorCode_t (*set_mode) (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan,int mode);
	AM_ErrorCode_t (*close) (AM_DSC_Device_t *dev);
};

/**\brief 解扰器通道*/
struct AM_DSC_Channel
{
	int		id;
	uint16_t	pid;
	/* channel handle in secure os. */
	uint8_t		stream_path;
	AM_Bool_t          used;
	void              *drv_data;
	int 				mode;
};

/**\brief 解扰器设备*/
struct AM_DSC_Device
{
	int                dev_no;
	const AM_DSC_Driver_t   *drv;
	void              *drv_data;
	AM_DSC_Channel_t   channels[DSC_CHANNEL_COUNT];
	AM_Bool_t          openned;
	pthread_mutex_t    lock;
};

/****************************************************************************
 * Function prototypes
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

