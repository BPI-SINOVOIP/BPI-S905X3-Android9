/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 
 *
 * \author  <jianhui.zhou@amlogic.com>
 * \date 2014-06-13: create the document
 ***************************************************************************/

#ifndef _AM_VBI_INTERNAL_H
#define _AM_VBI_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include <am_types.h>
#include "am_vbi.h"

 #ifdef __cplusplus
 extern "C"
 {
 #endif



/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define VBI_FILTER_COUNT      (1)

#define VBI_DRIVER_LENGTH     (32)

#define VBI_FL_RUN_CB         (1)

/****************************************************************************
 * Type definitions
 ***************************************************************************/
/**\brief*/
typedef struct AM_VBI_Device AM_VBI_Device_t;

/**\brief */
typedef struct AM_VBI_Filter  AM_VBI_Filter_t;


/**\brief*/
typedef uint32_t AM_VBI_FilterMask_t;

#define AM_VBI_FILTER_MASK_ISEMPTY(m)    (!(*(m)))
#define AM_VBI_FILTER_MASK_CLEAR(m)      (*(m)=0)
#define AM_VBI_FILTER_MASK_ISSET(m,i)    (*(m)&(1<<(i)))
#define AM_VBI_FILTER_MASK_SET(m,i)      (*(m)|=(1<<(i)))

/**\brief*/
typedef struct
{
	AM_ErrorCode_t (*open)(AM_VBI_Device_t *dev, const AM_VBI_OpenPara_t *para);
	AM_ErrorCode_t (*close)(AM_VBI_Device_t *dev);
	AM_ErrorCode_t (*alloc_filter)(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter);
	AM_ErrorCode_t (*free_filter)(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter);
	AM_ErrorCode_t (*enable_filter)(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, AM_Bool_t enable);
	AM_ErrorCode_t (*set_buf_size)(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, int size);
	AM_ErrorCode_t (*poll)(AM_VBI_Device_t *dev, AM_VBI_FilterMask_t *mask, int timeout);
	AM_ErrorCode_t (*read)(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, uint8_t *buf, int *size);
} AM_VBI_Driver_t;

/**\brief Section*/
struct AM_VBI_Filter
{	void      *drv_data; 
	AM_Bool_t  used;    
	AM_Bool_t  enable;   
	int       id;       
        AM_VBI_DataCb    cb; 
	void     *user_data; 
};

/**\brief */
struct AM_VBI_Device
{
	int                 dev_no;  
	const AM_VBI_Driver_t *drv;  
	void               *drv_data;
        AM_VBI_Filter_t     filters[VBI_FILTER_COUNT];   
	AM_Bool_t            openned;                                  
	AM_Bool_t            enable_thread; 
	int                 flags;   
	pthread_t           thread; 
	pthread_mutex_t     lock;    
	pthread_cond_t      cond;   
	AM_VBI_Source_t     src;
};


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


 #ifdef __cplusplus
 }
 #endif

#endif

