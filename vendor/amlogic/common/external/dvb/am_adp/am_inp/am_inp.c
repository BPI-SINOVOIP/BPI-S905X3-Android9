#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 输入设备模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-19: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_util.h>
#include <am_time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "am_inp_internal.h"
#include "../am_adp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief 事件处理线程中，等待输入事件的超时时间(毫秒为单位)*/
#define INP_WAIT_TIMEOUT	(500)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/
 
#ifdef LINUX_INPUT
extern const AM_INP_Driver_t linux_input_drv;
#endif
#ifdef TTY_INPUT
extern const AM_INP_Driver_t tty_drv;
#endif
#ifdef SDL_INPUT
extern const AM_INP_Driver_t sdl_drv;
#endif

static AM_INP_Device_t inp_devices[] =
{
#ifdef LINUX_INPUT
{
.drv = &linux_input_drv
},
{
.drv = &linux_input_drv
}
#endif
#ifdef TTY_INPUT
{
.drv = &tty_drv
}
#endif
#ifdef SDL_INPUT
{
.drv = &sdl_drv
}
#endif
};

/**\brief 输入设备数*/
#define INP_DEV_COUNT    AM_ARRAY_SIZE(inp_devices)

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构*/
static AM_INLINE AM_ErrorCode_t inp_get_dev(int dev_no, AM_INP_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=INP_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid input device number %d, must in(%d~%d)", dev_no, 0, INP_DEV_COUNT-1);
		return AM_INP_ERR_INVALID_DEV_NO;
	}
	
	*dev = &inp_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t inp_get_openned_dev(int dev_no, AM_INP_Device_t **dev)
{
	AM_TRY(inp_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "input device %d has not been openned", dev_no);
		return AM_INP_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief 查找按键映射表*/
static AM_INP_Key_t inp_lookup_map(AM_INP_Device_t *dev, int code)
{
	int i;
	
	if(!dev->enable_map) return code;
	
	for(i=0; i<AM_INP_KEY_COUNT; i++)
	{
		if(dev->map.codes[i]==code)
			return i;
	}
	
	return KEY_RESERVED;
}

/**\brief 输入事件检测线程*/
static void* inp_event_thread(void *arg)
{
	AM_INP_Device_t *dev = (AM_INP_Device_t*)arg;
	struct input_event evt;
	AM_ErrorCode_t ret;
	
	AM_DEBUG(2, "input %d thread begin", dev->dev_no);
	
	while(dev->enable_thread)
	{
		ret = dev->drv->wait(dev, &evt, INP_WAIT_TIMEOUT);
		if(ret==AM_SUCCESS)
		{
			pthread_mutex_lock(&dev->lock);
			evt.code = inp_lookup_map(dev, evt.code);
			dev->flags |= INP_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			
			if(dev->enable && dev->event_cb && (evt.code!=KEY_RESERVED))
			{
				dev->event_cb(dev->dev_no, &evt, dev->user_data);
			}
			
			pthread_mutex_lock(&dev->lock);
			dev->flags &= ~INP_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);
		}
	}
	
	AM_DEBUG(2, "input %d thread end", dev->dev_no);
	
	return NULL;
}

