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
 * \brief 解扰器模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5
#include <stdio.h>
#include <string.h>
#include <am_debug.h>
#include <am_mem.h>
#include "am_dsc_internal.h"
#include "../am_adp_internal.h"
#include <assert.h>
#include "am_misc.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <am_kl.h>

/****************************************************************************
 * Structures
 ***************************************************************************/
/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#ifdef EMU_DSC
#define dsc_get_driver() &emu_dsc_drv
#else
#define dsc_get_driver() &aml_dsc_drv
#endif

#ifdef EMU_DSC
extern const AM_DSC_Driver_t emu_dsc_drv;
#else
extern const AM_DSC_Driver_t aml_dsc_drv;
#endif
extern AM_ErrorCode_t am_kl_get_config(struct meson_kl_config *kl_config);


/****************************************************************************
 * Definations
 ***************************************************************************/
#define DISABLE_SEC_DSC 0
#define TAG "AMDSC"
#define LOGD(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)



/****************************************************************************
 * Static data
 ***************************************************************************/
static AM_DSC_Device_t *dsc_devices;
static AM_DSC_WorkMode_e g_dsc_sec_mode = DSC_MODE_MAX;

#if DISABLE_SEC_DSC == 0
	static void* g_dsc_sec_lib_handle;
#endif

/****************************************************************************
 * Functions  According to funcions in sec_dsc.c
 ***************************************************************************/
AM_ErrorCode_t (*AM_DSC_SEC_Open)(uint8_t dsc_no);
AM_ErrorCode_t (*AM_DSC_SEC_SetChannelPid)(uint8_t dsc_no, int channel_num, int pid);
AM_ErrorCode_t (*AM_DSC_SEC_AllocChannel)(uint8_t dsc_no, uint8_t *channel_num_return);
AM_ErrorCode_t (*AM_DSC_SEC_FreeChannel)(uint8_t dsc_no, int channel_num);
AM_ErrorCode_t (*AM_DSC_SEC_SetKey)(
	uint8_t dsc_no,
	int stream_path,
	int pid,
	int parity,
	const uint8_t* key,
	int key_len,
	AM_DSC_KeyType_t key_type);
AM_ErrorCode_t (*AM_DSC_SEC_KeyladderRun)(
	uint8_t dsc_no,
	int stream_path,
	int pid,
	uint8_t parity,
	struct meson_kl_config *kl_config,
	AM_DSC_KeyType_t key_type);
AM_ErrorCode_t (*AM_DSC_SEC_KeyladderCR)(int level, char keys[16]);
AM_ErrorCode_t (*AM_DSC_SEC_Close)(uint8_t dsc_no);
AM_ErrorCode_t (*AM_DSC_SEC_SetOutput)(int module, int output);

int (*alloc_dsc_sec_channel)(int* channel);

/***************************************************************************/
static AM_ErrorCode_t get_functions()
{
	AM_DSC_SEC_Open = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_Open");
	AM_DSC_SEC_SetChannelPid = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_SetChannelPid");
	AM_DSC_SEC_AllocChannel = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_AllocChannel");
	AM_DSC_SEC_FreeChannel = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_FreeChannel");
	AM_DSC_SEC_SetKey = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_SetKey");
	AM_DSC_SEC_KeyladderRun = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_KeyladderRun");
	AM_DSC_SEC_KeyladderCR = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_KeyladderCR");
	AM_DSC_SEC_Close = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_Close");
	AM_DSC_SEC_SetOutput = dlsym(g_dsc_sec_lib_handle, "AM_DSC_SEC_SetOutput");

	if (!AM_DSC_SEC_Open || !AM_DSC_SEC_SetChannelPid ||
		!AM_DSC_SEC_AllocChannel || !AM_DSC_SEC_FreeChannel ||
		!AM_DSC_SEC_SetKey || !AM_DSC_SEC_KeyladderRun ||
		!AM_DSC_SEC_KeyladderCR|| !AM_DSC_SEC_SetOutput)
	{
		printf("ADP_DSC: get sec_dsc functions failed\n");
		return AM_FALSE;
	}
	else
	{
		printf("ADP_DSC: get sec_dsc functions ok\n");
		return AM_SUCCESS;
	}
}

