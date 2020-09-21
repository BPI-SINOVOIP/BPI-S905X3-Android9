/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Subtitle module (version 2)
 *
 * \author Yan Yan <yan.yan@amlogic.com>
 * \date 2019-07-22: create the document
 ***************************************************************************/


#ifndef _AM_SCTE27_H
#define _AM_SCTE27_H

#include "am_types.h"
#include "am_osd.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/
#define SCTE27_TID 0xC6

enum AM_SCTE27_ErrorCode
{
	AM_SCTE27_ERROR_BASE=AM_ERROR_BASE(AM_MOD_SCTE27),
	AM_SCTE27_ERR_INVALID_PARAM,   /**< Invalid parameter*/
	AM_SCTE27_ERR_INVALID_HANDLE,  /**< Invalid handle*/
	AM_SCTE27_ERR_NOT_SUPPORTED,   /**< not surport action*/
	AM_SCTE27_ERR_CREATE_DECODE,   /**< open subtitle decode error*/
	AM_SCTE27_ERR_SET_BUFFER,      /**< set pes buffer error*/
	AM_SCTE27_ERR_NO_MEM,                  /**< out of memmey*/
	AM_SCTE27_ERR_CANNOT_CREATE_THREAD,    /**< cannot creat thread*/
	AM_SCTE27_ERR_NOT_RUN,            /**< thread run error*/
	AM_SCTE27_INIT_DISPLAY_FAILED,    /**< init display error*/
	AM_SCTE27_PACKET_INVALID,
	AM_SCTE27_ERR_END
};

enum AM_SCTE27_Decoder_Error
{
	AM_SCTE27_Decoder_Error_LoseData,
	AM_SCTE27_Decoder_Error_InvalidData,
	AM_SCTE27_Decoder_Error_TimeError,
	AM_SCTE27_Decoder_Error_END
};


typedef void*      AM_SCTE27_Handle_t;
typedef void (*AM_SCTE27_DrawBegin_t)(AM_SCTE27_Handle_t handle);
typedef void (*AM_SCTE27_DrawEnd_t)(AM_SCTE27_Handle_t handle);
typedef void (*AM_SCTE27_LangCb_t)(AM_SCTE27_Handle_t handle, char* buffer, int size);
typedef void (*AM_SCTE27_ReportError)(AM_SCTE27_Handle_t handle, int error);
typedef void (*AM_SCTE27_PicAvailable)(AM_SCTE27_Handle_t handle);
typedef void (*AM_SCTE27_UpdateSize)(AM_SCTE27_Handle_t handle, int width, int height);


typedef struct
{
	AM_SCTE27_DrawBegin_t   draw_begin;
	AM_SCTE27_DrawEnd_t     draw_end;
	AM_SCTE27_LangCb_t  lang_cb;
	AM_SCTE27_ReportError report;
	AM_SCTE27_PicAvailable report_available;
	AM_SCTE27_UpdateSize update_size;
	uint8_t         **bitmap;         /**< draw bitmap buffer*/
	int              pitch;          /**< the length of draw bitmap buffer per line*/
	int width;
	int height;
	void                    *user_data;      /**< user private data*/
}AM_SCTE27_Para_t;

AM_ErrorCode_t AM_SCTE27_Create(AM_SCTE27_Handle_t *handle, AM_SCTE27_Para_t *para);
AM_ErrorCode_t AM_SCTE27_Destroy(AM_SCTE27_Handle_t handle);
void*          AM_SCTE27_GetUserData(AM_SCTE27_Handle_t handle);
AM_ErrorCode_t AM_SCTE27_Decode(AM_SCTE27_Handle_t handle, const uint8_t *buf, int size);
AM_ErrorCode_t AM_SCTE27_Start(AM_SCTE27_Handle_t handle);
AM_ErrorCode_t AM_SCTE27_Stop(AM_SCTE27_Handle_t handle);

#ifdef __cplusplus
}
#endif

#endif

