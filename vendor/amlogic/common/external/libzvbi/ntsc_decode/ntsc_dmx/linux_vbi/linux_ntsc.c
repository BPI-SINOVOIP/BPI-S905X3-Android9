/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Linux DVB demux 驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-21: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include "am_vbi_internal.h"
#include "vbi_dmx.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <poll.h> 


/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct
{
	char   dev_name[DMX_DRIVER_LENGTH];
	int    fd[DMX_FILTER_COUNT];
} VbiDmx_t;

#define N_ELEMENTS(array) (sizeof (array) / sizeof ((array)[0]))
/****************************************************************************
 * Static data definitions
 ***************************************************************************/

static AM_ErrorCode_t vbi_open(AM_VBI_Device_t *dev, const AM_VBI_DMX_OpenPara_t *para);
static AM_ErrorCode_t vbi_close(AM_VBI_Device_t *dev);
static AM_ErrorCode_t vbi_alloc_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter);
static AM_ErrorCode_t vbi_free_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter);
static AM_ErrorCode_t vbi_enable_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, vbi_bool enable);
static AM_ErrorCode_t vbi_set_buf_size(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, int size);
static AM_ErrorCode_t vbi_poll(AM_VBI_Device_t *dev, AM_VBI_FilterMask_t *mask, int timeout);
static AM_ErrorCode_t vbi_read(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, uint8_t *buf, int *size);
static AM_ErrorCode_t vbi_set_source(AM_VBI_Device_t *dev, AM_VBI_DMX_Source_t src);


	
	

const AM_VBI_Driver_t linux_vbi_drv = {
.open  = vbi_open,
.close = vbi_close,
.alloc_filter = vbi_alloc_filter,
.free_filter  = vbi_free_filter,
.enable_filter  = vbi_enable_filter,
.set_buf_size   = vbi_set_buf_size,
.poll           = vbi_poll,
.read           = vbi_read,
.set_source     = vbi_set_source
};


/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t vbi_open(AM_VBI_Device_t *dev, const AM_VBI_DMX_OpenPara_t *para)
{
	VbiDmx_t *dmx;
	int i;
	AM_DEBUG("vbi_open\n");
	dmx = (VbiDmx_t*)malloc(sizeof(VbiDmx_t));
	if(!dmx)
	{
		AM_DEBUG( "not enough memory");
		return AM_VBI_DMX_ERR_NO_MEM;
	}
	
	//snprintf(dmx->dev_name, sizeof(dmx->dev_name), "/dev/vbi", dev->dev_no);
	snprintf(dmx->dev_name, sizeof(dmx->dev_name), "/dev/vbi");
	AM_DEBUG("************dmx->dev_name= %s\n",dmx->dev_name);
	for(i=0; i<DMX_FILTER_COUNT; i++)
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
static unsigned int buf_size1 = 1024;
static AM_ErrorCode_t vbi_alloc_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter)
{
	AM_DEBUG("************************-----vbi_alloc_filter*********************\n");
	VbiDmx_t *dmx = (VbiDmx_t*)dev->drv_data;
	int fd;

	fd = open(dmx->dev_name, O_RDWR|O_NONBLOCK);//fd = open(dmx->dev_name, O_RDWR|O_NONBLOCK);
	if(fd==-1)
	{
		AM_DEBUG("cannot open \"%s\" (%s)", dmx->dev_name, strerror(errno));
		return AM_VBI_DMX_ERR_CANNOT_OPEN_DEV;
	}else
		AM_DEBUG("can open  fd=%d\n",fd);
	
	dmx->fd[filter->id] = fd;
	
	
	
//***************************************************
	
/*
	
	int ret = ioctl(fd, VBI_IOC_S_BUF_SIZE, 20000);
    if (ret < 0) {
        AM_DEBUG("set afe_fd1 ioctl VBI_IOC_S_BUF_SIZE Error!!! \n");
        return 0;
    }
    //AM_DEBUG("set afe_fd1 ioctl VBI_IOC_S_BUF_SIZE   \n");
    int cc_type = 1;
    ret = ioctl(fd, VBI_IOC_SET_FILTER, &cc_type);//0,1
    if (ret < 0) {
        AM_DEBUG("set afe_fd1 ioctl VBI_IOC_SET_FILTER Error!!! \n");
        return 0;
    }
    //AM_DEBUG("set afe_fd1 ioctl VBI_IOC_SET_FILTER  \n");

    ret = ioctl(fd, VBI_IOC_START);
    if (ret < 0) {
        AM_DEBUG("set afe_fd1 ioctl VBI_IOC_START Error!!! \n");
        return 0;
    }
	
*/
//***************************************************
	filter->drv_data = (void*)(long)fd;
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_free_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter)
{
	VbiDmx_t *dmx = (VbiDmx_t*)dev->drv_data;
	int fd = (long)filter->drv_data;

	close(fd);
	dmx->fd[filter->id] = -1;
	
	return AM_SUCCESS;
}