static AM_ErrorCode_t get_work_mode()
{
/* Try using sec dsc enviorment */
//TODO: 不允许动态切换，开机之后就固定模式，之后均是返回第一次的值
#if DISABLE_SEC_DSC == 0
	int ret;
	struct stat file_stat;
	if (DSC_MODE_MAX == g_dsc_sec_mode)
	{
		g_dsc_sec_lib_handle = dlopen(DSC_SEC_CaLib, RTLD_NOW);
		if (!g_dsc_sec_lib_handle)
		{
			AM_DEBUG(1, "ADP_DSC: dlopen %s failed\n", DSC_SEC_CaLib);
			g_dsc_sec_mode = DSC_MODE_NORMAL;
		}
		else
		{
			AM_DEBUG(1, "ADP_DSC: dlopen %s ok\n", DSC_SEC_CaLib);
			g_dsc_sec_mode = DSC_MODE_SEC;
		}

		if (g_dsc_sec_mode == DSC_MODE_SEC)
		{
			if (AM_SUCCESS != get_functions())
				g_dsc_sec_mode = DSC_MODE_NORMAL;
		}
	}
	AM_DEBUG(1, "ADP_DSC: Work mode %s\n", g_dsc_sec_mode==0?"DSC_MODE_NORMAL":"DSC_MODE_SEC");
	return AM_SUCCESS;
#else
	g_dsc_sec_mode = DSC_MODE_NORMAL;
	return AM_SUCCESS;
#endif
}

