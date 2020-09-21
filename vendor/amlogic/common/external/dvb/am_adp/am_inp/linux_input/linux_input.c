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
 * \brief linux input输入设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-20: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_time.h>
#include <am_mem.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <limits.h>
#include "../am_inp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static function declaration
 ***************************************************************************/
static AM_ErrorCode_t linux_input_open (AM_INP_Device_t *dev, const AM_INP_OpenPara_t *para);
static AM_ErrorCode_t linux_input_close (AM_INP_Device_t *dev);
static AM_ErrorCode_t linux_input_wait (AM_INP_Device_t *dev, struct input_event *event, int timeout);

/****************************************************************************
 * Static data
 ***************************************************************************/

/**\brief Linux input 输入设备驱动*/
const AM_INP_Driver_t linux_input_drv =
{
.open =  linux_input_open,
.close = linux_input_close,
.wait =  linux_input_wait
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t linux_input_open (AM_INP_Device_t *dev, const AM_INP_OpenPara_t *para)
{
	char name[PATH_MAX];
	int fd;
	
	snprintf(name, sizeof(name), "/dev/input/event%d", dev->dev_no);
	
	fd = open(name, O_RDONLY);
	if(fd == -1)
	{
		AM_DEBUG(1, "cannot open device %s (%s)", name, strerror(errno));
		return AM_INP_ERR_CANNOT_OPEN_DEV;
	}
	
	dev->drv_data = (void*)fd;
	return AM_SUCCESS;
}

static AM_ErrorCode_t linux_input_close (AM_INP_Device_t *dev)
{
	int fd = (int)dev->drv_data;
	
	close(fd);
	return AM_SUCCESS;
}

static AM_ErrorCode_t linux_input_wait (AM_INP_Device_t *dev, struct input_event *event, int timeout)
{
	int fd = (int)dev->drv_data;
	struct pollfd pfd;
	AM_Bool_t ok=AM_FALSE;
	int ret, readlen;
	
	pfd.fd = fd;
	pfd.events = POLLIN;
	
	ret = poll(&pfd, 1, timeout);
	if(ret!=1) return AM_INP_ERR_TIMEOUT;

	readlen = read(fd, event, sizeof(struct input_event));
	if(readlen!=sizeof(struct input_event))
	{
		AM_DEBUG(1, "read input device %d failed (ret %d)", dev->dev_no, readlen);
		return AM_INP_ERR_READ_FAILED;
	}
	
	return AM_SUCCESS;
}

