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
 * \brief linux终端输入设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-01: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include "../am_inp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static function declaration
 ***************************************************************************/
static AM_ErrorCode_t tty_open (AM_INP_Device_t *dev, const AM_INP_OpenPara_t *para);
static AM_ErrorCode_t tty_close (AM_INP_Device_t *dev);
static AM_ErrorCode_t tty_wait (AM_INP_Device_t *dev, struct input_event *event, int timeout);

/****************************************************************************
 * Static data
 ***************************************************************************/

static struct termios tty_old_opt;

/**\brief Linux 终端输入设备驱动*/
const AM_INP_Driver_t tty_drv =
{
.open =  tty_open,
.close = tty_close,
.wait =  tty_wait
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t tty_open (AM_INP_Device_t *dev, const AM_INP_OpenPara_t *para)
{
	struct termios opt;
	int fd = 0;
	
	tcgetattr(fd, &opt);
	tcflush(fd, TCIOFLUSH);
	
	tty_old_opt = opt;
	
	opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	
	opt.c_cc[VMIN] = 1;
	opt.c_cc[VTIME] = 0;
	
	tcsetattr(fd, TCSANOW, &opt);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t tty_close (AM_INP_Device_t *dev)
{
	int fd = 0;
	
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &tty_old_opt);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t tty_wait (AM_INP_Device_t *dev, struct input_event *event, int timeout)
{
	int fd = 0, ret;
	struct pollfd fds;
	
	fds.fd = fd;
	fds.events = POLLIN;
	
	ret = poll(&fds, 1, timeout);
	if(ret!=1)
	{
		return AM_INP_ERR_TIMEOUT;
	}
	else
	{
		char ch;
		
		if(read(fd, &ch, 1)!=1)
		{
			AM_DEBUG(1, "read tty failed (errno=%d)", errno);
			return AM_INP_ERR_READ_FAILED;
		}
		
		gettimeofday(&event->time, NULL);
		event->type = EV_KEY;
		event->code = ch;
		event->value = 1;
		
		return AM_SUCCESS;
	}
}

