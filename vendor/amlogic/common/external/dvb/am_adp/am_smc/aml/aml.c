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
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-05: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <limits.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include "../am_smc_internal.h"
#include <sys/ioctl.h>
#include <amsmc.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static functions declaration
 ***************************************************************************/
 
static AM_ErrorCode_t aml_open (AM_SMC_Device_t *dev, const AM_SMC_OpenPara_t *para);
static AM_ErrorCode_t aml_close (AM_SMC_Device_t *dev);
static AM_ErrorCode_t aml_get_status (AM_SMC_Device_t *dev, AM_SMC_CardStatus_t *status);
static AM_ErrorCode_t aml_reset (AM_SMC_Device_t *dev, uint8_t *atr, int *len);
static AM_ErrorCode_t aml_read (AM_SMC_Device_t *dev, uint8_t *data, int *len, int timeout);
static AM_ErrorCode_t aml_write (AM_SMC_Device_t *dev, const uint8_t *data, int *len, int timeout);
static AM_ErrorCode_t aml_get_param (AM_SMC_Device_t *dev, AM_SMC_Param_t *para);
static AM_ErrorCode_t aml_set_param (AM_SMC_Device_t *dev, const AM_SMC_Param_t *para);
static AM_ErrorCode_t aml_active (AM_SMC_Device_t *dev);
static AM_ErrorCode_t aml_deactive (AM_SMC_Device_t *dev);

/**\brief 模拟智能卡设备驱动*/
const AM_SMC_Driver_t aml_smc_drv =
{
.open  = aml_open,
.close = aml_close,
.get_status =  aml_get_status,
.reset = aml_reset,
.read  = aml_read,
.write = aml_write,
.get_param = aml_get_param,
.set_param = aml_set_param,
.active = aml_active,
.deactive = aml_deactive
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

AM_ErrorCode_t aml_open (AM_SMC_Device_t *dev, const AM_SMC_OpenPara_t *para)
{
	char name[PATH_MAX];
	int fd;

	UNUSED(para);

	snprintf(name, sizeof(name), "/dev/smc%d", dev->dev_no);
	fd = open(name, O_RDWR);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open device \"%s\"", name);
		return AM_SMC_ERR_CANNOT_OPEN_DEV;
	}
	
	dev->drv_data = (void*)(long)fd;
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_close (AM_SMC_Device_t *dev)
{
	int fd = (long)dev->drv_data;
	
	close(fd);
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_get_status (AM_SMC_Device_t *dev, AM_SMC_CardStatus_t *status)
{
	int fd = (long)dev->drv_data;
	int ds;
	
	if(ioctl(fd, AMSMC_IOC_GET_STATUS, &ds))
	{
		AM_DEBUG(1, "get card status failed \"%s\"", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	*status = ds ? AM_SMC_CARD_IN : AM_SMC_CARD_OUT;
	
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_reset (AM_SMC_Device_t *dev, uint8_t *atr, int *len)
{
	int fd = (long)dev->drv_data;
	struct am_smc_atr abuf;
	
	if(ioctl(fd, AMSMC_IOC_RESET, &abuf))
	{
		AM_DEBUG(1, "reset the card failed \"%s\"", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	memcpy(atr, abuf.atr, abuf.atr_len);
	*len = abuf.atr_len;
	
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_read (AM_SMC_Device_t *dev, uint8_t *data, int *len, int timeout)
{
	struct pollfd pfd;
	int fd = (long)dev->drv_data;
	int ret;
	
	pfd.fd = fd;
	pfd.events = POLLIN;
	
	ret = poll(&pfd, 1, timeout);
	if(ret!=1)
	{
		return AM_SMC_ERR_TIMEOUT;
	}
	
	ret = read(fd, data, *len);
	if(ret<0)
	{
		AM_DEBUG(1, "card read error %s", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	*len = ret;
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_write (AM_SMC_Device_t *dev, const uint8_t *data, int *len, int timeout)
{
	struct pollfd pfd;
	int fd = (long)dev->drv_data;
	int ret;
	
	pfd.fd = fd;
	pfd.events = POLLOUT;
	
	ret = poll(&pfd, 1, timeout);
	if(ret!=1)
	{
		return AM_SMC_ERR_TIMEOUT;
	}
	
	ret = write(fd, data, *len);
	if(ret<0)
	{
		AM_DEBUG(1, "card write error %s", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	*len = ret;
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_get_param (AM_SMC_Device_t *dev, AM_SMC_Param_t *para)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, AMSMC_IOC_GET_PARAM, para))
	{
		AM_DEBUG(1, "get card params failed \"%s\"", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_set_param (AM_SMC_Device_t *dev, const AM_SMC_Param_t *para)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, AMSMC_IOC_SET_PARAM, para))
	{
		AM_DEBUG(1, "set card params failed \"%s\"", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_active (AM_SMC_Device_t *dev)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, AMSMC_IOC_ACTIVE, 0))
	{
		AM_DEBUG(1, "active card failed \"%s\"", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	return AM_SUCCESS;
}

AM_ErrorCode_t aml_deactive (AM_SMC_Device_t *dev)
{
	int fd = (long)dev->drv_data;
	
	if(ioctl(fd, AMSMC_IOC_DEACTIVE, 0))
	{
		AM_DEBUG(1, "deactive card failed \"%s\"", strerror(errno));
		return AM_SMC_ERR_IO;
	}
	
	return AM_SUCCESS;
}

