/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 解复用设备模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-21: create the document
 ***************************************************************************/

#ifndef _VBI_DMX_H
#define _VBI_DMX_H

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
#include <android/log.h>

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

/**\brief 解复用模块错误代码*/
enum AM_VBI_ErrorCode
{
	AM_VBI_DMX_ERROR_BASE = 0,
	AM_VBI_DMX_ERR_INVALID_DEV_NO,          /**< 设备号无效*/
	AM_VBI_DMX_ERR_INVALID_ID,              /**< 过滤器ID无效*/
	AM_VBI_DMX_ERR_BUSY,                    /**< 设备已经被打开*/
	AM_VBI_DMX_ERR_NOT_ALLOCATED,           /**< 设备没有分配*/
	AM_VBI_DMX_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_VBI_DMX_ERR_CANNOT_OPEN_DEV,         /**< 无法打开设备*/
	AM_VBI_DMX_ERR_NOT_SUPPORTED,           /**< 不支持的操作*/
	AM_VBI_DMX_ERR_NO_FREE_FILTER,          /**< 没有空闲的section过滤器*/
	AM_VBI_DMX_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_VBI_DMX_ERR_TIMEOUT,                 /**< 等待设备数据超时*/
	AM_VBI_DMX_ERR_SYS,                     /**< 系统操作错误*/
	AM_VBI_DMX_ERR_NO_DATA,                 /**< 没有收到数据*/
	AM_VBI_DMX_ERR_END
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

#ifndef AM_SUCCESS
#define AM_SUCCESS     (0)
#endif

#ifndef AM_FAILURE
#define AM_FAILURE     (-1)
#endif

#ifndef AM_TRUE
#define AM_TRUE        (1)
#endif

#ifndef AM_FALSE
#define AM_FALSE       (0)
#endif


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG    "VBI_DEMUX"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


//#define AM_DEBUG  printf
#define AM_DEBUG  LOGI


typedef int            AM_ErrorCode_t;


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 解复用设备输入源*/
typedef enum
{
	DUAL_FD1,                    /**< vbi 输入0*/
	DUAL_FD2,                    /**< vbi 输入1*/
} AM_VBI_DMX_Source_t;

/**\brief 解复用设备开启参数*/
typedef struct
{
	int    foo;
} AM_VBI_DMX_OpenPara_t;

/**\brief 数据回调函数
 * data为数据缓冲区指针，len为数据长度。如果data==NULL表示demux接收数据超时。
 */
typedef void (*AM_DMX_DataCb) (int dev_no, int fhandle, const uint8_t *data, int len, void *user_data);

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开解复用设备
 * \param dev_no 解复用设备号
 * \param[in] para 解复用设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_NTSC_DMX_Open(int dev_no, const AM_VBI_DMX_OpenPara_t *para);

/**\brief 关闭解复用设备
 * \param dev_no 解复用设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_NTSC_DMX_Close(int dev_no);

/**\brief 分配一个过滤器
 * \param dev_no 解复用设备号
 * \param[out] fhandle 返回过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_NTSC_DMX_AllocateFilter(int dev_no, int *fhandle);

/**\brief 设定Section过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] params Section过滤器参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
//extern AM_ErrorCode_t AM_DMX_SetSecFilter(int dev_no, int fhandle, const struct dmx_sct_filter_params *params);

/**\brief 设定PES过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] params PES过滤器参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
//extern AM_ErrorCode_t AM_DMX_SetPesFilter(int dev_no, int fhandle, const struct dmx_pes_filter_params *params);

/**\brief 释放一个过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
//extern AM_ErrorCode_t AM_DMX_FreeFilter(int dev_no, int fhandle);

/**\brief 让一个过滤器开始运行
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_NTSC_DMX_StartFilter(int dev_no, int fhandle);

/**\brief 停止一个过滤器
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
//extern AM_ErrorCode_t AM_DMX_StopFilter(int dev_no, int fhandle);

/**\brief 设置一个过滤器的缓冲区大小
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param size 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_NTSC_DMX_SetBufferSize(int dev_no, int fhandle, int size);

/**\brief 取得一个过滤器对应的回调函数和用户参数
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[out] cb 返回过滤器对应的回调函数
 * \param[out] data 返回用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
//extern AM_ErrorCode_t AM_DMX_GetCallback(int dev_no, int fhandle, AM_DMX_DataCb *cb, void **data);

/**\brief 设置一个过滤器对应的回调函数和用户参数
 * \param dev_no 解复用设备号
 * \param fhandle 过滤器句柄
 * \param[in] cb 回调函数
 * \param[in] data 回调函数的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
extern AM_ErrorCode_t AM_NTSC_DMX_SetCallback(int dev_no, int fhandle, AM_DMX_DataCb cb, void *data);

/**\brief 设置解复用设备的输入源
 * \param dev_no 解复用设备号
 * \param src 输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
//extern AM_ErrorCode_t AM_DMX_SetSource(int dev_no, AM_DMX_Source_t src);

/**\brief DMX同步，可用于等待回调函数执行完毕
 * \param dev_no 解复用设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dmx.h)
 */
//extern AM_ErrorCode_t AM_DMX_Sync(int dev_no);

#ifdef __cplusplus
}
#endif

#endif

