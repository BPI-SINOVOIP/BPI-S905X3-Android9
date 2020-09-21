/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief smart card communication module
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-29: create the document
 ***************************************************************************/

#ifndef _AM_SMC_H
#define _AM_SMC_H

#include "am_types.h"
#include "am_evt.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_SMC_MAX_ATR_LEN    (33)

/****************************************************************************
 * Error code definitions
 ****************************************************************************/
/**\brief Error code of the Smart card module*/
enum AM_SMC_ErrorCode
{
	AM_SMC_ERROR_BASE=AM_ERROR_BASE(AM_MOD_SMC),
	AM_SMC_ERR_INVALID_DEV_NO,           /**< Invalid device number*/
	AM_SMC_ERR_BUSY,                     /**< The device has already been openned*/
	AM_SMC_ERR_NOT_OPENNED,              /**< The device is not yet open*/
	AM_SMC_ERR_CANNOT_OPEN_DEV,          /**< Failed to open device*/
	AM_SMC_ERR_CANNOT_CREATE_THREAD,     /**< Creating device failure*/
	AM_SMC_ERR_TIMEOUT,                  /**< Timeout*/
	AM_SMC_ERR_NOT_SUPPORTED,            /**< The device does not support this function*/
	AM_SMC_ERR_IO,                       /**< Device input or output errors*/
	AM_SMC_ERR_BUF_TOO_SMALL,            /**< Buffer too small*/
	AM_SMC_ERR_NO_CARD,                  /**< Smart card not inserted*/
	AM_SMC_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief Event type of Smart card module*/
enum AM_SMC_EventType
{
	AM_SMC_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_SMC),
	AM_SMC_EVT_CARD_IN,                  /**< Smart card insert*/
	AM_SMC_EVT_CARD_OUT,                 /**< Smart card pull out*/
	AM_SMC_EVT_END
};


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Smart card device enable parameter*/
typedef struct
{
	int  enable_thread;                  /**< check the thread of check smart card status is enable*/
} AM_SMC_OpenPara_t;

/**\brief Insert Status type of smart card */
typedef enum
{
	AM_SMC_CARD_OUT, /**< Smart card pull out*/
	AM_SMC_CARD_IN   /**< Smart card insert*/
} AM_SMC_CardStatus_t;

/**\brief Smart card status callback*/
typedef void (*AM_SMC_StatusCb_t) (int dev_no, AM_SMC_CardStatus_t status, void *data);

/** brief Smart card device parameter*/
typedef struct
{
	int     f;                 /**<Clock frequency conversion coefficient*/
	int     d;                 /**<baud rate coefficient*/
	int     n;                 /**<the N parameter is used to provide extra guard time between characters.*/
	int     bwi;               /**<The BWI is used to define the Block Wait Time*/
	int     cwi;               /**<The CWT is defined as the wait time between characters*/
	int     bgt;               /**<Block Guard Time*/
	int     freq;              /**<clock frequency*/
	int     recv_invert;       /**<1:invert the data bits (excluding the parity) during receiving*/
	int     recv_lsb_msb;      /**<1:Swap MSBits / Lsbits during receiving*/
	int     recv_no_parity;    /**<Ignore parity,This is useful during debug*/
	int     recv_parity;			 /**< 1:Invert Parity during receiving*/
	int     xmit_invert;       /**<1:invert the data bits (excluding the parity)*/
	int     xmit_lsb_msb;      /**<1:Swap MSBits / LSbits*/
	int     xmit_retries;      /**<Number of attempts to make sending a character.*/
	int     xmit_repeat_dis;   /**<Set this bit to 1 to disable character repeats when an error is detected.*/
	int     xmit_parity;			 /**<1:Invert Parity*/
}AM_SMC_Param_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Open smart card device
 * \param dev_no Smart card device number
 * \param[in] para Smart card device open parameter
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_Open(int dev_no, const AM_SMC_OpenPara_t *para);

