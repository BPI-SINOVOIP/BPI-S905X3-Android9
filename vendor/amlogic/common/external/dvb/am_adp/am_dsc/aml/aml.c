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
 * \brief AMLogic 解扰器驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "../am_dsc_internal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <amdsc.h>
#include <string.h>
#include <errno.h>
/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/ca.h>
#include "am_misc.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DEV_NAME "/dev/dvb0.ca"

/****************************************************************************
 * Static data
 ***************************************************************************/

static AM_ErrorCode_t aml_open (AM_DSC_Device_t *dev, const AM_DSC_OpenPara_t *para);
static AM_ErrorCode_t aml_alloc_chan (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan);
static AM_ErrorCode_t aml_free_chan (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan);
static AM_ErrorCode_t aml_set_pid (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan, uint16_t pid);
static AM_ErrorCode_t aml_set_key (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan, AM_DSC_KeyType_t type, const uint8_t *key);
static AM_ErrorCode_t aml_set_source (AM_DSC_Device_t *dev, AM_DSC_Source_t src);
static AM_ErrorCode_t aml_set_mode (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan,int mode);
static AM_ErrorCode_t aml_close (AM_DSC_Device_t *dev);

AM_DSC_Driver_t aml_dsc_drv =
{
.open        = aml_open,
.alloc_chan  = aml_alloc_chan,
.free_chan   = aml_free_chan,
.set_pid     = aml_set_pid,
.set_key     = aml_set_key,
.set_source  = aml_set_source,
.set_mode	 = aml_set_mode,
.close       = aml_close
};

/****************************************************************************
 * API functions
 ***************************************************************************/

static AM_ErrorCode_t aml_open (AM_DSC_Device_t *dev, const AM_DSC_OpenPara_t *para)
{
	int fd;
	char buf[32];

	snprintf(buf, sizeof(buf), DEV_NAME"%d", dev->dev_no);
	fd = open(buf, O_RDWR);

	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open \"%s\" (%d:%s)", DEV_NAME, errno, strerror(errno));
		return AM_DSC_ERR_CANNOT_OPEN_DEV;
	}
	dev->drv_data = (void*)(long)fd;
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_alloc_chan (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan)
{
	// int fd;
	// int ret = 0;
	// ca_pid_t pi;

	// fd = (long)dev->drv_data;
	// pi.index = chan->id;
	// pi.pid = chan->pid;

	// ret = ioctl(fd, CA_SET_PID, &pi);
	// if(ret == 0)
		return AM_SUCCESS;
}

static AM_ErrorCode_t aml_free_chan (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan)
{
	int fd;
	int ret = 0;
	ca_pid_t pi;

	fd = (long)dev->drv_data;
	// pi.index = chan->id;
	// pi.pid = 0x1fff;
	/*index == -1 pid is chan set pid ,will free chan*/
	pi.index = -1;
	pi.pid = chan->pid;
	ret = ioctl(fd, CA_SET_PID, &pi);
	if(ret == 0)
		return AM_SUCCESS;
	else
		return AM_FAILURE;
}

static AM_ErrorCode_t aml_set_pid (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan, uint16_t pid)
{
	ca_pid_t pi;
	int fd = (long)dev->drv_data;

	pi.pid = pid;
	pi.index = chan->id;
	if(ioctl(fd, CA_SET_PID, &pi)==-1)
	{
		AM_DEBUG(1, "set pid failed \"%s\"", strerror(errno));
		return AM_DSC_ERR_SYS;
	}
	else
	{
		AM_DEBUG(2, "SET DSC %d PID %d", chan->id, pid);
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_key (
	AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan, AM_DSC_KeyType_t type, const uint8_t *key)
{
	int key_len, fd;
	unsigned int alg_type, kl_flag;
	/* For dvbcsa use */
	struct am_dsc_key dkey;
	struct ca_descr_ex aes_param;
	/* For aes use */
	ca_descr_t dvbcsa_param;

	fd = (long)dev->drv_data;
	alg_type = type &(~AM_DSC_KEY_FROM_KL);
	kl_flag = type & AM_DSC_KEY_FROM_KL;

	/* DVBCSA */
	if(alg_type == AM_DSC_KEY_TYPE_ODD || alg_type == AM_DSC_KEY_TYPE_EVEN)
	{
		dvbcsa_param.parity = (type == AM_DSC_KEY_TYPE_ODD) ? 1 : 0;
		dvbcsa_param.index = chan->id;
		memcpy(dvbcsa_param.cw, key, 8);
		if(ioctl(fd, CA_SET_DESCR, &dvbcsa_param)==-1)
			goto ERR;
		else
			goto SUCCESS;
	}
	else if(alg_type < AM_DSC_KEY_FROM_KL)/* AES/DES/SM4 */
	{
		memset(&aes_param,0,sizeof(struct ca_descr_ex));
		aes_param.index = chan->id;
		aes_param.flags = (kl_flag) ? CA_CW_FROM_KL : 0;
		/* Need to ensure type in am_dsc.h equal to the one in ca.h */
		aes_param.type = alg_type;
		aes_param.mode = chan->mode;
		memcpy(aes_param.cw, key, 16);
		AM_DEBUG("%s mode:%d\n",__FUNCTION__,aes_param.mode);
		if (ioctl(fd, CA_SET_DESCR_EX, &aes_param) == -1)
			goto ERR;
		else
			goto SUCCESS;
	}
	else
		goto ERR;

ERR:
	AM_DEBUG(1, "set key failed \"%s\"", strerror(errno));
	return AM_DSC_ERR_SYS;
SUCCESS:
	AM_DEBUG(2, "SET DSC %d KEY %d, %02x %02x %02x %02x %02x %02x %02x %02x",
		chan->id, type, key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7]);
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_source (AM_DSC_Device_t *dev, AM_DSC_Source_t src)
{
	char *cmd;
	char buf[64];

	switch(src)
	{
		case AM_DSC_SRC_DMX0:
			cmd = "dmx0";
		break;
		case AM_DSC_SRC_DMX1:
			cmd = "dmx1";
		break;
		case AM_DSC_SRC_DMX2:
			cmd = "dmx2";
		break;
		case AM_DSC_SRC_BYPASS:
			cmd = "bypass";
		break;

		default:
			return AM_DSC_ERR_NOT_SUPPORTED;
		break;
	}

	snprintf(buf, sizeof(buf), "/sys/class/stb/dsc%d_source", dev->dev_no);
	return AM_FileEcho(buf, cmd);
}
static AM_ErrorCode_t aml_set_mode (AM_DSC_Device_t *dev, AM_DSC_Channel_t *chan,int mode)
{
	chan->mode = mode;
	return AM_SUCCESS;
}
static AM_ErrorCode_t aml_close (AM_DSC_Device_t *dev)
{
	int fd;
	UNUSED(dev);
	fd = (long)dev->drv_data;
	if (fd != -1) {
		close(fd);
	}
	return AM_SUCCESS;
}

