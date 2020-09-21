/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Audio output control module
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#ifndef _AM_AOUT_H
#define _AM_AOUT_H

#include "am_types.h"
#include "am_evt.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_AOUT_VOLUME_MIN 0   /**< The minimum volumn value */
#define AM_AOUT_VOLUME_MAX 100 /**< The maximum volumn value */

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Error code of the audio output module */
enum AM_AOUT_ErrorCode
{
	AM_AOUT_ERROR_BASE=AM_ERROR_BASE(AM_MOD_AOUT),
	AM_AOUT_ERR_INVALID_DEV_NO,          /**< Invalid audio output device number */
	AM_AOUT_ERR_BUSY,                    /**< The device has already been openned */
	AM_AOUT_ERR_ILLEGAL_OP,              /**< Illegal operation */
	AM_AOUT_ERR_INVAL_ARG,               /**< Invalid argument */
	AM_AOUT_ERR_NOT_ALLOCATED,           /**< The device has not been allocated */
	AM_AOUT_ERR_CANNOT_CREATE_THREAD,    /**< Cannot create a new thread */
	AM_AOUT_ERR_CANNOT_OPEN_DEV,         /**< Cannot open the device */
	AM_AOUT_ERR_CANNOT_OPEN_FILE,        /**< Cannot open the file */
	AM_AOUT_ERR_NOT_SUPPORTED,           /**< The operation is not supported */
	AM_AOUT_ERR_NO_MEM,                  /**< Not enough memory */
	AM_AOUT_ERR_TIMEOUT,                 /**< Timeout*/
	AM_AOUT_ERR_SYS,                     /**< System error*/
	AM_AOUT_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief Event type of the audio output module*/
enum AM_AOUT_EventType
{
	AM_AOUT_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_AOUT),
	AM_AOUT_EVT_VOLUME_CHANGED,          /**< Volumn has been changed，the parameter is the volumn value (int 0~100)*/
	AM_AOUT_EVT_MUTE_CHANGED,            /**< Mute/Unmute status changed，the parameter is the new mute status (AM_Bool_t)*/
	AM_AOUT_EVT_OUTPUT_MODE_CHANGED,     /**< Audio output mode changed，the parameter is the new mode (AM_AOUT_OutputMode_t)*/
	AM_AOUT_EVT_PREGAIN_CHANGED,         /**< Pregain value changed，the parameter is the new pregain value*/
	AM_AOUT_EVT_PREMUTE_CHANGED,         /**< Premute value changed, the parameter is the new premute value*/
	AM_AOUT_EVT_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Audio output mode*/
typedef enum
{
	AM_AOUT_OUTPUT_STEREO,     /**< Stereo output*/
	AM_AOUT_OUTPUT_DUAL_LEFT,  /**< Left audio output to dual channel*/
	AM_AOUT_OUTPUT_DUAL_RIGHT, /**< Right audio output to dual channel*/
	AM_AOUT_OUTPUT_SWAP        /**< Swap left and right channel*/
} AM_AOUT_OutputMode_t;

/**\brief Audio output device open parameters*/
typedef struct
{
	int   foo;
} AM_AOUT_OpenPara_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Open an audio output device
 * \param dev_no Audio output device number
 * \param[in] para Audio output device open parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_Open(int dev_no, const AM_AOUT_OpenPara_t *para);

/**\brief Close the audio output device
 * \param dev_no Audio output device number
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_Close(int dev_no);

/**\brief Set the volumn of the audio output (0~100)
 * \param dev_no Audio output device number
 * \param vol Volumn number (0~100)
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_SetVolume(int dev_no, int vol);

/**\brief Get the current volumn of the audio output
 * \param dev_no Audio output device number
 * \param[out] vol Return the volumn value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_GetVolume(int dev_no, int *vol);

/**\brief Mute/unmute the audio output
 * \param dev_no Audio output device number
 * \param mute AM_TRUE to mute the output, AM_FALSE to unmute the output
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_SetMute(int dev_no, AM_Bool_t mute);

/**\brief Get the current mute status of the audio
 * \param dev_no Audio output device number
 * \param[out] mute Return the mute status
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_GetMute(int dev_no, AM_Bool_t *mute);

/**\brief Set the audio output mode
 * \param dev_no Audio output device number
 * \param mode New audio output mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_SetOutputMode(int dev_no, AM_AOUT_OutputMode_t mode);

/**\brief Get the current audio output mode
 * \param dev_no Audio output device number
 * \param[out] mode Return the current audio output mode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_GetOutputMode(int dev_no, AM_AOUT_OutputMode_t *mode);

/**\brief Set the audio pregain value
 * \param dev_no Audio output device number
 * \param gain Pregain value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_SetPreGain(int dev_no, float gain);

/**\brief Get the current audio pregain value
 * \param dev_no Audio output device number
 * \param[out] gain Return the audio pregain value
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_GetPreGain(int dev_no, float *gain);

/**\brief Set premute/preunmute status of the audio
 * \param dev_no Audio output device number
 * \param mute AM_TRUE means mute, AM_FALSE means unmute
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_SetPreMute(int dev_no, AM_Bool_t mute);

/**\brief Get the current premute status of the audio
 * \param dev_no Audio output device number
 * \param[out] mute Return the current premute status
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_AOUT_GetPreMute(int dev_no, AM_Bool_t *mute);


#ifdef __cplusplus
}
#endif

#endif

