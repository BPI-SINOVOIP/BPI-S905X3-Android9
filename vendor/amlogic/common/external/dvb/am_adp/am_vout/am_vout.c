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
 * \brief 视频输出模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "am_vout_internal.h"
#include "../am_adp_internal.h"
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef EMU_VOUT
extern const AM_VOUT_Driver_t emu_drv;
#else
extern const AM_VOUT_Driver_t aml_vout_drv;
#endif

static AM_VOUT_Device_t vout_devices[] =
{
#ifdef EMU_VOUT
{
.drv = &emu_drv
}
#else
{
.drv = &aml_vout_drv
}
#endif
};

/**\brief 视频输出设备数*/
#define VOUT_DEV_COUNT    AM_ARRAY_SIZE(vout_devices)

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构*/
static AM_INLINE AM_ErrorCode_t vout_get_dev(int dev_no, AM_VOUT_Device_t **dev)
{
	if((dev_no<0) || (((size_t)dev_no)>=VOUT_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid vout device number %d, must in(%d~%d)", dev_no, 0, VOUT_DEV_COUNT-1);
		return AM_VOUT_ERR_INVALID_DEV_NO;
	}
	
	*dev = &vout_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t vout_get_openned_dev(int dev_no, AM_VOUT_Device_t **dev)
{
	AM_TRY(vout_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "vout device %d has not been openned", dev_no);
		return AM_VOUT_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}


/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开视频输出设备
 * \param dev_no 视频输出设备号
 * \param[in] para 视频输出设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
AM_ErrorCode_t AM_VOUT_Open(int dev_no, const AM_VOUT_OpenPara_t *para)
{
	AM_VOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(vout_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "vout device %d has already been openned", dev_no);
		ret = AM_VOUT_ERR_BUSY;
		goto final;
	}
	
	dev->dev_no = dev_no;
	dev->format = AM_VOUT_FORMAT_720P;
	dev->enable = AM_TRUE;
	
	if(dev->drv->open)
	{
		AM_TRY_FINAL(dev->drv->open(dev, para));
	}
	
	pthread_mutex_init(&dev->lock, NULL);
	dev->openned = AM_TRUE;
	
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭视频输出设备
 * \param dev_no 视频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
AM_ErrorCode_t AM_VOUT_Close(int dev_no)
{
	AM_VOUT_Device_t *dev;
	
	AM_TRY(vout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->drv->close)
	{
		dev->drv->close(dev);
	}
	
	pthread_mutex_destroy(&dev->lock);
	
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return AM_SUCCESS;
}

/**\brief 设定输出模式
 * \param dev_no 视频输出设备号
 * \param fmt 视频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
AM_ErrorCode_t AM_VOUT_SetFormat(int dev_no, AM_VOUT_Format_t fmt)
{
	AM_VOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(vout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->format!=fmt)
	{
		if(dev->drv->set_format)
		{
			ret = dev->drv->set_format(dev, fmt);
		}
	
		if(ret==AM_SUCCESS)
		{
			dev->format = fmt;
			AM_EVT_Signal(dev_no, AM_VOUT_EVT_FORMAT_CHANGED, (void*)fmt);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前输出模式
 * \param dev_no 视频输出设备号
 * \param[out] fmt 返回当前视频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
AM_ErrorCode_t AM_VOUT_GetFormat(int dev_no, AM_VOUT_Format_t *fmt)
{
	AM_VOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(fmt);
	
	AM_TRY(vout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	*fmt = dev->format;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 使能视频信号输出
 * \param dev_no 视频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
AM_ErrorCode_t AM_VOUT_Enable(int dev_no)
{
	AM_VOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(vout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->enable)
	{
		if(dev->drv->enable)
		{
			ret = dev->drv->enable(dev, AM_TRUE);
		}
	
		if(ret==AM_SUCCESS)
		{
			dev->enable = AM_TRUE;
			AM_EVT_Signal(dev_no, AM_VOUT_EVT_ENABLE, NULL);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 停止视频信号输出
 * \param dev_no 视频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_vout.h)
 */
AM_ErrorCode_t AM_VOUT_Disable(int dev_no)
{
	AM_VOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(vout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->enable)
	{
		if(dev->drv->enable)
		{
			ret = dev->drv->enable(dev, AM_FALSE);
		}
	
		if(ret==AM_SUCCESS)
		{
			dev->enable = AM_TRUE;
			AM_EVT_Signal(dev_no, AM_VOUT_EVT_DISABLE, NULL);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

