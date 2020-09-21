/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Linux DVB demux 驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-21: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "am_vbi_internal.h"
#include "am_vbi.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <poll.h> 


#if 0
#undef AM_DEBUG
#define AM_DEBUG(level,arg...) printf(arg)
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct
{
	char   dev_name[VBI_DRIVER_LENGTH];
	int    fd[VBI_FILTER_COUNT];
} VbiDmx_t;

#define N_ELEMENTS(array) (sizeof (array) / sizeof ((array)[0]))
/****************************************************************************
 * Static data definitions
 ***************************************************************************/

static AM_ErrorCode_t vbi_open(AM_VBI_Device_t *dev, const AM_VBI_OpenPara_t *para);
static AM_ErrorCode_t vbi_close(AM_VBI_Device_t *dev);
static AM_ErrorCode_t vbi_alloc_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter);
static AM_ErrorCode_t vbi_free_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter);
static AM_ErrorCode_t vbi_enable_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, AM_Bool_t enable);
static AM_ErrorCode_t vbi_set_buf_size(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, int size);
static AM_ErrorCode_t vbi_poll(AM_VBI_Device_t *dev, AM_VBI_FilterMask_t *mask, int timeout);
static AM_ErrorCode_t vbi_read(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, uint8_t *buf, int *size);
static AM_ErrorCode_t vbi_set_source(AM_VBI_Device_t *dev, AM_VBI_Source_t src);


const AM_VBI_Driver_t linux_vbi_drv = {
.open  = vbi_open,
.close = vbi_close,
.alloc_filter = vbi_alloc_filter,
.free_filter  = vbi_free_filter,
.enable_filter  = vbi_enable_filter,
.set_buf_size   = vbi_set_buf_size,
.poll           = vbi_poll,
.read           = vbi_read,
//.set_source     = vbi_set_source
};

static unsigned int buf_size1 = 1024;

/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t vbi_open(AM_VBI_Device_t *dev, const AM_VBI_OpenPara_t *para)
{
	VbiDmx_t *dmx;
	int i;
	AM_DEBUG(1,"vbi_open\n");
	dmx = (VbiDmx_t*)malloc(sizeof(VbiDmx_t));
	if(!dmx)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_VBI_ERR_NO_MEM;
	}
	
	snprintf(dmx->dev_name, sizeof(dmx->dev_name), "/dev/vbi");
	for(i=0; i<VBI_FILTER_COUNT; i++)
		dmx->fd[i] = -1;
	
	dev->drv_data = dmx;
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_close(AM_VBI_Device_t *dev)
{
	VbiDmx_t *dmx = (VbiDmx_t*)dev->drv_data;
	
	free(dmx);
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_alloc_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter)
{
	VbiDmx_t *dmx = (VbiDmx_t*)dev->drv_data;
	int fd;

	fd = open(dmx->dev_name, O_RDWR|O_NONBLOCK);//fd = open(dmx->dev_name, O_RDWR|O_NONBLOCK);
	if(fd==-1)
	{
		AM_DEBUG(1,"cannot open \"%s\" (%s)", dmx->dev_name, strerror(errno));
		return AM_VBI_ERR_CANNOT_OPEN_DEV;
	}else
		AM_DEBUG(1,"\ncan open  fd=%d\n",fd);
	
	dmx->fd[filter->id] = fd;
	
	filter->drv_data = (void*)(long)fd;
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_free_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter)
{
	VbiDmx_t *dmx = (VbiDmx_t*)dev->drv_data;
	int fd = (int)filter->drv_data;

	close(fd);
	dmx->fd[filter->id] = -1;
	
	return AM_SUCCESS;
}



