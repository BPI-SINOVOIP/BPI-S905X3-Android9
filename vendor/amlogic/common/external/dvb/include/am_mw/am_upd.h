/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_upd.h
 * \brief Update头文件
 *
 * \author 
 * \date 
 ***************************************************************************/
 
#ifndef _AM_UPD_H_
#define _AM_UPD_H_

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif
/****************************************************************************
 * Macro definitions
 ***************************************************************************/
 
 
/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief UPD模块错误代码*/
enum AM_UPD_ErrorCode
{
	AM_UPD_ERROR_BASE=AM_ERROR_BASE(AM_MOD_UPD),
	AM_UPD_ERROR_BADPARA,                        /**<参数错误*/
	AM_UPD_ERROR_BADID,                          /**<ID错误*/
	AM_UPD_ERROR_BADSTAT,                        /**<状态错误*/
	AM_UPD_ERROR_NOMEM,                          /**<内存错误*/
	AM_UPD_ERROR_END                                
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef void* AM_TSUPD_MonHandle_t;
typedef void* AM_TSUPD_DlHandle_t;

/**\brief UPD Monitor 打开参数*/
typedef struct {
	int dmx;    /**<dmx id for the monitor*/
}AM_TSUPD_OpenMonitorParam_t;

/**\brief UPD Monitor启动参数*/
typedef struct {
	int (*callback)(unsigned char* p_nit, unsigned int len, void *user); /**<nit数据回调。首次启动monitor模块或者nit发生变化时，monitor会回调此接口*/
	void *callback_args;                     /**<回调参数。用户参数，monitor不会修改*/
	int use_ext_nit;                         /**<该参数置1时，通知monitor模块使用外部注入nit，monitor内部停止nit获取*/
	int timeout;                             /*超时设置。0表示使用默认值。单位毫秒*/
	int nit_check_interval;                  /**<nit检查间隔，0表示使用默认值。单位毫秒*/
}AM_TSUPD_MonitorParam_t;

/**\brief UPD Downloader 打开参数*/
typedef struct {
	int dmx;   /**<为Downloader指定dmx id*/
}AM_TSUPD_OpenDownloaderParam_t;

/**\brief UPD Downloader 启动参数*/
typedef struct {
	int (*callback)(unsigned char* p_data, unsigned int len, void *user);  /**<数据回调。Downloader在数据接收完成时回调此接口*/
	void *callback_args;  /**<回调参数。用户参数，Downloader不会修改*/
	int pid;              /**<接收数据的pid*/
	int tableid;          /**<接收数据的table id*/
	int ext;              /**<接受数据的ext信息*/

	int timeout;          /**<接收数据的超时设置，0表示使用默认值。单位毫秒*/
}AM_TSUPD_DownloaderParam_t;

/**\brief 打开UPD Monitor模块
 * \param [in] para 打开参数
 * \param [out] mid Monitor模块句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_OpenMonitor(AM_TSUPD_OpenMonitorParam_t *para, AM_TSUPD_MonHandle_t *mid);

/**\brief 启动UPD Monitor模块
 * \param [in] mid  Monitor句柄
 * \param [in] para 启动参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_StartMonitor(AM_TSUPD_MonHandle_t mid, AM_TSUPD_MonitorParam_t *para);

/**\brief 停止UPD Monitor模块
 * \param [in] mid  Monitor句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_StopMonitor(AM_TSUPD_MonHandle_t mid);

/**\brief 外部注入NIT，见AM_TSUPD_MonitorParam_t:use_ext_nit
 * \param [in] mid  Monitor句柄
 * \param [in] p_nit nit 数据指针
 * \param [in] len nit数据长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_InjectNIT(AM_TSUPD_MonHandle_t mid, unsigned char* p_nit, unsigned int len);

/**\brief 关闭UPD Monitor模块
 * \param [in] mid  Monitor句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_CloseMonitor(AM_TSUPD_MonHandle_t mid);

/**\brief 打开UPD Downloader模块
 * \param [in] para 打开参数
 * \param [out] did  Downloader句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_OpenDownloader(AM_TSUPD_OpenDownloaderParam_t *para, AM_TSUPD_DlHandle_t *did);

/**\brief 启动UPD Downloader模块
 * \param [in] did  Downloader句柄
 * \param [in] para 启动参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_StartDownloader(AM_TSUPD_DlHandle_t did, AM_TSUPD_DownloaderParam_t *para);

/**\brief 获得UPD Downloader 数据
 * \param [in] did  Downloader句柄
 * \param [inout] ppdata 数据指针
 * \param [inout] len    数据长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_GetDownloaderData(AM_TSUPD_DlHandle_t did, unsigned char **ppdata, unsigned int *len);

/**\brief 停止UPD Downloader模块
 * \param [in] did  Downloader句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_StopDownloader(AM_TSUPD_DlHandle_t did);

/**\brief 关闭UPD Downloader模块
 * \param [in] did  Downloader句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_upd.h)
 */
AM_ErrorCode_t AM_TSUPD_CloseDownloader(AM_TSUPD_DlHandle_t did);


#ifdef __cplusplus
}
#endif

#endif

