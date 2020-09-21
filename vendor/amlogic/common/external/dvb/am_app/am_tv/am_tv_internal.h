/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:

 */
/**\file am_tv_internal.h
 * \brief TV模块内部头文件
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-11: create the document
 ***************************************************************************/

#ifndef _AM_TV_INTERNAL_H
#define _AM_TV_INTERNAL_H

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

/**\brief TV内部数据*/
typedef struct
{
	int					status;		/**< 当前状态*/
	int 				fend_dev;	/**< 前端设备*/
	int					av_dev;		/**< 关联的音视频解码设备*/
	int 				chan_num;	/**< 当前节目的频道号*/
	int					srv_dbid;	/**< 当前节目在数据库中的索引*/
	int					db_tv;		/**< 记录正在播放的电视节目*/
	int					db_radio;	/**< 记录正在播放的广播节目*/
	sqlite3				*hdb;		/**< 数据库操作句柄*/
	pthread_mutex_t     lock;   	/**< 保护互斥体*/
	
	struct dvb_frontend_parameters fend_para; /**< 当前前端参数*/
}AM_TV_Data_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

