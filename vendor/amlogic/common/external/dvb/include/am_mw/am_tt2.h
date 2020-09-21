/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Teletext module(version 2)
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2012-08-08: create the document
 ***************************************************************************/


#ifndef _AM_TT2_H
#define _AM_TT2_H

#include "am_types.h"
#include "am_osd.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Teletext parse handle*/
typedef void* AM_TT2_Handle_t;

#define AM_TT2_ANY_SUBNO 0x3F7F

/**\brief Error code of the Teletext module*/
enum AM_TT2_ErrorCode
{
	AM_TT2_ERROR_BASE=AM_ERROR_BASE(AM_MOD_SUB2),
	AM_TT2_ERR_INVALID_PARAM,   /**< Invalid parameter*/
	AM_TT2_ERR_INVALID_HANDLE,  /**< Invalid handle*/
	AM_TT2_ERR_NOT_SUPPORTED,   /**< not surport action*/
	AM_TT2_ERR_CREATE_DECODE,   /**< open Teletext decode error*/
	AM_TT2_ERR_OPEN_PES,        /**< open pes filter error*/
	AM_TT2_ERR_SET_BUFFER,      /**< set pes buffer error*/
	AM_TT2_ERR_NO_MEM,                  /**< out of memmey*/
	AM_TT2_ERR_CANNOT_CREATE_THREAD,    /**< cannot creat thread*/
	AM_TT2_ERR_NOT_RUN,            /**< thread run error*/
	AM_TT2_INIT_DISPLAY_FAILED,    /**< init display error*/
	AM_TT2_ERR_END
};

/**\brief Teletext color*/
typedef enum{
	AM_TT2_COLOR_RED,           /**< red*/
	AM_TT2_COLOR_GREEN,         /**< green*/
	AM_TT2_COLOR_YELLOW,        /**< yellow*/
	AM_TT2_COLOR_BLUE           /**< blue*/
}AM_TT2_Color_t;

/**\brief Teletext color*/
typedef enum{
	AM_TT2_DisplayMode_ShowAll,
}AM_TT2_DisplayMode_t;


/**\brief start draw teletext callback function*/
typedef void (*AM_TT2_DrawBegin_t)(AM_TT2_Handle_t handle);

/**\brief stop draw teletext callback function*/
typedef void (*AM_TT2_DrawEnd_t)(AM_TT2_Handle_t handle,
								 int page_type,
								 int pgno,
								 char* subs,
								 int sub_cnt,
							     int red,
							     int green,
							     int yellow,
							     int blue,
								 int curr_subpg);

typedef void (*AM_TT2_NotifyData_t)(AM_TT2_Handle_t handle, int pgno);

/**\brief get PTS callback function */
typedef uint64_t (*AM_TT2_GetPTS_t)(AM_TT2_Handle_t handle, uint64_t pts);

/**\brief get new teletext page callback function */
typedef void (*AM_TT2_NewPage_t)(AM_TT2_Handle_t handle, int pgno, int sub_pgno);

typedef enum {
	AM_TT_INPUT_PES,
	AM_TT_INPUT_VBI
}AM_TT_Input_t;

/**\brief Teletext parameter*/
typedef struct
{
	AM_TT_Input_t input;
	AM_TT2_DrawBegin_t draw_begin;   /**< start draw teletext callback function*/
	AM_TT2_DrawEnd_t   draw_end;     /**< stop draw teletext callback function*/
	AM_TT2_NotifyData_t     notify_contain_data;
	AM_TT2_NewPage_t   new_page;     /**< get new teletext page callback function*/
	AM_Bool_t        is_subtitle;    /**< is subtitle or not*/
	uint8_t         **bitmap;         /**< draw bitmap buffer*/
	int              pitch;          /**< the length of draw bitmap buffer per line*/
	void            *user_data;      /**< user data*/
	int             default_region;  /**< default region，see vbi_font_descriptors in libzvbi/src/lang.c*/
}AM_TT2_Para_t;

/**\brief creat teletext parser handle
 * \param[out] handle the handle of parser
 * \param[in] para teletext parse parameter
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_Create(AM_TT2_Handle_t *handle, AM_TT2_Para_t *para);

/**\brief destory teletext parser handle
 * \param handle the handle of parser
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_Destroy(AM_TT2_Handle_t handle);

/**\brief set subtitle mode
 * \param handle the handle of parser
 * \param subtitle true:set subtitle, false:not set
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_SetSubtitleMode(AM_TT2_Handle_t handle, AM_Bool_t subtitle);
extern AM_ErrorCode_t AM_TT2_SetTTDisplayMode(AM_TT2_Handle_t handle, int mode);
extern AM_ErrorCode_t AM_TT2_SetRevealMode(AM_TT2_Handle_t handle, int mode);
extern AM_ErrorCode_t AM_TT2_LockSubpg(AM_TT2_Handle_t handle, int lock);
extern AM_ErrorCode_t AM_TT2_GotoSubtitle(AM_TT2_Handle_t handle);
extern AM_ErrorCode_t AM_TT2_SetRegion(AM_TT2_Handle_t handle, int region_id);


/**\brief get user private data
 * \param handle the handle of parser
 * \return user private data
 */
extern void*          AM_TT2_GetUserData(AM_TT2_Handle_t handle);

/**\brief decode teletext data
 * \param handle the handle of parser
 * \param[in] buf teletext buffer
 * \param size teletext buffer length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_Decode(AM_TT2_Handle_t handle, uint8_t *buf, int size);

/**\brief start show teletext
 * \param handle the handle of parser
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_Start(AM_TT2_Handle_t handle, int region_id);

/**\brief stop show teletext
 * \param handle the handle of parser
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_Stop(AM_TT2_Handle_t handle);

/**\brief jump to the specific page
 * \param handle the handle of parser
 * \param page_no jump page
 * \param sub_page_no jump sub page
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_GotoPage(AM_TT2_Handle_t handle, int page_no, int sub_page_no);

/**\brief jump home page
 * \param handle the handle of parser
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_GoHome(AM_TT2_Handle_t handle);

/**\brief jump next page
 * \param handle the handle of parser
 * \param dir jump direction，+1:as positive，-1:as reverse
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_NextPage(AM_TT2_Handle_t handle, int dir);

/**\brief jump next sub page
 * \param handle the handle of parser
 * \param dir jump direction，+1:as positive，-1:as reverse
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_NextSubPage(AM_TT2_Handle_t handle, int dir);

/**\brief get sub page info
 * \param handle the handle of parser
 * \param pgno page number
 * \param [out] subs sub page number
 * \param [in] len in:subs length，out:The actual sub pages
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_GetSubPageInfo(AM_TT2_Handle_t handle, int pgno, int *subs, int *len);

/**\brief Jump to the specified link according to the color
 * \param [in] handle the handle of parser
 * \param [in] color color
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_ColorLink(AM_TT2_Handle_t handle, AM_TT2_Color_t color);

/**\brief Set search string
 * \param handle the handle of parser
 * \param pattern search string
 * \param casefold Whether Case sensitive
 * \param regex Whether to use regular expression matching
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_SetSearchPattern(AM_TT2_Handle_t handle, const char *pattern, AM_Bool_t casefold, AM_Bool_t regex);

/**\brief Search the specified page
 * \param handle the handle of parser
 * \param dir jump direction，+1:as positive，-1:as reverse
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_TT2_Search(AM_TT2_Handle_t handle, int dir);

#ifdef __cplusplus
}
#endif



#endif