static AM_ErrorCode_t dsc_init_dev_db( int *dev_num )
{
	char buf[32];
	static int num = 1;
	static int init = 0;

	if (init) {
		*dev_num = num;
		return AM_SUCCESS;
	}

	if(AM_FileRead("/sys/module/aml_dmx/parameters/dsc_max", buf, sizeof(buf)) >= 0)
		sscanf(buf, "%d", &num);
	else
		num = 2;

	if (num) {
		int i;

		if (!(dsc_devices = malloc(sizeof(AM_DSC_Device_t)*num))) {
			AM_DEBUG(1, "ADP_DSC: no memory for dsc init");
			*dev_num = 0;
			return AM_DSC_ERR_NOT_ALLOCATED;
		}
		memset(dsc_devices, 0, sizeof(AM_DSC_Device_t)*num);
		for(i=0; i<num; i++)
			dsc_devices[i].drv = dsc_get_driver();
	}

	*dev_num = num;
	init = 1;

	return AM_SUCCESS;
}

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t dsc_get_dev(int dev_no, AM_DSC_Device_t **dev)
{
	int dev_cnt;

	AM_TRY(dsc_init_dev_db(&dev_cnt));

	if ((dev_no<0) || (dev_no >= dev_cnt))
	{
		AM_DEBUG(1, "ADP_DSC: invalid dsc device number %d, must in(%d~%d)", dev_no, 0, dev_cnt-1);
		return AM_DSC_ERR_INVALID_DEV_NO;
	}
	
	*dev = &dsc_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t dsc_get_openned_dev(int dev_no, AM_DSC_Device_t **dev)
{
	AM_TRY(dsc_get_dev(dev_no, dev));
	
	if (!(*dev)->openned)
	{
		AM_DEBUG(1, "ADP_DSC: dsc device %d has not been openned", dev_no);
		return AM_DSC_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief 根据ID取得对应解扰通道，并检查通道是否在使用*/
static AM_INLINE AM_ErrorCode_t dsc_get_used_chan(AM_DSC_Device_t *dev, int chan_id, AM_DSC_Channel_t **pchan)
{
	AM_DSC_Channel_t *chan;
	
	if ((chan_id<0) || (chan_id >= DSC_CHANNEL_COUNT))
	{
		AM_DEBUG(1, "ADP_DSC: invalid channel id, must in %d~%d", 0, DSC_CHANNEL_COUNT-1);
		return AM_DSC_ERR_INVALID_ID;
	}
	
	chan = &dev->channels[chan_id];
	
	if (!chan->used)
	{
		AM_DEBUG(1, "ADP_DSC: channel %d has not been allocated", chan_id);
		return AM_DSC_ERR_NOT_ALLOCATED;
	}
	
	*pchan = chan;
	return AM_SUCCESS;
}

/**\brief 释放解扰通道*/
static AM_ErrorCode_t dsc_free_chan(AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan)
{
	if (!chan->used)
		return AM_SUCCESS;
	
	if (dev->drv->free_chan)
		dev->drv->free_chan(dev, chan);
	
	chan->used = AM_FALSE;
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/
/**\brief 打开解扰器设备
 * \param dev_no 解扰器设备号
 * \param[in] para 解扰器设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_Open(int dev_no, const AM_DSC_OpenPara_t *para)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	void* dl_handle = NULL;

	assert(para);
	
	AM_TRY(dsc_get_dev(dev_no, &dev));

	if (AM_SUCCESS != get_work_mode())
		return AM_FAILURE;

	pthread_mutex_lock(&am_gAdpLock);
	
	if (dev->openned)
	{
		AM_DEBUG(1, "ADP_DSC: dsc device %d has already been openned", dev_no);
		ret = AM_DSC_ERR_BUSY;
		goto final;
	}
	
	dev->dev_no = dev_no;
	
	if (g_dsc_sec_mode == DSC_MODE_NORMAL && dev->drv->open)
		ret = dev->drv->open(dev, para);
	else
		ret = AM_DSC_SEC_Open(dev_no);
	
	if (ret == AM_SUCCESS)
	{
		pthread_mutex_init(&dev->lock, NULL);
		dev->openned = AM_TRUE;
	}
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 分配一个解扰通道
 * \param dev_no 解扰器设备号
 * \param[out] chan_id 返回解扰通道ID
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_AllocateChannel(int dev_no, int *chan_id)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_DSC_Channel_t *chan = NULL;
	int i;
	
	assert(chan_id);
	
	AM_TRY(dsc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	for(i=0; i<DSC_CHANNEL_COUNT; i++)
	{
		if (!dev->channels[i].used)
		{
			chan = &dev->channels[i];
			break;
		}
	}
	
	if (i >= DSC_CHANNEL_COUNT)
	{
		AM_DEBUG(1, "ADP_DSC: No more free channels");
		ret = AM_DSC_ERR_NO_FREE_CHAN;
	}
	
	if (ret == AM_SUCCESS)
	{
		chan->id   = i;
		chan->pid  = 0xFFFF;
		chan->used = AM_TRUE;
		if (g_dsc_sec_mode == DSC_MODE_NORMAL)
		{
			if (dev->drv->alloc_chan)
				ret = dev->drv->alloc_chan(dev, chan);
		}
		else if (g_dsc_sec_mode == DSC_MODE_SEC)
		{
			ret = AM_DSC_SEC_AllocChannel(dev_no, &chan->stream_path);
			printf("AM_DSC_SEC_AllocChannel id: %d\n", chan->stream_path);
		}
		else
			ret = AM_FAILURE;
		if (ret != AM_SUCCESS)
			chan->used = AM_FALSE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	*chan_id = i;
	
	return ret;
}

/**\brief 设定解扰通道对应流的PID值
 * \param dev_no 解扰器设备号
 * \param chan_id 解扰通道ID
 * \param pid 流的PID
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_SetChannelPID(int dev_no, int chan_id, uint16_t pid)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_DSC_Channel_t *chan;
	
	AM_TRY(dsc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dsc_get_used_chan(dev, chan_id, &chan);

	if (ret == AM_SUCCESS)
	{
		/*
		In kernel path, we set pid into driver, in sec linux, we set pid with
		other parameters together, so we just need to store pid.
		 */
		if (g_dsc_sec_mode == DSC_MODE_NORMAL)
		{
			if (dev->drv->set_pid)
				ret = dev->drv->set_pid(dev, chan, pid);
		}
	}
	
	if (ret == AM_SUCCESS)
		chan->pid = pid;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定解扰通道的控制字
 * \param dev_no 解扰器设备号
 * \param chan_id 解扰通道ID
 * \param type 控制字类型
 * \param[in] key 控制字
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_SetKey(int dev_no, int chan_id, AM_DSC_KeyType_t type, const uint8_t *key)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_DSC_Channel_t *chan;
	AM_DSC_KeyParity_e parity = 0;
	struct meson_kl_config kl_config;

	assert(key);
	/* Ciplus can only set to devno 0 */
	if (dev_no > 0)
	{
		if ((type & (~AM_DSC_KEY_FROM_KL)) > 1)
		{
			AM_DEBUG(1, "Ciplus can only set to devno 0\n");
			return AM_FAILURE;
		}
	}
	AM_TRY(dsc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dsc_get_used_chan(dev, chan_id, &chan);
	
	if (ret == AM_SUCCESS)
	{
		if (g_dsc_sec_mode == DSC_MODE_NORMAL)
		{
			if (dev->drv->set_key)
				ret = dev->drv->set_key(dev, chan, type, key);
		}
		else if (g_dsc_sec_mode == DSC_MODE_SEC)
		{
			switch (type&(~AM_DSC_KEY_FROM_KL))
			{
				case AM_DSC_KEY_TYPE_ODD:
					parity = DSC_KEY_ODD;
					break;

				case AM_DSC_KEY_TYPE_AES_ODD:
				case AM_DSC_KEY_TYPE_AES_IV_ODD:
					parity = DSC_KEY_ODD;
					break;

				case AM_DSC_KEY_TYPE_EVEN:
					parity = DSC_KEY_EVEN;
					break;

				case AM_DSC_KEY_TYPE_AES_EVEN:
				case AM_DSC_KEY_TYPE_AES_IV_EVEN:
					parity = DSC_KEY_EVEN;
					break;
			};

			if (type & AM_DSC_KEY_FROM_KL)
			{
				am_kl_get_config(&kl_config);
				ret = AM_DSC_SEC_KeyladderRun(
					dev_no,
					chan->stream_path,
					chan->pid,
					parity,
					&kl_config,
					type);
			}
			else
				ret = AM_DSC_SEC_SetKey(
					dev_no,
					chan->stream_path,
					chan->pid,
					parity,
					key,
					16,
					type);
		}
		else
			ret = AM_FAILURE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 释放一个解扰通道
 * \param dev_no 解扰器设备号
 * \param chan_id 解扰通道ID
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_FreeChannel(int dev_no, int chan_id)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_DSC_Channel_t *chan;
	
	AM_TRY(dsc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = dsc_get_used_chan(dev, chan_id, &chan);
	
	if (ret == AM_SUCCESS)
	{
		if (g_dsc_sec_mode == DSC_MODE_NORMAL)
			ret = dsc_free_chan(dev, chan);
		else if (g_dsc_sec_mode == DSC_MODE_SEC)
			ret = AM_DSC_SEC_FreeChannel(dev_no, chan->stream_path);
		else
			ret = AM_FAILURE;

		chan->used = AM_FALSE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 关闭解扰器设备
 * \param dev_no 解扰器设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_Close(int dev_no)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	AM_TRY(dsc_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	for(i=0; i<DSC_CHANNEL_COUNT; i++)
	{
		if (g_dsc_sec_mode == DSC_MODE_NORMAL)
			dsc_free_chan(dev, &dev->channels[i]);
		else if (g_dsc_sec_mode == DSC_MODE_SEC)
			ret = AM_DSC_SEC_FreeChannel(dev_no, (&dev->channels[i])->stream_path);
		else
			ret = AM_FAILURE;
	}
	
	if (g_dsc_sec_mode == DSC_MODE_NORMAL)
	{
		if (dev->drv->close)
			dev->drv->close(dev);
	}
	else if (g_dsc_sec_mode == DSC_MODE_SEC)
	{
		ret = AM_DSC_SEC_Close(dev_no);
		if (g_dsc_sec_lib_handle)
			dlclose(g_dsc_sec_lib_handle);
		g_dsc_sec_lib_handle = NULL;
	}
	else
		ret = AM_FAILURE;
	
	pthread_mutex_destroy(&dev->lock);

	/* Reset status */
	dev->openned = AM_FALSE;

	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 设定解扰器设备的输入源
 * \param dev_no 解扰器设备号
 * \param src 输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_SetSource(int dev_no, AM_DSC_Source_t src)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_TRY(dsc_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);
	if (dev->drv->set_source)
		ret = dev->drv->set_source(dev, src);

	pthread_mutex_unlock(&dev->lock);

	/* src: demux0 --> 1<<0
	     src: demux1 --> 1<<1
	     src: demux2 --> 1<<2
	*/
	if (g_dsc_sec_mode == DSC_MODE_SEC)
		AM_DSC_SEC_SetOutput(dev_no, 1<<src);
	
	return ret;
}

/**\brief 设定解扰器设备的输出
 * \param dev_no 解扰器设备号
 * \param src 输出demux
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dsc.h)
 */
AM_ErrorCode_t AM_DSC_SetOutput(int dev_no, int dst)
{
	if (g_dsc_sec_mode == DSC_MODE_SEC)
		AM_DSC_SEC_SetOutput(dev_no, dst);
	return AM_SUCCESS;
}
AM_ErrorCode_t AM_DSC_SetMode(int dev_no, int chan_id, int mode)
{
	AM_DSC_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_DSC_Channel_t *chan;

	AM_TRY(dsc_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	ret = dsc_get_used_chan(dev, chan_id, &chan);

	if (ret == AM_SUCCESS)
	{
		if (dev->drv->set_pid)
			ret = dev->drv->set_mode(dev, chan, mode);
	}

	pthread_mutex_unlock(&dev->lock);

	return 0;
}
