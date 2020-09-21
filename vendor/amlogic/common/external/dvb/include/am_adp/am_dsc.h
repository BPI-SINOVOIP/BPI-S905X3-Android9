/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Descrambler device module
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#ifndef _AM_DSC_H
#define _AM_DSC_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
/* For sec os mode */
#define DSC_SEC_CaLib 	"libam_sec_dsc.so"
#define KL_SEC_Lib 		"libam_kl_api.so"

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Error code of the descrambler module*/
enum AM_DSC_ErrorCode
{
	AM_DSC_ERROR_BASE=AM_ERROR_BASE(AM_MOD_DSC),
	AM_DSC_ERR_INVALID_DEV_NO,          /**< Invalid device number*/
	AM_DSC_ERR_BUSY,                    /**< The device is busy*/
	AM_DSC_ERR_NOT_ALLOCATED,           /**< The device has not been allocated*/
	AM_DSC_ERR_CANNOT_OPEN_DEV,         /**< Cannot open the device*/
	AM_DSC_ERR_NOT_SUPPORTED,           /**< Not supported*/
	AM_DSC_ERR_NO_FREE_CHAN,            /**< No free channel left*/
	AM_DSC_ERR_NO_MEM,                  /**< Not enough memory*/
	AM_DSC_ERR_SYS,                     /**< System error*/
	AM_DSC_ERR_INVALID_ID,              /**< Invalid channel index*/
	AM_DSC_ERR_END
};


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Descrambler device open parameters*/
typedef struct
{
	int  foo;
} AM_DSC_OpenPara_t;

/**\brief Work enviorment, tee/kernel */
typedef enum _work_mode_e{
	DSC_MODE_NORMAL,
	DSC_MODE_SEC,
	DSC_MODE_MAX
}AM_DSC_WorkMode_e;

/**\brief Parity */
typedef enum _key_parity{
	DSC_KEY_ODD,
	DSC_KEY_EVEN
}AM_DSC_KeyParity_e;

/**\brief Video encrypt algoritm */
typedef enum {
	CW_ALGO_AES = 0,
	CW_ALGO_DVBCSA = 1,
	CW_ALGO_MAX = 2,
} CW_ALGO_T;

typedef enum {
	DSC_DSC_CBC = 1,
	DSC_DSC_ECB,
	DSC_DSC_IDSA
}AM_DSC_MODE_t;

/**\brief CW key type */
typedef enum {
	AM_DSC_KEY_TYPE_EVEN = 0,        /**< DVB-CSA even control word*/
	AM_DSC_KEY_TYPE_ODD = 1,         /**< DVB-CSA odd control word*/
	AM_DSC_KEY_TYPE_AES_EVEN = 2,    /**< AES even control word*/
	AM_DSC_KEY_TYPE_AES_ODD = 3,     /**< AES odd control word*/
	AM_DSC_KEY_TYPE_AES_IV_EVEN = 4, /**< AES-CBC even control word's IV data*/
	AM_DSC_KEY_TYPE_AES_IV_ODD = 5,  /**< AES-CBC odd control word's IV data*/
	AM_DSC_KEY_TYPE_DES_EVEN = 6,    /**< DES even control word*/
	AM_DSC_KEY_TYPE_DES_ODD = 7,     /**< DES odd control word*/
	AM_DSC_KEY_TYPE_SM4_EVEN = 8,	 /**< SM4 even key */
	AM_DSC_KEY_TYPE_SM4_ODD = 9,	 /**< SM4 odd key */
	AM_DSC_KEY_TYPE_SM4_EVEN_IV = 10,/**< SM4-CBC iv even key */
	AM_DSC_KEY_TYPE_SM4_ODD_IV = 11, /**< SM4-CBC iv odd key */
	AM_DSC_KEY_FROM_KL = (1<<7)      /**< Read the control word from hardware keyladder*/
} AM_DSC_KeyType_t;

/**\brief Input source of the descrambler*/
typedef enum {
	AM_DSC_SRC_DMX0,         /**< Demux device 0*/
	AM_DSC_SRC_DMX1,         /**< Demux device 1*/
	AM_DSC_SRC_DMX2,         /**< Demux device 2*/
	AM_DSC_SRC_BYPASS        /**< Bypass TS data*/
} AM_DSC_Source_t;

typedef AM_ErrorCode_t (*AM_KL_SEC_GetKeys_t)(int index, int level, unsigned char keys[6][16]);


/****************************************************************************
 * Function prototypes
 ***************************************************************************/

/**\brief Open a descrambler device
 * \param dev_no Descrambler device number
 * \param[in] para Device open paramaters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_Open(int dev_no, const AM_DSC_OpenPara_t *para);

/**\brief Allocate a new descrambler channel
 * \param dev_no Descrambler device number
 * \param[out] chan_id Return the new channel's index
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_AllocateChannel(int dev_no, int *chan_id);

/**\brief Set the descrambler channel's PID value
 * \param dev_no Descrambler device number
 * \param chan_id Descrambler channel's index
 * \param pid PID value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_SetChannelPID(int dev_no, int chan_id, uint16_t pid);

/**\brief Set the control word to the descrambler channel
 * \param dev_no Descrambler device number
 * \param chan_id Descrambler channel's index
 * \param type Control word's type
 * \param[in] key Control word
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_SetKey(int dev_no, int chan_id, AM_DSC_KeyType_t type, const uint8_t *key);

/**\brief Release an unused descrambler channel
 * \param dev_no Descrambler device number
 * \param chan_id Descrambler channel's index to be released
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_FreeChannel(int dev_no, int chan_id);

/**\brief Close the descrambler device
 * \param dev_no Descrambler device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_Close(int dev_no);

/**\brief Set the input source of the descrambler device
 * \param dev_no Descrambler device number
 * \param src Input source
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_SetSource(int dev_no, AM_DSC_Source_t src);

/**\brief Set the output of descrambler device
 * \param dev_no Descrambler device number
 * \param src destination demux
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_SetOutput(int dev_no, int dst);

/**\brief Set descrambler mode
 * \param dev_no Descrambler device number
 * \param mode
 * \ typedef enum {
	DSC_DSC_CBC = 1,
	DSC_DSC_ECB,
	DSC_DSC_IDSA
 * \	}AM_DSC_MODE_t
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DSC_SetMode(int dev_no, int chan_id, int mode);

#ifdef __cplusplus
}
#endif

#endif

