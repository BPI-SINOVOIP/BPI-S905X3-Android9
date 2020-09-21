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
 * \brief 音频输出模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "../am_adp_internal.h"
#include "am_aout_internal.h"
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/

/**\brief 音频输出设备*/
static AM_AOUT_Device_t aout_devices[] =
{
{
.lock  = PTHREAD_MUTEX_INITIALIZER
}
};

/**\brief 音频输出设备数*/
#define AOUT_DEV_COUNT    AM_ARRAY_SIZE(aout_devices)

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构*/
static AM_INLINE AM_ErrorCode_t aout_get_dev(int dev_no, AM_AOUT_Device_t **dev)
{
	if((dev_no<0) || (((size_t)dev_no)>=AOUT_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid aout device number %d, must in(%d~%d)", dev_no, 0, AOUT_DEV_COUNT-1);
		return AM_AOUT_ERR_INVALID_DEV_NO;
	}
	
	*dev = &aout_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t aout_get_openned_dev(int dev_no, AM_AOUT_Device_t **dev)
{
	AM_TRY(aout_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "aout device %d has not been openned", dev_no);
		return AM_AOUT_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief 重新设定音频输出*/
static AM_ErrorCode_t aout_reset(AM_AOUT_Device_t *dev)
{
	if(dev->drv->set_volume)
		dev->drv->set_volume(dev, dev->volume);

	if(dev->drv->set_mute)
		dev->drv->set_mute(dev, dev->mute);

	if(dev->drv->set_pre_gain)
	    dev->drv->set_pre_gain(dev, dev->pre_gain);

	if(dev->drv->set_pre_mute)
	    dev->drv->set_pre_mute(dev, dev->pre_mute);

	if(dev->drv->set_output_mode)
		dev->drv->set_output_mode(dev, dev->mode);
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开音频输出设备
 * \param dev_no 音频输出设备号
 * \param[in] para 音频输出设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_Open(int dev_no, const AM_AOUT_OpenPara_t *para)
{
	AM_AOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(aout_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "aout device %d has already been openned", dev_no);
		ret = AM_AOUT_ERR_BUSY;
		goto final;
	}
	
	pthread_mutex_lock(&dev->lock);
	
	dev->dev_no = dev_no;
	
	if(dev->drv && dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
	}
	
	if(ret==AM_SUCCESS)
	{
		dev->openned = AM_TRUE;
		dev->volume  = 100;
		dev->mute    = AM_FALSE;
		dev->mode    = AM_AOUT_OUTPUT_STEREO;
		
		if(dev->drv)
			aout_reset(dev);
	}
	
	pthread_mutex_unlock(&dev->lock);
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭音频输出设备
 * \param dev_no 音频输出设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_Close(int dev_no)
{
	AM_AOUT_Device_t *dev;
	
	AM_TRY(aout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv && dev->drv->close)
	{
		dev->drv->close(dev);
	}
	
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&dev->lock);
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return AM_SUCCESS;
}

/**\brief 设定音量(0~100)
 * \param dev_no 音频输出设备号
 * \param vol 音量值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_SetVolume(int dev_no, int vol)
{
	AM_AOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(aout_get_openned_dev(dev_no, &dev));

	vol = AM_MAX(vol, AM_AOUT_VOLUME_MIN);
	vol = AM_MIN(vol, AM_AOUT_VOLUME_MAX);
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->volume!=vol)
	{
		if(dev->drv && dev->drv->set_volume)
			ret = dev->drv->set_volume(dev, vol);
	
		if(ret==AM_SUCCESS)
		{
			dev->volume = vol;
			
			AM_EVT_Signal(dev_no, AM_AOUT_EVT_VOLUME_CHANGED, (void*)(long)vol);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前音量
 * \param dev_no 音频输出设备号
 * \param[out] vol 返回当前音量值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_GetVolume(int dev_no, int *vol)
{
	AM_AOUT_Device_t *dev;
	
	assert(vol);
	
	AM_TRY(aout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	*vol = dev->volume;
	
	pthread_mutex_unlock(&dev->lock);
	
	return AM_SUCCESS;
}

/**\brief 静音/取消静音
 * \param dev_no 音频输出设备号
 * \param mute AM_TRUE表示静音，AM_FALSE表示取消静音
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_SetMute(int dev_no, AM_Bool_t mute)
{
	AM_AOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(aout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	AM_DEBUG(1, "SetMute %d\n", mute?1:0);
	//if(dev->mute!=mute)
	{
		if(dev->drv && dev->drv->set_mute)
			ret = dev->drv->set_mute(dev, mute);
	
		if(ret==AM_SUCCESS)
		{
			dev->mute = mute;
			
			AM_EVT_Signal(dev_no, AM_AOUT_EVT_MUTE_CHANGED, (void*)(long)mute);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前静音状态
 * \param dev_no 音频输出设备号
 * \param[out] mute 返回当前静音状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_GetMute(int dev_no, AM_Bool_t *mute)
{
	AM_AOUT_Device_t *dev;
	
	assert(mute);
	
	AM_TRY(aout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	*mute = dev->mute;
	
	pthread_mutex_unlock(&dev->lock);
	
	return AM_SUCCESS;
}

/**\brief 设定音频输出模式
 * \param dev_no 音频输出设备号
 * \param mode 音频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_SetOutputMode(int dev_no, AM_AOUT_OutputMode_t mode)
{
	AM_AOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(aout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);

	/*do not check, for SWAP is a toggle operation*/
	//if(dev->mode!=mode)
	{
		if(dev->drv && dev->drv->set_output_mode)
			ret = dev->drv->set_output_mode(dev, mode);
		else
			ret = AM_FAILURE;
	
		if(ret==AM_SUCCESS)
		{
			dev->mode = mode;
			AM_EVT_Signal(dev_no, AM_AOUT_EVT_OUTPUT_MODE_CHANGED, (void*)(long)mode);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 返回当前音频输出模式
 * \param dev_no 音频输出设备号
 * \param[out] mode 返回当前音频输出模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_GetOutputMode(int dev_no, AM_AOUT_OutputMode_t *mode)
{
	AM_AOUT_Device_t *dev;
	
	assert(mode);
	
	AM_TRY(aout_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	*mode = dev->mode;
	
	pthread_mutex_unlock(&dev->lock);
	
	return AM_SUCCESS;
}

/**\brief 设定音频输出驱动
 * \param dev_no 音频输出设备号
 * \param[in] drv 音频输出驱动
 * \param[in] drv_data 驱动私有数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_SetDriver(int dev_no, const AM_AOUT_Driver_t *drv, void *drv_data)
{
	AM_AOUT_Device_t *dev;
	
	AM_TRY(aout_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if((drv!=dev->drv) || (drv_data!=dev->drv_data))
	{
		if(dev->drv && dev->openned)
		{
			if(dev->drv->close)
				dev->drv->close(dev);
		}
	
		dev->drv = drv;
		dev->drv_data = drv_data;
	
		if(drv && dev->openned)
		{
			if(dev->drv->open)
				dev->drv->open(dev, &dev->open_para);
		
			aout_reset(dev);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return AM_SUCCESS;
}

/**\brief 设定预增益
 * \param dev_no 音频输出设备号
 * \param gain 预增益值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_SetPreGain(int dev_no, float gain)
{
	AM_AOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(aout_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	AM_DEBUG(1, "SetPreGain %f\n", gain);
	//if(dev->pre_gain!=gain)
	{
		if(dev->drv && dev->drv->set_pre_gain)
			ret = dev->drv->set_pre_gain(dev, gain);

		if(ret==AM_SUCCESS)
		{
			dev->pre_gain = gain;

			AM_EVT_Signal(dev_no, AM_AOUT_EVT_PREGAIN_CHANGED, (void*)&gain);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前预增益
 * \param dev_no 音频输出设备号
 * \param[out] gain 返回当前预增益值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_GetPreGain(int dev_no, float *gain)
{
	AM_AOUT_Device_t *dev;

	assert(gain);

	AM_TRY(aout_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	*gain = dev->pre_gain;

	pthread_mutex_unlock(&dev->lock);

	AM_DEBUG(1, "GetPreGain %f\n", *gain);
	return AM_SUCCESS;
}

/**\brief 预静音/取消预静音
 * \param dev_no 音频输出设备号
 * \param mute AM_TRUE表示静音，AM_FALSE表示取消静音
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_SetPreMute(int dev_no, AM_Bool_t mute)
{
	AM_AOUT_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(aout_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);
	AM_DEBUG(1, "SetPreMute %d\n", mute?1:0);
	//if(dev->pre_mute!=mute)
	{
		if(dev->drv && dev->drv->set_pre_mute)
			ret = dev->drv->set_pre_mute(dev, mute);

		if(ret==AM_SUCCESS)
		{
			dev->pre_mute = mute;

			AM_EVT_Signal(dev_no, AM_AOUT_EVT_PREMUTE_CHANGED, (void*)(long)mute);
		}
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 取得当前预静音状态
 * \param dev_no 音频输出设备号
 * \param[out] mute 返回当前预静音状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_aout.h)
 */
AM_ErrorCode_t AM_AOUT_GetPreMute(int dev_no, AM_Bool_t *mute)
{
	AM_AOUT_Device_t *dev;

	assert(mute);

	AM_TRY(aout_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	*mute = dev->pre_mute;

	pthread_mutex_unlock(&dev->lock);

	return AM_SUCCESS;
}
