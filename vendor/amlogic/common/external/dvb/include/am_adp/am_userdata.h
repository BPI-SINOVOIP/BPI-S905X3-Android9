/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
 /**\file am_userdata.h
 * \brief MPEG user data device module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2013-3-13: create the document
 ***************************************************************************/

#ifndef _AM_USERDATA_H
#define _AM_USERDATA_H

#include <unistd.h>
#include <sys/types.h>
#include "am_types.h"
#include "am_evt.h"

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

/**\brief Error code of the user data module*/
enum AM_USERDATA_ErrorCode
{
	AM_USERDATA_ERROR_BASE=AM_ERROR_BASE(AM_MOD_USERDATA),
	AM_USERDATA_ERR_INVALID_ARG,             /**< Invalid argument*/
	AM_USERDATA_ERR_INVALID_DEV_NO,          /**< Invalid device number*/
	AM_USERDATA_ERR_BUSY,                    /**< The device is busy*/
	AM_USERDATA_ERR_CANNOT_OPEN_DEV,         /**< Cannot open the device*/
	AM_USERDATA_ERR_NOT_SUPPORTED,           /**< Not supported*/
	AM_USERDATA_ERR_NO_MEM,                  /**< Not enough memory*/
	AM_USERDATA_ERR_TIMEOUT,                 /**< Timeout*/
	AM_USERDATA_ERR_SYS,                     /**< System error*/
	AM_USERDATA_ERR_END
};

enum AM_USERDATA_EventType
{
	AM_USERDATA_EVT_BASE = AM_EVT_TYPE_BASE(AM_MOD_USERDATA),
	AM_USERDATA_EVT_AFD,
};

enum AM_USERDATA_Mode
{
	AM_USERDATA_MODE_CC = 0x1,
	AM_USERDATA_MODE_AFD = 0x2,
};

/**\brief MPEG user data device open parameters*/
typedef struct
{
	int    foo;
	int vfmt;
	int cc_default_stop;
} AM_USERDATA_OpenPara_t;

typedef struct
{
	uint8_t         :6;
	uint8_t  af_flag:1;
	uint8_t         :1;
	uint8_t  af     :4;
	uint8_t         :4;
	uint16_t reserved;
	uint32_t pts;
} AM_USERDATA_AFD_t;

/****************************************************************************
 * Function prototypes
 ***************************************************************************/

/**\brief Open the MPEG user data device
 * \param dev_no Device number
 * \param[in] para Device open parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_USERDATA_Open(int dev_no, const AM_USERDATA_OpenPara_t *para);

/**\brief Close the MPEG user data device
 * \param dev_no Device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_USERDATA_Close(int dev_no);

/**\brief Read MPEG user data from the device
 * \param dev_no Device number
 * \param[out] buf Output buffer to store the user data
 * \param size	Buffer length in bytes
 * \param timeout_ms Timeout time in milliseconds
 * \return Read data length in bytes
 */
extern int AM_USERDATA_Read(int dev_no, uint8_t *buf, int size, int timeout_ms);

extern AM_ErrorCode_t AM_USERDATA_SetMode(int dev_no, int mode);
extern AM_ErrorCode_t AM_USERDATA_GetMode(int dev_no, int *mode);

#ifdef __cplusplus
}
#endif

#endif

