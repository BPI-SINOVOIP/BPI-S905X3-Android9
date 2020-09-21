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
 * \brief Linux DVB dvr 驱动
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-12-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 2

#include <am_debug.h>
#include <am_mem.h>
#include "../am_dvr_internal.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/dmx.h>
#include <am_misc.h>

/****************************************************************************
 * Type definitions
 ***************************************************************************/


/****************************************************************************
 * Static data definitions
 ***************************************************************************/

static AM_ErrorCode_t dvb_open(AM_DVR_Device_t *dev, const AM_DVR_OpenPara_t *para);
static AM_ErrorCode_t dvb_close(AM_DVR_Device_t *dev);
static AM_ErrorCode_t dvb_set_buf_size(AM_DVR_Device_t *dev, int size);
static AM_ErrorCode_t dvb_poll(AM_DVR_Device_t *dev, int timeout);
static AM_ErrorCode_t dvb_read(AM_DVR_Device_t *dev, uint8_t *buf, int *size);
static AM_ErrorCode_t dvb_set_source(AM_DVR_Device_t *dev, AM_DVR_Source_t src);

const AM_DVR_Driver_t linux_dvb_dvr_drv = {
.open  = dvb_open,
.close = dvb_close,
.set_buf_size   = dvb_set_buf_size,
.poll           = dvb_poll,
.read           = dvb_read,
.set_source = dvb_set_source,
};


/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t dvb_open(AM_DVR_Device_t *dev, const AM_DVR_OpenPara_t *para)
{
	char dev_name[32];
	int fd;
	
	UNUSED(para);

	snprintf(dev_name, sizeof(dev_name), "/dev/dvb0.dvr%d", dev->dev_no);
	fd = open(dev_name, O_RDONLY);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open \"%s\" (%s)", dev_name, strerror(errno));
		return AM_DVR_ERR_CANNOT_OPEN_DEV;
	}
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK, 0);
	dev->drv_data = (void*)(long)fd;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_close(AM_DVR_Device_t *dev)
{
	int fd = (long)dev->drv_data;
	
	close(fd);
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_buf_size(AM_DVR_Device_t *dev, int size)
{
	int fd = (long)dev->drv_data;
	int ret;
	
	ret = ioctl(fd, DMX_SET_BUFFER_SIZE, size);
	if(ret==-1)
	{
		AM_DEBUG(1, "set buffer size failed (%s)", strerror(errno));
		return AM_DVR_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_poll(AM_DVR_Device_t *dev, int timeout)
{
	int fd = (long)dev->drv_data;
	struct pollfd fds;
	int ret;
	
	fds.events = POLLIN|POLLERR;
	fds.fd     = fd;

	ret = poll(&fds, 1, timeout);
	if(ret<=0)
	{
		return AM_DVR_ERR_TIMEOUT;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_read(AM_DVR_Device_t *dev, uint8_t *buf, int *size)
{
	int fd = (long)dev->drv_data;
	int len = *size;
	int ret;
	
	if(fd==-1)
		return AM_DVR_ERR_NOT_ALLOCATED;
	
	ret = read(fd, buf, len);
	if(ret<=0)
	{
		if(errno==ETIMEDOUT)
			return AM_DVR_ERR_TIMEOUT;
		AM_DEBUG(1, "read dvr failed (%s) %d", strerror(errno), errno);
		return AM_DVR_ERR_SYS;
	}
	
	*size = ret;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_set_source(AM_DVR_Device_t *dev, AM_DVR_Source_t src)
{
	char buf[64];
	char *cmd;
	
	snprintf(buf, sizeof(buf), "/sys/class/stb/asyncfifo%d_source", src);
	
	switch(dev->dev_no)
	{
		case 0:
			cmd = "dmx0";
		break;
		case 1:
			cmd = "dmx1";
		break;
#if defined(CHIP_8226M) || defined(CHIP_8626X)
		case 2:
			cmd = "dmx2";
		break;
#endif
		default:
			AM_DEBUG(1, "do not support demux no %d", src);
		return -1;
	}
	
	return AM_FileEcho(buf, cmd);
}
	