static AM_ErrorCode_t vbi_enable_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, AM_Bool_t enable)
{
	int fd = (int)filter->drv_data;
	int ret;
	
        AM_DEBUG(1,"\n***********vbi_enable_filter***************enable = %d \n",enable);
	if(enable)
		ret = ioctl(fd, VBI_IOC_START, 1);
	else
		ret = ioctl(fd, VBI_IOC_STOP, 1);
	
	if(ret==-1)
	{
		AM_DEBUG(1, "start filter failed (%s)", strerror(errno));
		return AM_VBI_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_set_buf_size(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, int size)
{
	int fd = (int)filter->drv_data;
	int ret;
        int vbi_buf_size = 1024; 

	AM_DEBUG(1,"vbi_set_buf_size fd =%d size = %d\n",fd,size);
	
	ret = ioctl(fd, VBI_IOC_S_BUF_SIZE, &size);
        if(ret==-1)
	{
		AM_DEBUG(1, "set buffer size failed (%s)", strerror(errno));
		return AM_VBI_ERR_SYS;
	}
   
    	int cc_type = 1;
    	ret = ioctl(fd, VBI_IOC_SET_FILTER, &cc_type);//0,1
   	if(ret==-1)
	{
		AM_DEBUG(1, "set buffer size failed (%s)", strerror(errno));
		return AM_VBI_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_poll(AM_VBI_Device_t *dev, AM_VBI_FilterMask_t *mask, int timeout)
{
	VbiDmx_t *dmx = (VbiDmx_t*)dev->drv_data;
	struct pollfd fds[VBI_FILTER_COUNT];
	int fids[VBI_FILTER_COUNT];
	int i, cnt = 0, ret;
	
	for(i=0; i<VBI_FILTER_COUNT; i++)
	{
		if(dmx->fd[i]!=-1)
		{
			fds[cnt].events = POLLIN|POLLERR;
			fds[cnt].fd     = dmx->fd[i];
			fids[cnt] = i;
			cnt++;
		}
	}
	
	if(!cnt)
		return AM_VBI_ERR_TIMEOUT;
	
	ret = poll(fds, cnt, timeout);
	if(ret<=0)
	{
		AM_DEBUG(1,"vbi_poll  *********AM_VBI_ERR_TIMEOUT \n");
		return AM_VBI_ERR_TIMEOUT;
	}
	
	for(i=0; i<cnt; i++)
	{
		if(fds[i].revents&(POLLIN|POLLERR))
		{
			AM_VBI_FILTER_MASK_SET(mask, fids[i]);
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_read(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, uint8_t *buf, int *size)
{
	int fd = (int)filter->drv_data;
	int len = *size;
	int ret;
	struct pollfd pfd;
	
	if(fd==-1)
		return AM_VBI_ERR_NOT_ALLOCATED;
	
#if 0	
	pfd.events = POLLIN|POLLERR;
	pfd.fd     = fd;
	ret = poll(&pfd, 1, 0);
	if(ret<=0){
		AM_DEBUG(1,"vbi_read************AM_VBI_ERR_NO_DATA\n");
                return AM_VBI_ERR_NO_DATA;
	}
#endif

	ret = read(fd, buf, len);
	if(ret<=0)
	{
		if(errno==ETIMEDOUT)
		    return AM_VBI_ERR_TIMEOUT;
		AM_DEBUG(1, "read demux failed (%s) %d\n", strerror(errno), errno);
		return AM_VBI_ERR_SYS;
	}else
		AM_DEBUG(1,"vbi_read************read result = %d\n",ret);
#if 0		
	AM_DEBUG(1,"\n0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x,0x%2x, 0x%2x, 0x%2x \n",
		buf[0], buf[1], buf[2], buf[3], buf[4],buf[5], buf[6], buf[7]);
#endif
	*size = ret;
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_set_source(AM_VBI_Device_t *dev, AM_VBI_Source_t src)
{
	char buf[32];
	char *cmd;
	
	switch(src)
	{
		case DUAL_FD1:
			cmd = "/dev/vbi";
		break;
		case DUAL_FD2:
			cmd = "/dev/vbi1";
		break;	
	}
	
return AM_VBI_ERR_CANNOT_OPEN_DEV;
}

