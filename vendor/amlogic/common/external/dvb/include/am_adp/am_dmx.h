/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file
 * \brief Demux module
 *
 * Basic data strucutres definition in "linux/dvb/dmx.h"
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#ifndef _AM_DMX_H
#define _AM_DMX_H

#include "am_types.h"
/*add for config define for linux dvb *.h*/
#include "am_config.h"
#include <linux/dvb/dmx.h>

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

/**\brief Error code of the demux module*/
enum AM_DMX_ErrorCode
{
	AM_DMX_ERROR_BASE=AM_ERROR_BASE(AM_MOD_DMX),
	AM_DMX_ERR_INVALID_DEV_NO,          /**< Invalid device number*/
	AM_DMX_ERR_INVALID_ID,              /**< Invalid filer handle*/
	AM_DMX_ERR_BUSY,                    /**< The device has already been openned*/
	AM_DMX_ERR_NOT_ALLOCATED,           /**< The device has not been allocated*/
	AM_DMX_ERR_CANNOT_CREATE_THREAD,    /**< Cannot create new thread*/
	AM_DMX_ERR_CANNOT_OPEN_DEV,         /**< Cannot open device*/
	AM_DMX_ERR_NOT_SUPPORTED,           /**< Not supported*/
	AM_DMX_ERR_NO_FREE_FILTER,          /**< No free filter*/
	AM_DMX_ERR_NO_MEM,                  /**< Not enough memory*/
	AM_DMX_ERR_TIMEOUT,                 /**< Timeout*/
	AM_DMX_ERR_SYS,                     /**< System error*/
	AM_DMX_ERR_NO_DATA,                 /**< No data received*/
	AM_DMX_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Input source of the demux*/
typedef enum
{
	AM_DMX_SRC_TS0,                    /**< TS input port 0*/
	AM_DMX_SRC_TS1,                    /**< TS input port 1*/
	AM_DMX_SRC_TS2,                    /**< TS input port 2*/
	AM_DMX_SRC_TS3,                    /**< TS input port 3*/
	AM_DMX_SRC_HIU,                     /**< HIU input (memory)*/
	AM_DMX_SRC_HIU1
} AM_DMX_Source_t;

/**\brief Demux device open parameters*/
typedef struct
{
	AM_Bool_t use_sw_filter;           /**< M_TURE to use DVR software filters.*/
	int       dvr_fifo_no;             /**< Async fifo number if use software filters.*/
	int       dvr_buf_size;            /**< Async fifo buffer size if use software filters.*/
} AM_DMX_OpenPara_t;

/**\brief Filter received data callback function
 * \a fandle is the filter's handle.
 * \a data is the received data buffer pointer.
 * \a len is the data length.
 * If \a data == null, means timeout.
 */
typedef void (*AM_DMX_DataCb) (int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Open a demux device
 * \param dev_no Demux device number
 * \param[in] para Demux device's open parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_Open(int dev_no, const AM_DMX_OpenPara_t *para);

/**\brief Close a demux device
 * \param dev_no Demux device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_Close(int dev_no);

/**\brief Allocate a filter
 * \param dev_no Demux device number
 * \param[out] fhandle Return the allocated filter's handle
 * \retval AM_SUCCESS On success
 * \return Error Code
 */
extern AM_ErrorCode_t AM_DMX_AllocateFilter(int dev_no, int *fhandle);

/**\brief Set a section filter's parameters
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \param[in] params Section filter's parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_SetSecFilter(int dev_no, int fhandle, const struct dmx_sct_filter_params *params);

/**\brief Set a PES filter's parameters
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \param[in] params PES filter's parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_SetPesFilter(int dev_no, int fhandle, const struct dmx_pes_filter_params *params);

/**\brief Release an unused filter
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_FreeFilter(int dev_no, int fhandle);

/**\brief Start filtering
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_StartFilter(int dev_no, int fhandle);

/**\brief Stop filtering
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_StopFilter(int dev_no, int fhandle);

/**\brief Set the ring queue buffer size of a filter
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \param size Ring queue buffer size in bytes
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_SetBufferSize(int dev_no, int fhandle, int size);

/**\brief Get a filter's data callback function
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \param[out] cb Return the data callback function of the filter
 * \param[out] data Return the user defined parameter of the callback function
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_GetCallback(int dev_no, int fhandle, AM_DMX_DataCb *cb, void **data);

/**\brief Set a filter's data callback function
 * \param dev_no Demux device number
 * \param fhandle Filter handle
 * \param[in] cb New data callback function
 * \param[in] data User defined parameter of the callback function
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_SetCallback(int dev_no, int fhandle, AM_DMX_DataCb cb, void *data);

/**\brief Set the demux's input source
 * \param dev_no Demux device number
 * \param src Input source
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_SetSource(int dev_no, AM_DMX_Source_t src);

/**\cond */
/**\brief Sync the demux data
 * \param dev_no Demux device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_Sync(int dev_no);
/**\endcond */

/**
 * Get the scramble status of the AV channels in the demux
 * \param dev_no Demux device number
 * \param[out] dev_status Return the AV channels's scramble status.
 * dev_status[0] is the video status, dev_status[1] is the audio status.
 * AM_TRUE means the channel is scrambled, AM_FALSE means the channel is not scrambled.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DMX_GetScrambleStatus(int dev_no, AM_Bool_t dev_status[2]);

#ifdef __cplusplus
}
#endif

#endif

