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
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2012-08-01: create the document
 ***************************************************************************/


#ifndef _AM_SUB2_H
#define _AM_SUB2_H

#include "am_types.h"
#include "am_osd.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Subtitle parse handle*/
typedef void* AM_SUB2_Handle_t;

/**\brief Error code of the subtitle module*/
enum AM_SUB2_ErrorCode
{
	AM_SUB2_ERROR_BASE=AM_ERROR_BASE(AM_MOD_SUB2),
	AM_SUB2_ERR_INVALID_PARAM,   /**< Invalid parameter*/
	AM_SUB2_ERR_INVALID_HANDLE,  /**< Invalid handle*/
	AM_SUB2_ERR_NOT_SUPPORTED,   /**< not surport action*/
	AM_SUB2_ERR_CREATE_DECODE,   /**< open subtitle decode error*/
	AM_SUB2_ERR_OPEN_PES,        /**< open pes filter error*/
	AM_SUB2_ERR_SET_BUFFER,      /**< set pes buffer error*/
	AM_SUB2_ERR_NO_MEM,                  /**< out of memmey*/
	AM_SUB2_ERR_CANNOT_CREATE_THREAD,    /**< cannot creat thread*/
	AM_SUB2_ERR_NOT_RUN,            /**< thread run error*/
	AM_SUB2_INIT_DISPLAY_FAILED,    /**< init display error*/
	AM_SUB2_ERR_END
};

enum AM_SUB2_Decoder_Error
{
	AM_SUB2_Decoder_Error_LoseData,
	AM_SUB2_Decoder_Error_InvalidData,
	AM_SUB2_Decoder_Error_TimeError,
	AM_SUB2_Decoder_Error_END
};

/**\brief Subtitle region*/
typedef struct AM_SUB2_Region
{
    int32_t                 left;               /**< subtitle show X coordinate*/
    int32_t                 top;                /**< subtitle show Y coordinate*/
    uint32_t                width;              /**< subtitle show width*/
    uint32_t                height;             /**< subtitle show height*/

    uint32_t                entry;              /**< Color palette*/
    AM_OSD_Color_t          clut[256];          /**< palette*/

    /* for background */
    uint32_t                background;         /**< subtitle background color*/

    /* for pixel map */
    uint8_t                *p_buf;              /**< show subtitle bitmap*/

    /* for text */
    uint32_t                fg;                 /**< text Foreground color*/
    uint32_t                bg;                 /**< text background color*/

    uint32_t                length;             /**< Character string length*/
    uint16_t               *p_text;             /**< Character string*/

    struct AM_SUB2_Region  *p_next;             /**< next Region of subtitle list*/

}AM_SUB2_Region_t;

/**\brief Subtitle picture*/
typedef struct AM_SUB2_Picture
{
    uint64_t                pts;                /**< PTS*/
    uint32_t                timeout;            /**< Display time(s)*/

    int32_t                 original_x;         /**< subtitle show X coordinate*/
    int32_t                 original_y;         /**< subtitle show Y coordinate*/

    uint32_t                original_width;     /**< subtitle show width*/
    uint32_t                original_height;    /**< subtitle show height*/

    AM_SUB2_Region_t       *p_region;           /**< Region list*/

    struct AM_SUB2_Picture *p_prev;             /**< pre Picture in the list*/
    struct AM_SUB2_Picture *p_next;             /**< next Picture in the list*/

}AM_SUB2_Picture_t;

/**\brief Subtitle callback function of show*/
typedef void (*AM_SUB2_ShowCb_t)(AM_SUB2_Handle_t handle, AM_SUB2_Picture_t* pic);

/**\brief get current PTS*/
typedef uint64_t (*AM_SUB2_GetPTS_t)(AM_SUB2_Handle_t handle, uint64_t pts);
typedef void (*AM_SUB2_ReportError)(AM_SUB2_Handle_t handle, int error);
typedef void (*AM_SUB2_PicAvailable)(AM_SUB2_Handle_t handle);

/**\brief Subtitle parameter*/
typedef struct
{
	AM_SUB2_ShowCb_t show;           /**< callback function of show subtitle*/
	AM_SUB2_GetPTS_t get_pts;        /**< current PTS*/
	AM_SUB2_ReportError report;
	AM_SUB2_PicAvailable report_available;
	uint16_t         composition_id; /**< Subtitle composition ID*/
	uint16_t         ancillary_id;   /**< Subtitle ancillary ID*/
	void            *user_data;      /**< user private data*/
}AM_SUB2_Para_t;

/**\brief creat subtitle parse handle
 * \param[out] handle subtitle parse handle
 * \param[in] para subtitle parse parameter
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SUB2_Create(AM_SUB2_Handle_t *handle, AM_SUB2_Para_t *para);

/**\brief destroy subtitle parse handle
 * \param handle subtitle parse handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SUB2_Destroy(AM_SUB2_Handle_t handle);

/**\brief get user private data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern void*          AM_SUB2_GetUserData(AM_SUB2_Handle_t handle);

/**\brief parse subtitle data
 * \param handle subtitle parse handle
 * \param[in] buf PES buffer
 * \param size PES buffer length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SUB2_Decode(AM_SUB2_Handle_t handle, uint8_t *buf, int size);

/**\brief start show subtitle
 * \param handle subtitle parse handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SUB2_Start(AM_SUB2_Handle_t handle);

/**\brief stop show subtitle
 * \param handle subtitle parse handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_SUB2_Stop(AM_SUB2_Handle_t handle);

#ifdef __cplusplus
}
#endif



#endif