/**\brief Wait the callback function*/
static AM_ErrorCode_t inp_wait_cb(AM_INP_Device_t *dev)
{
	if(dev->event_thread!=pthread_self())
	{
		while(dev->flags&INP_FL_RUN_CB)
			pthread_cond_wait(&dev->cond, &dev->lock);
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开一个输入设备
 * \param dev_no 输入设备号
 * \param[in] para 设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_Open(int dev_no, const AM_INP_OpenPara_t *para)
{
	AM_INP_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(inp_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "input device %d has already been openned", dev_no);
		ret = AM_INP_ERR_BUSY;
		goto final;
	}
	
	dev->dev_no = dev_no;
	
	if(dev->drv->open)
	{
		AM_TRY_FINAL(dev->drv->open(dev, para));
	}
	
	pthread_mutex_init(&dev->lock, NULL);
	pthread_cond_init(&dev->cond, NULL);
	
	dev->enable = AM_TRUE;
	dev->enable_map = AM_FALSE;
	dev->openned = AM_TRUE;
	dev->event_cb = NULL;
	dev->user_data = NULL;
	dev->flags = 0;
	dev->enable_thread = para->enable_thread;
	
	if(dev->enable_thread)
	{
		if(pthread_create(&dev->event_thread, NULL, inp_event_thread, dev))
		{
			AM_DEBUG(1, "cannot create the input event thread");
			pthread_mutex_destroy(&dev->lock);
			pthread_cond_destroy(&dev->cond);
			if(dev->drv->close)
			{
				dev->drv->close(dev);
			}
			ret = AM_INP_ERR_CANNOT_CREATE_THREAD;
			goto final;
		}
	}
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭一个输入设备
 * \param dev_no 输入设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_Close(int dev_no)
{
	AM_INP_Device_t *dev;
	
	AM_TRY(inp_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	/*停止事件检测线程*/
	if(dev->enable_thread)
	{
		dev->enable_thread = AM_FALSE;
		pthread_join(dev->event_thread, NULL);
	}
	
	/*关闭设备*/
	if(dev->drv->close)
	{
		dev->drv->close(dev);
	}
	
	pthread_mutex_destroy(&dev->lock);
	pthread_cond_destroy(&dev->cond);
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return AM_SUCCESS;
}

/**\brief 使能输入设备，是能之后输入设备的按键会被读取
 * \param dev_no 输入设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_Enable(int dev_no)
{
	AM_INP_Device_t *dev;
	
	AM_TRY(inp_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->enable)
	{
		inp_wait_cb(dev);
		dev->enable = AM_TRUE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return AM_SUCCESS;
}

/**\brief 禁止输入设备,禁止之后，设备不会返回输入事件
 * \param dev_no 输入设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_Disable(int dev_no)
{
	AM_INP_Device_t *dev;
	
	AM_TRY(inp_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->enable)
	{
		inp_wait_cb(dev);
		dev->enable = AM_FALSE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return AM_SUCCESS;
}

/**\brief 等待一个输入事件
 *挂起调用线程直到输入设备产生一个输入事件。如果超过超时时间没有事件，返回AM_INP_ERR_TIMEOUT。
 * \param dev_no 输入设备号
 * \param[out] evt 返回输入事件
 * \param timeout 以毫秒为单位的超时时间，<0表示永远等待，=0表示检查后不等待立即返回
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_WaitEvent(int dev_no, struct input_event *evt, int timeout)
{
	AM_INP_Device_t *dev;
	int now, end;
	AM_Bool_t got = AM_FALSE;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(evt);
	
	AM_TRY(inp_get_openned_dev(dev_no, &dev));
	
	if(dev->enable_thread)
	{
		AM_DEBUG(1, "input event thread is running");
		return AM_FAILURE;
	}
	
	if(!dev->drv->wait)
	{
		AM_DEBUG(1, "driver do not support wait function");
		return AM_INP_ERR_NOT_SUPPORTED;
	}
	
	AM_TIME_GetClock(&now);
	end = now+timeout;
	
	do
	{
		ret = dev->drv->wait(dev, evt, end-now);
		if(dev->enable)
		{
			got = AM_TRUE;
			break;
		}
		
		AM_TIME_GetClock(&now);
	}
	while((now-end)<0);
	
	if(got)
	{
		pthread_mutex_lock(&dev->lock);
		evt->code = inp_lookup_map(dev, evt->code);
		pthread_mutex_unlock(&dev->lock);
	
		if(evt->code==KEY_RESERVED)
		{
			ret = AM_INP_ERR_UNKNOWN_KEY;
		}
	}
	else
	{
		ret = AM_INP_ERR_TIMEOUT;
	}
	
	return ret;
}

/**\brief 向输入设备注册一个事件回调函数
 *输入设备将创建一个线程，检查输入输入事件，如果有输入事件，线程调用回调函数。
 *如果回调函数为NULL,表示取消回调函数，输入设备将结束事件检测线程。
 * \param dev_no 输入设备号
 * \param[in] cb 新注册的回调函数
 * \param[in] user_data 回调函数的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_SetCallback(int dev_no,
                               AM_INP_EventCb_t cb,
                               void *user_data)
{
	AM_INP_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(inp_get_openned_dev(dev_no, &dev));
	
	if(!dev->enable_thread)
	{
		AM_DEBUG(1, "input event thread is not running");
		return AM_FAILURE;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->event_cb!=cb)
	{
		/*等待回调函数执行完*/
		inp_wait_cb(dev);
		
		dev->event_cb = cb;
		dev->user_data = user_data;
	}

	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得输入设备已经注册事件回调函数及参数
 * \param dev_no 输入设备号
 * \param[out] cb 返回回调函数指针
 * \param[out] user_data 返回回调函数的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_GetCallback(int dev_no, AM_INP_EventCb_t *cb, void **user_data)
{
	AM_INP_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(inp_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(cb)
	{
		*cb = dev->event_cb;
	}
	
	if(user_data)
	{
		*user_data = dev->user_data;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定输入设备的键码映射表
 * \param dev_no 输入设备号
 * \param[in] map 新的键码映射表，如果为NULL表示不使用映射表，直接返回键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
AM_ErrorCode_t AM_INP_SetKeyMap(int dev_no, const AM_INP_KeyMap_t *map)
{
	AM_INP_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(inp_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(map)
	{
		dev->map = *map;
		dev->enable_map = AM_TRUE;
	}
	else
	{
		dev->enable_map = AM_FALSE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

