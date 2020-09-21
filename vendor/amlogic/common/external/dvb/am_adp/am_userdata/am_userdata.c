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
/**\file am_userdata.c
 * \brief user data 驱动模块
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2013-3-13: create the document
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
#include "am_userdata_internal.h"
#include <am_dmx.h>
#include <am_fend.h>
#include <am_time.h>
#include <am_mem.h>
#include <am_cond.h>

#include "../am_adp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define USERDATA_DEV_COUNT      (1)


/****************************************************************************
 * Static data
 ***************************************************************************/
//#define EMU_USERDATA

#ifdef EMU_USERDATA
extern const AM_USERDATA_Driver_t emu_ud_drv;
#else
extern const AM_USERDATA_Driver_t aml_ud_drv;
#endif


static AM_USERDATA_Device_t userdata_devices[USERDATA_DEV_COUNT] =
{
#ifdef EMU_USERDATA
{
.drv = &emu_ud_drv,
},
#else
{
.drv = &aml_ud_drv,
},
#endif
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t userdata_get_dev(int dev_no, AM_USERDATA_Device_t **dev)
{
	if ((dev_no<0) || (dev_no >= USERDATA_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid userdata device number %d, must in(%d~%d)", dev_no, 0, USERDATA_DEV_COUNT-1);
		return AM_USERDATA_ERR_INVALID_DEV_NO;
	}
	
	*dev = &userdata_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t userdata_get_openned_dev(int dev_no, AM_USERDATA_Device_t **dev)
{
	AM_TRY(userdata_get_dev(dev_no, dev));
	
	if (!(*dev)->open_cnt)
	{
		AM_DEBUG(1, "userdata device %d has not been openned", dev_no);
		return AM_USERDATA_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

static void dump_user_data(const uint8_t *buff, int size)
{
	int i;
	char buf[4096];

	if (size > 1024)
		size = 1024;
	for (i=0; i<size; i++)
	{
		sprintf(buf+i*3, "%02x ", buff[i]);
	}

	AM_DEBUG(5, "%s", buf);
}

int userdata_ring_buf_init(AM_USERDATA_RingBuffer_t *ringbuf, size_t len)
{
	ringbuf->pread=ringbuf->pwrite=0;
	ringbuf->data=(uint8_t*)malloc(len);
	if (ringbuf->data == NULL)
		return -1;
	ringbuf->size=len;
	ringbuf->error=0;
	
	pthread_cond_init(&ringbuf->cond, NULL);
	
	return 0;
}

int userdata_ring_buf_deinit(AM_USERDATA_RingBuffer_t *ringbuf)
{
	ringbuf->pread=ringbuf->pwrite=0;
	if (ringbuf->data != NULL)
		free(ringbuf->data);
	ringbuf->size=0;
	ringbuf->error=0;
	
	pthread_cond_destroy(&ringbuf->cond);
	
	return 0;
}

int userdata_ring_buf_empty(AM_USERDATA_RingBuffer_t *ringbuf)
{
	return (ringbuf->pread==ringbuf->pwrite);
}

ssize_t userdata_ring_buf_free(AM_USERDATA_RingBuffer_t *ringbuf)
{
	ssize_t free;

	free = ringbuf->pread - ringbuf->pwrite;
	if (free <= 0)
		free += ringbuf->size;

	return free-1;
}

ssize_t userdata_ring_buf_avail(AM_USERDATA_RingBuffer_t *ringbuf)
{
	ssize_t avail;
	
	avail = ringbuf->pwrite - ringbuf->pread;
	if (avail < 0)
		avail += ringbuf->size;

	return avail;
}

void userdata_ring_buf_read(AM_USERDATA_RingBuffer_t *ringbuf, uint8_t *buf, size_t len)
{
	size_t todo = len;
	size_t split;

	if (ringbuf->data == NULL)
		return;
	
	split = ((ssize_t)(ringbuf->pread + len) > ringbuf->size) ? ringbuf->size - ringbuf->pread : 0;
	if (split > 0) {
		memcpy(buf, ringbuf->data+ringbuf->pread, split);
		buf += split;
		todo -= split;
		ringbuf->pread = 0;
	}
	
	memcpy(buf, ringbuf->data+ringbuf->pread, todo);

	ringbuf->pread = (ringbuf->pread + todo) % ringbuf->size;
}

ssize_t userdata_ring_buf_write(AM_USERDATA_RingBuffer_t *ringbuf, const uint8_t *buf, size_t len)
{
	size_t todo = len;
	size_t split;
	
	if (ringbuf->data == NULL)
		return 0;
	
	split = ((ssize_t)(ringbuf->pwrite + len) > ringbuf->size) ? ringbuf->size - ringbuf->pwrite : 0;

	if (split > 0) {
		memcpy(ringbuf->data+ringbuf->pwrite, buf, split);
		buf += split;
		todo -= split;
		ringbuf->pwrite = 0;
	}
	
	memcpy(ringbuf->data+ringbuf->pwrite, buf, todo);
	
	ringbuf->pwrite = (ringbuf->pwrite + todo) % ringbuf->size;
	
	if (len > 0)
		pthread_cond_signal(&ringbuf->cond);

	return len;
}

static void read_unused_data(AM_USERDATA_RingBuffer_t *ringbuf, size_t len)
{
	uint8_t *buf = (uint8_t*)malloc(len);
	
	if (buf != NULL)
	{
		//AM_DEBUG(1, "read %d bytes unused data", len);
		userdata_ring_buf_read(ringbuf, buf, len);
		free(buf);
	}
}

static int userdata_package_write(AM_USERDATA_Device_t *dev, const uint8_t *buf, size_t size)
{
	int cnt, ret;
	
	pthread_mutex_lock(&dev->lock);
	cnt = userdata_ring_buf_free(&dev->pkg_buf);
	if (cnt < (int)(size+sizeof(cnt)))
	{
		AM_DEBUG(1, "write userdata error: data size to large, %d > %d", size+sizeof(cnt), cnt);
		ret = 0;
	}
	else
	{
		cnt = size;
		userdata_ring_buf_write(&dev->pkg_buf, (uint8_t*)&cnt, sizeof(cnt));
		userdata_ring_buf_write(&dev->pkg_buf, buf, size);
		ret = size;
	}
	pthread_mutex_unlock(&dev->lock);
	
	//AM_DEBUG(3, "write %d bytes\n", ret);
	dump_user_data(buf, size);
	return ret;
}

static int userdata_package_read(AM_USERDATA_Device_t *dev, uint8_t *buf, int size)
{	
	int cnt, ud_cnt;
	
	ud_cnt = 0;
	cnt = userdata_ring_buf_avail(&dev->pkg_buf);
	if (cnt > 4)
	{
		userdata_ring_buf_read(&dev->pkg_buf, (uint8_t*)&ud_cnt, sizeof(ud_cnt));
		cnt = userdata_ring_buf_avail(&dev->pkg_buf);
		if (cnt < ud_cnt)
		{
			/* this case must not happen */
			//AM_DEBUG(0, "read userdata error: expect %d bytes, but only %d bytes avail", ud_cnt, cnt);
			cnt = 0;
			read_unused_data(&dev->pkg_buf, cnt);
		}
		else
		{
			cnt = 0;
			if (ud_cnt > size)
			{
				//AM_DEBUG(0, "read userdata error: source buffer not enough, bufsize %d , datasize %d", size, ud_cnt);
				read_unused_data(&dev->pkg_buf, ud_cnt);
			}
			else if (ud_cnt > 0)
			{
				userdata_ring_buf_read(&dev->pkg_buf, buf, ud_cnt);
				cnt = ud_cnt;
			}
		}
	}
	else
	{
		//AM_DEBUG(0, "read userdata error: count = %d < 4", cnt);
		cnt = 0;
		read_unused_data(&dev->pkg_buf, cnt);
	}
	
	return cnt;
}

static AM_ErrorCode_t userdata_package_poll(AM_USERDATA_Device_t *dev, int timeout)
{
	int rv, ret = AM_SUCCESS, cnt;
	struct timespec rt;
	
	cnt = userdata_ring_buf_avail(&dev->pkg_buf);
	if (cnt <= 0)
	{
		AM_TIME_GetTimeSpecTimeout(timeout, &rt);
		rv = pthread_cond_timedwait(&dev->pkg_buf.cond, &dev->lock, &rt);
		if (rv == ETIMEDOUT)
		{
			//AM_DEBUG(1, "poll userdata timeout, timeout = %d ms", timeout);
			ret = AM_FAILURE;
		}
		else
		{
			cnt = userdata_ring_buf_avail(&dev->pkg_buf);
			if (cnt <= 0)
			{
				AM_DEBUG(1, "poll error, unexpected error");
				ret = AM_FAILURE;
			}
		}
	}
	return ret;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开USERDATA设备
 * \param dev_no USERDATA设备号
 * \param[in] para USERDATA设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_userdata.h)
 */
AM_ErrorCode_t AM_USERDATA_Open(int dev_no, const AM_USERDATA_OpenPara_t *para)
{
	AM_USERDATA_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int i;
	
	assert(para);
	
	AM_TRY(userdata_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if (dev->open_cnt)
	{
		AM_DEBUG(1, "userdata device %d has already been openned", dev_no);
		dev->open_cnt++;
		goto final;
	}

	dev->dev_no = dev_no;

	dev->write_package = userdata_package_write;
	userdata_ring_buf_init(&dev->pkg_buf, USERDATA_BUF_SIZE);
	pthread_mutex_init(&dev->lock, NULL);
	
	if (dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
	}
	
	if (ret == AM_SUCCESS)
	{
		dev->open_cnt++;
	}
	else
	{
		pthread_mutex_destroy(&dev->lock);
	}
	
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭USERDATA设备
 * \param dev_no USERDATA设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_userdata.h)
 */
AM_ErrorCode_t AM_USERDATA_Close(int dev_no)
{
	AM_USERDATA_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(userdata_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);

	if (dev->open_cnt > 0)
	{
		if (dev->open_cnt == 1)
		{				
			if (dev->drv->close)
			{
				dev->drv->close(dev);
			}

			pthread_mutex_destroy(&dev->lock);
			
			userdata_ring_buf_deinit(&dev->pkg_buf);
		}
		dev->open_cnt--;
	}
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 从USERDATA读取数据
 * \param dev_no USERDATA设备号
 * \param	[out] buf 缓冲区
 * \param size	需要读取的数据长度
 * \param timeout 读取超时时间 ms
 * \return
 *   - 实际读取的字节数
 */
int AM_USERDATA_Read(int dev_no, uint8_t *buf, int size, int timeout_ms)
{
	AM_USERDATA_Device_t *dev;
	AM_ErrorCode_t ret;
	int cnt = -1;

	AM_TRY(userdata_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);
	ret = userdata_package_poll(dev, timeout_ms);
	if (ret == AM_SUCCESS)
	{
		cnt = userdata_package_read(dev, buf, size);
	}
	pthread_mutex_unlock(&dev->lock);

	return cnt;
}

AM_ErrorCode_t AM_USERDATA_SetMode(int dev_no, int mode)
{
	AM_USERDATA_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(userdata_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_mode)
		ret = dev->drv->set_mode(dev, mode);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_USERDATA_GetMode(int dev_no, int *mode)
{
	AM_USERDATA_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	if (!mode)
		return AM_USERDATA_ERR_INVALID_ARG;

	AM_TRY(userdata_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->get_mode)
		ret = dev->drv->get_mode(dev, mode);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

