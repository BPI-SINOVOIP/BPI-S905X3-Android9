/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Teletextģ��(version 2)
 *
 * \author kui.zhang <kui.zhang@amlogic.com>
 * \date 2012-09-03: create the document
 ***************************************************************************/



#ifndef _AM_NTSC_CC_H
#define _AM_NTSC_CC_H


#include <unistd.h>
#include "misc.h"
#include "trigger.h"
#include "format.h"
#include "lang.h"
#include "hamm.h"
#include "tables.h"
#include "vbi.h"
#include <android/log.h>    
#include <am_xds.h>    
#include "vbi_dmx.h"
#ifdef __cplusplus
extern "C"
{
#endif



enum AM_CC_ErrorCode
{
	AM_CC_ERROR_BASE =0,
	AM_CC_ERR_INVALID_PARAM,   /**< ������Ч*/
	AM_CC_ERR_INVALID_HANDLE,  /**< �����Ч*/
	AM_CC_ERR_NOT_SUPPORTED,   /**< ��֧�ֵĲ���*/
	AM_CC_ERR_CREATE_DECODE,   /**< ��cc������ʧ��*/
	AM_CC_ERR_OPEN_PES,        /**< ��PESͨ��ʧ��*/
	AM_CC_ERR_SET_BUFFER,      /**< ����PES ������ʧ��*/
	AM_CC_ERR_NO_MEM,                  /**< �����ڴ治��*/
	AM_CC_ERR_CANNOT_CREATE_THREAD,    /**< �޷������߳�*/
	AM_CC_ERR_NOT_RUN,            /**< �޷������߳�*/
	AM_CC_INIT_DISPLAY_FAILED,    /**< ��ʼ����ʾ��Ļʧ��*/
	AM_CC_ERR_END
};


/****************************************************************************
 * Type definitions
 ***************************************************************************/
 /**\brief Teletext���������*/
typedef void* AM_VBI_Handle_t;

/**\brief ��ʼ����*/
typedef void (*AM_VBI_CC_DrawBegin_t)(AM_VBI_Handle_t handle);

/**\brief ��������*/
typedef void (*AM_VBI_CC_DrawEnd_t)(AM_VBI_Handle_t handle);

/**\brief notify xds data*/
typedef void (*AM_VBI_xds_callback_t)(AM_VBI_Handle_t handle, vbi_xds_subclass_program xds_program,vbi_program_info 	prog_info);

 typedef struct
{
	AM_VBI_CC_DrawBegin_t draw_begin;   /**< ��ʼ����*/
	AM_VBI_CC_DrawEnd_t   draw_end;     /**< ��������*/
	vbi_bool        is_subtitle;    /**< �Ƿ�Ϊ��Ļ*/
	uint8_t         *bitmap;         /**< ��ͼ������*/
	int              pitch;          /**< ��ͼ������ÿ���ֽ���*/
	void            *user_data;      /**< �û���������*/
}AM_VBI_CC_Para_t;

 typedef struct
{
	AM_VBI_xds_callback_t        xds_callback;      /**< notify to user */
}AM_VBI_XDS_Para_t;



typedef enum {
	VBI_CC1 = 0x01,
	VBI_CC2,
	VBI_CC3,
	VBI_CC4,
	VBI_TT1,
	VBI_TT2,
	VBI_TT3,
	VBI_TT4,

} VBI_CC_TYPE;


extern vbi_bool AM_VBI_CC_Create(AM_VBI_Handle_t *handle, AM_VBI_CC_Para_t *para);

extern  AM_ErrorCode_t AM_VBI_CC_Start(AM_VBI_Handle_t handle);

extern  AM_ErrorCode_t AM_VBI_CC_Stop(AM_VBI_Handle_t handle);

extern vbi_bool
decode_vbi		(int dev_no, int fid, const uint8_t *data, int len, void *user_data);

extern void* AM_VBI_CC_GetUserData(AM_VBI_Handle_t handle);

extern vbi_bool AM_VBI_CC_set_type(AM_VBI_Handle_t handle,VBI_CC_TYPE cc_type);  //*cc_type caption 1-4, text 1-4, garbage */

extern vbi_bool AM_VBI_CC_set_status(AM_VBI_Handle_t handle,vbi_bool flag);

extern vbi_bool AM_VBI_XDS_Create(AM_VBI_Handle_t *handle,AM_VBI_XDS_Para_t *para);



#ifdef __cplusplus
}
#endif



#endif
