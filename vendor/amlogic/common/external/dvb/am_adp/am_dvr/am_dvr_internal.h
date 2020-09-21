/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief DVR设备内部头文件
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-13: create the document
 ***************************************************************************/

#ifndef _AM_DVR_INTERNAL_H
#define _AM_DVR_INTERNAL_H

#include <am_dvr.h>
#include <am_util.h>
#include <am_thread.h>

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
 
/**\brief DVR设备*/
typedef struct AM_DVR_Device AM_DVR_Device_t;


typedef struct
{
	int			fid;	/**< DMX Filter ID*/
	uint16_t	pid;	/**< Stream PID*/
}AM_DVR_Stream_t;

/**\brief DVR设备驱动*/
typedef struct
{
	AM_ErrorCode_t (*open)(AM_DVR_Device_t *dev, const AM_DVR_OpenPara_t *para);
	AM_ErrorCode_t (*close)(AM_DVR_Device_t *dev);
	AM_ErrorCode_t (*set_buf_size)(AM_DVR_Device_t *dev, int size);
	AM_ErrorCode_t (*poll)(AM_DVR_Device_t *dev, int timeout);
	AM_ErrorCode_t (*read)(AM_DVR_Device_t *dev, uint8_t *buf, int *size);
	AM_ErrorCode_t (*set_source)(AM_DVR_Device_t *dev, AM_DVR_Source_t src);
} AM_DVR_Driver_t;

/**\brief DVR设备*/
struct AM_DVR_Device
{
	int                 dev_no;  /**< 设备号*/
	int					dmx_no;	 /**< DMX设备号*/
	const AM_DVR_Driver_t *drv;  /**< 设备驱动*/
	void               *drv_data;/**< 驱动私有数据*/
	int                 open_cnt;   /**< 设备打开计数*/
	int					stream_cnt;	/**< 流个数*/
	AM_DVR_Stream_t		streams[AM_DVR_MAX_PID_COUNT]; /**< Streams*/
	AM_Bool_t	   		record;/**< 是否正在录像标志*/
	pthread_mutex_t     	lock;    /**< 设备保护互斥体*/
	AM_DVR_StartRecPara_t	start_para;	/**< 启动参数*/
	AM_DVR_Source_t		src;			/**< 源数据*/
};



/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