/**\brief Close smart cart device
 * \param dev_no Smart card device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_Close(int dev_no);

/**\brief Get smart card insert status
 * \param dev_no Smart card device number
 * \param[out] status inert status of smart card device
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_GetCardStatus(int dev_no, AM_SMC_CardStatus_t *status);

/**\brief Reset smart card
 * \param dev_no Smart card device number
 * \param[out] atr Smart card attribute data
 * \param[in,out] len input attribute len,out the actual attribute length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_Reset(int dev_no, uint8_t *atr, int *len);

/**\brief Read data from Smart card device
 * Read data directly from the smart card device,
 * The thread that calls the function will block,
 * Until the expiration of the expected number of data read
 * Or timeout.
 * \param dev_no Smart card device number
 * \param[out] data Read data buffer
 * \param[in] len want read Data length
 * \param timeout Read timeout in milliseconds, <0 said the permanent wait.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_Read(int dev_no, uint8_t *data, int len, int timeout);

/**\brief write data to smard card device
 * Write data to the smart card device,
 * The thread that calls the function will block,
 * Until the expiration of the expected number of data write
 * Or timeout.
 * \param dev_no Smart card device number
 * \param[in] data Write data buffer
 * \param[in] len want write Data length
 * \param timeout Write timeout in milliseconds, <0 said the permanent wait.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_Write(int dev_no, const uint8_t *data, int len, int timeout);

/**\brief Read data from Smart card device
 * Read data directly from the smart card device,
 * The thread that calls the function will block,
 * Until the expiration of the expected number of data read
 * Or timeout.
  * \param dev_no Smart card device number
 * \param[out] data Read data buffer
 * \param[out] act_len Actual read data length
 * \param[in] len len want read Data length
 * \param timeout Read timeout in milliseconds, <0 said the permanent wait.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_ReadEx(int dev_no, uint8_t *data, int *act_len, int len, int timeout);

/**\brief write data to smard card device
 * Write data to the smart card device,
 * The thread that calls the function will block,
 * Until the expiration of the expected number of data write
 * Or timeout.
 * \param dev_no Smart card device number
 * \param[in] data Write data buffer
 * \param[out] act_len Actual Write data length
 * \param[in] len len want write Data length
 * \param timeout Write timeout in milliseconds, <0 said the permanent wait.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_WriteEx(int dev_no, const uint8_t *data, int *act_len, int len, int timeout);

/**\brief Transmit data by T0 protocol
 * \param dev_no Smart card device number
 * \param[in] send send data buffer
 * \param[in] slen send data length
 * \param[out] recv Receive data buffer
 * \param[out] rlen Receive data length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_TransferT0(int dev_no, const uint8_t *send, int slen, uint8_t *recv, int *rlen);

/**\brief Transmit data by T1 protocol
 * \param dev_no Smart card device number
 * \param[in] send send data buffer
 * \param[in] slen send data length
 * \param[out] recv Receive data buffer
 * \param[out] rlen Receive data length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_TransferT1(int dev_no, const uint8_t *send, int slen, uint8_t *recv, int *rlen);

/**\brief Transmit data by T14 protocol
 * \param dev_no Smart card device number
 * \param[in] send send data buffer
 * \param[in] slen send data length
 * \param[out] recv Receive data buffer
 * \param[out] rlen Receive data length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_TransferT14(int dev_no, const uint8_t *send, int slen, uint8_t *recv, int *rlen);

/**\brief Get callback function of the smart card status
 * \param dev_no Smart card device number
 * \param[out] cb The pointer of callback function
 * \param[out] data user data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_GetCallback(int dev_no, AM_SMC_StatusCb_t *cb, void **data);

/**\brief Set callback function of the smart card status
 * \param dev_no Smart card device number
 * \param[in] cb The pointer of callback function
 * \param[in] data user data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_SetCallback(int dev_no, AM_SMC_StatusCb_t cb, void *data);

/**\brief Activate smart card device
 * \param dev_no Smart card device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_Active(int dev_no);

/**\brief Deactivate smart card device
 * \param dev_no Smart card device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_Deactive(int dev_no);

/**\brief Get smart card parameters
 * \param dev_no Smart card device number
 * \param[out] para smart card parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_GetParam(int dev_no, AM_SMC_Param_t *para);

/**\brief Set smart card parameters
 * \param dev_no Smart card device number
 * \param[in] para smart card parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SMC_SetParam(int dev_no, const AM_SMC_Param_t *para);


#ifdef __cplusplus
}
#endif

#endif