static AM_ErrorCode_t vbi_enable_filter(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, vbi_bool enable)
{
	AM_DEBUG("**************************vbi_enable_filter*******************\n");
	int fd = (long)filter->drv_data;
	int ret;
	AM_DEBUG("***********vbi_enable_filter***************enable = %d \n",enable);
	if(enable)
		ret = ioctl(fd, VBI_IOC_START, 1);
	else
		ret = ioctl(fd, VBI_IOC_STOP, 1);
	
	if(ret==-1)
	{
		AM_DEBUG( "start filter failed (%s)", strerror(errno));
		return AM_VBI_DMX_ERR_SYS;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_set_buf_size(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, int size)
{
	int fd = (long)filter->drv_data;
	int ret;
	AM_DEBUG("vbi_set_buf_size fd =%d size = %d\n",fd,size);
	
	
	
	
	//ret = ioctl(fd, VBI_IOC_S_BUF_SIZE, size);
	//if(ret==-1)
	//{
	//	AM_DEBUG( "set buffer size failed (%s)", strerror(errno));
	//	return AM_VBI_DMX_ERR_SYS;
	//}
	//**************************temp****************************///
	
	ret = ioctl(fd, VBI_IOC_S_BUF_SIZE, size);
    if(ret==-1)
	{
		AM_DEBUG( "set buffer size failed (%s)", strerror(errno));
		return AM_VBI_DMX_ERR_SYS;
	}
   
    int cc_type = 1;
    ret = ioctl(fd, VBI_IOC_SET_FILTER, &cc_type);//0,1
   if(ret==-1)
	{
		AM_DEBUG( "set buffer size failed (%s)", strerror(errno));
		return AM_VBI_DMX_ERR_SYS;
	}
	
	//**************************finish****************************///
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_poll(AM_VBI_Device_t *dev, AM_VBI_FilterMask_t *mask, int timeout)
{
	AM_DEBUG("vbi_poll \n");
	VbiDmx_t *dmx = (VbiDmx_t*)dev->drv_data;
	struct pollfd fds[DMX_FILTER_COUNT];
	int fids[DMX_FILTER_COUNT];
	int i, cnt = 0, ret;
	AM_DEBUG("***********************************************\n");
	AM_DEBUG("vbi_poll  ***********N_ELEMENTS(dmx->fd) =%d \n",N_ELEMENTS(dmx->fd));
	for(i=0; i<DMX_FILTER_COUNT; i++)
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
		return AM_VBI_DMX_ERR_TIMEOUT;
	
	AM_DEBUG("vbi_poll  *********poll cnt = %d fds[0].fd =%d \n",cnt,fds[0].fd);
	ret = poll(fds, cnt, timeout);
	if(ret<=0)
	{
		AM_DEBUG("vbi_poll  *********AM_VBI_DMX_ERR_TIMEOUT \n");
		return AM_VBI_DMX_ERR_TIMEOUT;
	}
	
	for(i=0; i<cnt; i++)
	{
		if(fds[i].revents&(POLLIN|POLLERR))
		{
			AM_DMX_FILTER_MASK_SET(mask, fids[i]);
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_read(AM_VBI_Device_t *dev, AM_VBI_Filter_t *filter, uint8_t *buf, int *size)
{
	int fd = (long)filter->drv_data;
	int len = *size;
	int ret;
	struct pollfd pfd;
	
	if(fd==-1)
		return AM_VBI_DMX_ERR_NOT_ALLOCATED;
	
	pfd.events = POLLIN|POLLERR;
	pfd.fd     = fd;
	AM_DEBUG("vbi_read******************pfd.fd = %d \n",pfd.fd);
	ret = poll(&pfd, 1, 0);
	if(ret<=0){
		return AM_VBI_DMX_ERR_NO_DATA;
		AM_DEBUG( "vbi_read************AM_VBI_DMX_ERR_NO_DATA\n");
	}

	ret = read(fd, buf, len);
	if(ret<=0)
	{
		if(errno==ETIMEDOUT)
			return AM_VBI_DMX_ERR_TIMEOUT;
		AM_DEBUG( "read demux failed (%s) %d\n", strerror(errno), errno);
		return AM_VBI_DMX_ERR_SYS;
	}else
		AM_DEBUG("vbi_read************read result = %d\n",ret);
		
	AM_DEBUG("buf[0] =%d, buf[1] =%d, buf[2] =%d, buf[3] =%d, buf[4] =%d,buf[5] =%d, buf[6] =%d, buf[7] =%d \n",
						buf[0], buf[1], buf[2], buf[3], buf[4],buf[5], buf[6], buf[7]);
	*size = ret;
	return AM_SUCCESS;
}

static AM_ErrorCode_t vbi_set_source(AM_VBI_Device_t *dev, AM_VBI_DMX_Source_t src)
{
	char buf[32];
	char *cmd;
	
	snprintf(buf, sizeof(buf), "/sys/class/stb/demux%d_source", dev->dev_no);
	
	switch(src)
	{
		case DUAL_FD1:
			cmd = "/dev/vbi";
		break;
		case DUAL_FD2:
			cmd = "/dev/vbi1";
		break;	
	}
	
return AM_VBI_DMX_ERR_CANNOT_OPEN_DEV;
}

