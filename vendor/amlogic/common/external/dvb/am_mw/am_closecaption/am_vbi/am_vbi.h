/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 解复用设备模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#ifndef _AM_VBI_H
#define _AM_VBI_H

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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sched.h>
#include <signal.h>
#ifdef ANDOIRD
#include <android/log.h>
#endif
#include <am_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief error code for AM_VBI*/
enum AM_VBI_ErrorCode
{
	AM_VBI_ERROR_BASE = 0,
	AM_VBI_ERR_INVALID_DEV_NO,          
	AM_VBI_ERR_INVALID_ID,             
	AM_VBI_ERR_BUSY,                    
	AM_VBI_ERR_NOT_ALLOCATED,
	AM_VBI_ERR_CANNOT_CREATE_THREAD,    
	AM_VBI_ERR_CANNOT_OPEN_DEV,         
	AM_VBI_ERR_NOT_SUPPORTED,           
	AM_VBI_ERR_NO_FREE_FILTER,          
	AM_VBI_ERR_NO_MEM,                  
	AM_VBI_ERR_TIMEOUT,                 
	AM_VBI_ERR_SYS,                     
	AM_VBI_ERR_NO_DATA,                 
	AM_VBI_ERR_END
};

#define VBI_IOC_MAGIC 'X'
#define VBI_IOC_CC_EN       _IO (VBI_IOC_MAGIC, 0x01)
#define VBI_IOC_CC_DISABLE  _IO (VBI_IOC_MAGIC, 0x02)
#define VBI_IOC_SET_FILTER  _IOW(VBI_IOC_MAGIC, 0x03, int)
#define VBI_IOC_S_BUF_SIZE  _IOW(VBI_IOC_MAGIC, 0x04, int)
#define VBI_IOC_START       _IO (VBI_IOC_MAGIC, 0x05)
#define VBI_IOC_STOP        _IO (VBI_IOC_MAGIC, 0x06)
#define DECODER_VBI_VBI_SIZE                0x1000

#define VBI_DEV_NO  0


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#ifdef ANDOIRD
#define LOG_TAG    "AM_VBI"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#define LOGI(...) printf(...)
#define LOGE(...) printf(...)
#endif



/****************************************************************************
 * Type definitions
 ***************************************************************************/
/**\brief */
typedef enum
{
	DUAL_FD1,                    /**< vbi 0 */
	DUAL_FD2,                    /**< vbi 1 */
} AM_VBI_Source_t;

/**\brief */
typedef struct
{
	int    foo;
} AM_VBI_OpenPara_t;

/**\brief  */
typedef void (*AM_VBI_DataCb) (int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);


extern AM_ErrorCode_t AM_NTSC_VBI_Open(int dev_no, const AM_VBI_OpenPara_t *para);


extern AM_ErrorCode_t AM_NTSC_VBI_Close(int dev_no);


extern AM_ErrorCode_t AM_NTSC_VBI_AllocateFilter(int dev_no, int *fhandle);


extern AM_ErrorCode_t AM_NTSC_VBI_StartFilter(int dev_no, int fhandle);


extern AM_ErrorCode_t AM_NTSC_VBI_SetBufferSize(int dev_no, int fhandle, int size);


extern AM_ErrorCode_t AM_NTSC_VBI_SetCallback(int dev_no, int fhandle, AM_VBI_DataCb cb, void *data);


extern AM_Bool_t AM_NTSC_VBI_Decoder_Test(int dev_no, int fid, const uint8_t *data, int len, void *user_data);


extern AM_ErrorCode_t AM_CC_Decode_Line21VBI(uint8_t data1, uint8_t data2,unsigned int n_lines);

extern AM_ErrorCode_t AM_NTSC_VBI_StopFilter(int dev_no, int fhandle);

extern AM_ErrorCode_t AM_NTSC_VBI_FreeFilter(int dev_no, int fhandle);

#ifdef __cplusplus
}
#endif

#endif

