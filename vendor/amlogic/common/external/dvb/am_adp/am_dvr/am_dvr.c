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
 * \brief
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 2

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <am_debug.h>
#include "am_dvr_internal.h"
#include <am_dmx.h>
#include <am_fend.h>
#include <am_time.h>
#include <am_mem.h>

#include "../am_adp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#ifdef CHIP_8226H
#define DVR_DEV_COUNT      (2)
#elif defined(CHIP_8226M) || defined(CHIP_8626X)
#define DVR_DEV_COUNT      (3)
#else
#define DVR_DEV_COUNT      (2)
#endif

/****************************************************************************
 * Static data
 ***************************************************************************/
 
#ifdef EMU_DVR
extern const AM_DVR_Driver_t emu_dvr_drv;
#else
extern const AM_DVR_Driver_t linux_dvb_dvr_drv;
#endif

static ssize_t dvr_read(int dvr_no, void *buf, size_t size);
static ssize_t dvr_write(int dvr_no, void *buf, size_t size);
static loff_t dvr_seek(int dev_no, loff_t offset, int whence);

static AM_DVR_Device_t dvr_devices[DVR_DEV_COUNT] =
{
#ifdef EMU_DVR
{
.drv = &emu_dvr_drv,
},
{
.drv = &emu_dvr_drv,
}
#else
{
.drv = &linux_dvb_dvr_drv,
},
{
.drv = &linux_dvb_dvr_drv,
}
#if defined(CHIP_8226M) || defined(CHIP_8626X)
,
{
.drv = &linux_dvb_dvr_drv,
}
#endif
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t dvr_get_dev(int dev_no, AM_DVR_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=DVR_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid dvr device number %d, must in(%d~%d)", dev_no, 0, DVR_DEV_COUNT-1);
		return AM_DVR_ERR_INVALID_DEV_NO;
	}
	
	*dev = &dvr_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t dvr_get_openned_dev(int dev_no, AM_DVR_Device_t **dev)
{
	AM_TRY(dvr_get_dev(dev_no, dev));
	
	if(!(*dev)->open_cnt)
	{
		AM_DEBUG(1, "dvr device %d has not been openned", dev_no);
		return AM_DVR_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief 添加一个流*/
static AM_ErrorCode_t dvr_add_stream(AM_DVR_Device_t *dev, uint16_t pid)
{
	int i;
	
	if (dev->stream_cnt >= AM_DVR_MAX_PID_COUNT)
	{
		AM_DEBUG(1, "PID count overflow, Max support %d", AM_DVR_MAX_PID_COUNT);
		return AM_DVR_ERR_TOO_MANY_STREAMS;
	}

	i = dev->stream_cnt;

	dev->streams[i].pid = pid;
	dev->streams[i].fid = -1;
	dev->stream_cnt++;
	
	return AM_SUCCESS;
}


/**\brief 启动所有流的录制*/
static AM_ErrorCode_t dvr_start_all_streams(AM_DVR_Device_t *dev)
{
	int i;
	struct dmx_pes_filter_params pparam;

	AM_DEBUG(1, "Start DVR%d recording, pid count %d", dev->dev_no, dev->stream_cnt);
	for(i=0; i<dev->stream_cnt; i++)
	{
		if (dev->streams[i].fid == -1)
		{
			if (AM_DMX_AllocateFilter(dev->dmx_no, &dev->streams[i].fid) == AM_SUCCESS)
			{
				memset(&pparam, 0, sizeof(pparam));
				pparam.pid = dev->streams[i].pid;
				pparam.input = DMX_IN_FRONTEND;
				pparam.output = DMX_OUT_TS_TAP;
				pparam.pes_type = DMX_PES_OTHER;
				AM_DMX_SetPesFilter(dev->dmx_no, dev->streams[i].fid, &pparam);
				AM_DMX_StartFilter(dev->dmx_no, dev->streams[i].fid);
				AM_DEBUG(1, "Stream(pid=%d) start recording...", dev->streams[i].pid);
			}
			else
			{
				dev->streams[i].fid = -1;
				AM_DEBUG(1, "Cannot alloc filter, stream(pid=%d) will not record.", dev->streams[i].pid);
			}
		}
	}

	return AM_SUCCESS;
}

/**\brief 停止所有流的录制*/
static AM_ErrorCode_t dvr_stop_all_streams(AM_DVR_Device_t *dev)
{
	int i;
	
	for(i=0; i<dev->stream_cnt; i++)
	{
		if (dev->streams[i].fid != -1)
		{
			AM_DEBUG(1, "Stop stream(pid=%d)...", dev->streams[i].pid);
			AM_DMX_StopFilter(dev->dmx_no, dev->streams[i].fid);
			AM_DMX_FreeFilter(dev->dmx_no, dev->streams[i].fid);
			dev->streams[i].fid = -1;
		}
	}

	dev->stream_cnt = 0;

	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/
 
/**\brief 打开DVR设备
 * \param dev_no DVR设备号
 * \param[in] para DVR设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_Open(int dev_no, const AM_DVR_OpenPara_t *para)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	assert(para);
	
	AM_TRY(dvr_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->open_cnt)
	{
		AM_DEBUG(1, "dvr device %d has already been openned", dev_no);
		dev->open_cnt++;
		goto final;
	}

	dev->dev_no = dev_no;
	/*DVR与使用的DMX设备号一致*/
	dev->dmx_no = dev_no;

	for(i=0; i<AM_DVR_MAX_PID_COUNT; i++)
	{
		dev->streams[i].fid = -1;
	}
	
	if(dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
	}
	
	if(ret==AM_SUCCESS)
	{
		pthread_mutex_init(&dev->lock, NULL);
		dev->open_cnt++;
		dev->record = AM_FALSE;
		dev->stream_cnt = 0;
	}
	
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭DVR设备
 * \param dev_no DVR设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_Close(int dev_no)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);

	if(dev->open_cnt > 0){
		/*停止数据源*/
		dvr_stop_all_streams(dev);
			
		if(dev->drv->close)
		{
			dev->drv->close(dev);
		}

		pthread_mutex_destroy(&dev->lock);
		dev->record = AM_FALSE;

		dev->open_cnt--;
	}
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 设置DVR设备缓冲区大小
 * \param dev_no DVR设备号
 * \param size 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_SetBufferSize(int dev_no, int size)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->drv->set_buf_size)
	{
		AM_DEBUG(1, "do not support set_buf_size");
		ret = AM_DVR_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
		ret = dev->drv->set_buf_size(dev, size);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 开始录像
 * \param dev_no DVR设备号
 * \param [in] para 录像参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_StartRecord(int dev_no, const AM_DVR_StartRecPara_t *para)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int pid_cnt;

	assert(para);
	if (para->pid_count <= 0)
	{
		AM_DEBUG(1, "Invalid pid count %d", para->pid_count);
		return AM_DVR_ERR_INVALID_ARG;
	}
	pid_cnt = para->pid_count;
	if (pid_cnt > AM_DVR_MAX_PID_COUNT)
		pid_cnt = AM_DVR_MAX_PID_COUNT;
		
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	if (dev->record)
	{
		AM_DEBUG(1, "dvr device %d already recording.", dev_no);
		ret = AM_DVR_ERR_BUSY;
	}
	else
	{
		int i;

		for (i=0; i<pid_cnt; i++)
		{
			dvr_add_stream(dev, (uint16_t)para->pids[i]);
		}
		
		/*启动数据*/
		dvr_start_all_streams(dev);
		dev->record = AM_TRUE;
		dev->start_para = *para;
	}
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 停止录像
 * \param dev_no DVR设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_StopRecord(int dev_no)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	if (dev->record)
	{
		/*停止数据源*/
		dvr_stop_all_streams(dev);
		
		dev->record = AM_FALSE;
	}
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief 从DVR读取录像数据
 * \param dev_no DVR设备号
 * \param	[out] buf 缓冲区
 * \param size	需要读取的数据长度
 * \param timeout 读取超时时间 ms 
 * \return
 *   - 实际读取的字节数
 */
int AM_DVR_Read(int dev_no, uint8_t *buf, int size, int timeout_ms)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret;
	int cnt = -1;

	AM_TRY(dvr_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);
	if(!dev->drv->read)
	{
		AM_DEBUG(1, "do not support read");
	}
	else
	{
		ret = dev->drv->poll(dev, timeout_ms);
		if(ret==AM_SUCCESS)
		{
			cnt = size;
			ret = dev->drv->read(dev, buf, &cnt);
			if (ret != AM_SUCCESS)
				cnt = -1;
		}
	}
	pthread_mutex_unlock(&dev->lock);

	return cnt;
}

/**\brief 设置DVR源
 * \param dev_no DVR设备号
 * \param	src DVR源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_dvr.h)
 */
AM_ErrorCode_t AM_DVR_SetSource(int dev_no, AM_DVR_Source_t src)
{
	AM_DVR_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(dvr_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	if(!dev->drv->set_source)
	{
		AM_DEBUG(1, "do not support set_source");
		ret = AM_DVR_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
	{
		ret = dev->drv->set_source(dev, src);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret==AM_SUCCESS)
	{
		pthread_mutex_lock(&am_gAdpLock);
		dev->src = src;
		pthread_mutex_unlock(&am_gAdpLock);
	}

	return ret;
}

