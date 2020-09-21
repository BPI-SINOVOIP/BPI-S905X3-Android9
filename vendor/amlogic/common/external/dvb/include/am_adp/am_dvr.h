/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief DVR module
 *
 * DVR module use asyncfifo to record TS stream from a demux.
 * Each demux has a corresponding DVR device.
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-13: create the document
 ***************************************************************************/

#ifndef _AM_DVR_H
#define _AM_DVR_H

#include <unistd.h>
#include <sys/types.h>
#include <am_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_DVR_MAX_PID_COUNT  (32)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief DVR module's error code*/
enum AM_DVR_ErrorCode
{
	AM_DVR_ERROR_BASE=AM_ERROR_BASE(AM_MOD_DVR),
	AM_DVR_ERR_INVALID_ARG,			/**< Invalid argument*/
	AM_DVR_ERR_INVALID_DEV_NO,		/**< Invalid decide number*/
	AM_DVR_ERR_BUSY,                        /**< The device has already been openned*/
	AM_DVR_ERR_NOT_ALLOCATED,           /**< The device has not been allocated*/
	AM_DVR_ERR_CANNOT_CREATE_THREAD,    /**< Cannot create a new thread*/
	AM_DVR_ERR_CANNOT_OPEN_DEV,         /**< Cannot open the device*/
	AM_DVR_ERR_NOT_SUPPORTED,           /**< Not supported*/
	AM_DVR_ERR_NO_MEM,                  /**< Not enough memory*/
	AM_DVR_ERR_TIMEOUT,                 /**< Timeout*/
	AM_DVR_ERR_SYS,                     /**< System error*/
	AM_DVR_ERR_NO_DATA,                 /**< No data received*/
	AM_DVR_ERR_CANNOT_OPEN_OUTFILE,		/**< Cannot open the output file*/
	AM_DVR_ERR_TOO_MANY_STREAMS,		/**< PID number is too big*/
	AM_DVR_ERR_STREAM_ALREADY_ADD,		/**< The elementary stream has already been added*/
	AM_DVR_ERR_END
};


/**\brief DVR device open parameters*/
typedef struct
{
	int    foo;	
} AM_DVR_OpenPara_t;

/**\brief Recording parameters*/
typedef struct
{
	int		pid_count; /**< PID number*/
	int		pids[AM_DVR_MAX_PID_COUNT]; /**< PID array*/
} AM_DVR_StartRecPara_t;

/**\brief DVR recording source*/
typedef enum
{
	AM_DVR_SRC_ASYNC_FIFO0, /**< asyncfifo 0*/
	AM_DVR_SRC_ASYNC_FIFO1, /**< asyncfifo 1*/
	AM_DVR_SRC_ASYNC_FIFO2, /**< asyncfifo 2*/
}AM_DVR_Source_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Open a DVR device
 * \param dev_no DVR device number
 * \param[in] para DVR device open parameters
 * \retval AM_SUCCESS On succes
 * \return Error code
 */
extern AM_ErrorCode_t AM_DVR_Open(int dev_no, const AM_DVR_OpenPara_t *para);

/**\brief Close an unused DVR device
 * \param dev_no DVR device number
 * \retval AM_SUCCESS On succes
 * \return Error code
 */
extern AM_ErrorCode_t AM_DVR_Close(int dev_no);

/**\brief Set the DVR device's ring queue buffer size.
 * \param dev_no DVR device number
 * \param size Ring queue buffer size
 * \retval AM_SUCCESS On succes
 * \return Error code
 */
extern AM_ErrorCode_t AM_DVR_SetBufferSize(int dev_no, int size);

/**\brief Start recording
 * \param dev_no DVR device number
 * \param [in] para Recording parameters
 * \retval AM_SUCCESS On succes
 * \return Error code
 */
extern AM_ErrorCode_t AM_DVR_StartRecord(int dev_no, const AM_DVR_StartRecPara_t *para);

/**\brief Stop recording
 * \param dev_no DVR device number
 * \retval AM_SUCCESS On succes
 * \return Error code
 */
extern AM_ErrorCode_t AM_DVR_StopRecord(int dev_no);

/**\brief Set the DVR input source
 * \param dev_no DVR device number
 * \param	src DVR input source
 * \retval AM_SUCCESS On succes
 * \return Error code
 */
extern AM_ErrorCode_t AM_DVR_SetSource(int dev_no, AM_DVR_Source_t src);

/**\brief Read TS data from a DVR device
 * \param dev_no DVR device number
 * \param[out] buf TS output buffer
 * \param size	The buffer size in bytes
 * \param timeout_ms Timeout in milliseconds
 * \return Read data size in bytes
 */
extern int AM_DVR_Read(int dev_no, uint8_t *buf, int size, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif

