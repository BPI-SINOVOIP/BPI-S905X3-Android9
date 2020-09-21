/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief PES packet parser module
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2012-08-01: create the document
 ***************************************************************************/


#ifndef _AM_PES_H
#define _AM_PES_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/


/**\brief PES parser module's error code*/
enum AM_PES_ErrorCode
{
	AM_PES_ERROR_BASE=AM_ERROR_BASE(AM_MOD_PES),
	AM_PES_ERR_INVALID_PARAM,   /**< Invalid parameter*/
	AM_PES_ERR_INVALID_HANDLE,  /**< Invalid handle*/
	AM_PES_ERR_NO_MEM,          /**< Not enough memory*/
	AM_PES_ERR_END
};

/**\brief PES parser handle*/
typedef void* AM_PES_Handle_t;

/**\brief PES packet callback function
 * \a handle The PES parser's handle.
 * \a buf The PES packet.
 * \a size The packet size in bytes.
 */
typedef void (*AM_PES_PacketCb_t)(AM_PES_Handle_t handle, uint8_t *buf, int size);

/**\brief PES parser's parameters*/
typedef struct
{
	AM_PES_PacketCb_t packet;       /**< PES packet callback function*/
	AM_Bool_t         payload_only; /**< Only read PES payload*/
	void             *user_data;    /**< User dafined data*/
	int				  afmt;			/**< audio fmt*/
}AM_PES_Para_t;

/**\brief Create a new PES parser
 * \param[out] handle Return the new PES parser's handle
 * \param[in] para PES parser's parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_PES_Create(AM_PES_Handle_t *handle, AM_PES_Para_t *para);

/**\brief Release an unused PES parser
 * \param handle The PES parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_PES_Destroy(AM_PES_Handle_t handle);

/**\brief Parse the PES data
 * \param handle PES parser's handle
 * \param[in] buf PES data's buffer
 * \param size Data in the buffer
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_PES_Decode(AM_PES_Handle_t handle, uint8_t *buf, int size);

/**\brief Get the user defined data of the PES parser
 * \param handle PES parser's handle
 * \return The user defined data
 */
void*          AM_PES_GetUserData(AM_PES_Handle_t handle);

#ifdef __cplusplus
}
#endif



#endif
