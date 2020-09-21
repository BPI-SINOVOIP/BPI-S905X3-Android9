/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:

 */
/**\file am_tv.h
 * \brief TV模块头文件
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-11-11: create the document
 ***************************************************************************/

#ifndef _AM_TV_H
#define _AM_TV_H

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

typedef void* AM_TV_Handle_t;

/**\brief TV模块错误代码*/
enum AM_TV_ErrorCode
{
	AM_TV_ERROR_BASE=AM_ERROR_BASE(AM_MOD_TV),
	AM_TV_ERR_INVALID_HANDLE,          /**< 句柄无效*/
	AM_TV_ERR_NOT_SUPPORTED,           /**< 不支持的操作*/
	AM_TV_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_TV_ERR_NO_CHANNEL,			   /**< 当前没有可用播放频道*/
	AM_TV_ERR_INVALID_CHAN_NUM,		   /**< 频道号无效*/
	AM_TV_ERR_END
};

/**\brief TV状态定义*/
typedef enum
{
	AM_TV_ST_WAIT_FEND	= 0x01,		/**< 等待前端状态*/
	AM_TV_ST_LOCKED		= 0x02,  	/**< 为1表示前端已锁定，为0时表示未锁定*/
	AM_TV_ST_PLAYING	= 0x04,		/**< 为1表示正在播放，该位为0时表示未播放*/

	/*以下全为播放具体节目类型，其他状态置于上面*/
	AM_TV_ST_PLAY_TV	= 0x10000,	/**< 当前播放节目为电视节目*/
	AM_TV_ST_PLAY_RADIO	= 0x20000	/**< 当前播放节目为广播节目*/
}AM_TV_Status_t;

/**\brief 频道类型定义*/
typedef enum
{
	AM_TV_CHTYPE_TV,		/**< 电视频道*/
	AM_TV_CHTYPE_RADIO		/**< 广播频道*/
}AM_TV_ChannelType_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 创建一个TV设备
 * \param fend_dev 前端设备
 * \param av_dev 与该TV关联的音视频解码设备
 * \param [in] hdb 数据库操作句柄
 * \param [out] handle 返回TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_Create(int fend_dev, int av_dev, sqlite3 *hdb, AM_TV_Handle_t *handle);

/**\brief 销毁一个TV设备
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_Destroy(AM_TV_Handle_t handle);

/**\brief 播放下一个频道(频道+)
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_ChannelUp(AM_TV_Handle_t handle);

/**\brief 播放上一个频道(频道-)
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_ChannelDown(AM_TV_Handle_t handle);

/**\brief 播放指定频道
 * \param handle TV句柄
 * \param chan_num 频道号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_PlayChannel(AM_TV_Handle_t handle, int chan_num);

/**\brief 播放当前停止的频道,如未播放任何频道，则播放一个频道号最小的频道
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_Play(AM_TV_Handle_t handle);

/**\brief 停止当前播放
 * \param handle TV句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_StopPlay(AM_TV_Handle_t handle);


/**\brief 获取当前频道
 * \param handle TV句柄
 * \param [out] srv_dbid 当前频道的service在数据库中的唯一索引
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tv.h)
 */
extern AM_ErrorCode_t AM_TV_GetCurrentChannel(AM_TV_Handle_t handle, int *srv_dbid);

#ifdef __cplusplus
}
#endif

#endif

