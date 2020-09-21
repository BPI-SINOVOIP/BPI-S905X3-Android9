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
 * \brief Linux V4L2前端设备
 *
 * \author nengwen.chen <nengwen.chen@amlogic.com>
 * \date 2018-04-16: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_misc.h>
#include "../am_fend_internal.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
/*add for config define for linux dvb *.h*/
#include <am_config.h>
#include <linux/dvb/frontend.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <atv_frontend.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * Static functions
 ***************************************************************************/
static AM_ErrorCode_t v4l2_open (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para);
static AM_ErrorCode_t v4l2_set_mode (AM_FEND_Device_t *dev, int mode);
static AM_ErrorCode_t v4l2_set_para (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para);
static AM_ErrorCode_t v4l2_get_para (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para);
static AM_ErrorCode_t v4l2_set_prop (AM_FEND_Device_t *dev, const struct dtv_properties *prop);
static AM_ErrorCode_t v4l2_get_prop (AM_FEND_Device_t *dev, struct dtv_properties *prop);
static AM_ErrorCode_t v4l2_get_status (AM_FEND_Device_t *dev, fe_status_t *status);
static AM_ErrorCode_t v4l2_wait_event (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout);
static AM_ErrorCode_t v4l2_close (AM_FEND_Device_t *dev);
static AM_ErrorCode_t v4l2_get_tune_status(AM_FEND_Device_t *dev, atv_status_t *atv_status);

/****************************************************************************
 * Static data
 ***************************************************************************/
const AM_FEND_Driver_t linux_v4l2_fend_drv =
{
	.open = v4l2_open,
	.set_mode = v4l2_set_mode,
	.get_info = NULL,
	.get_ts   = NULL,
	.set_para = v4l2_set_para,
	.get_para = v4l2_get_para,
	.set_prop = v4l2_set_prop,
	.get_prop = v4l2_get_prop,
	.get_status = v4l2_get_status,
	.get_snr = NULL,
	.get_ber = NULL,
	.get_strength = NULL,
	.wait_event = v4l2_wait_event,
	.set_delay  = NULL,
	.diseqc_reset_overload = NULL,
	.diseqc_send_master_cmd = NULL,
	.diseqc_recv_slave_reply = NULL,
	.diseqc_send_burst = NULL,
	.set_tone = NULL,
	.set_voltage = NULL,
	.enable_high_lnb_voltage = NULL,
	.close = v4l2_close,
	.blindscan_scan = NULL,
	.blindscan_getscanevent = NULL,
	.blindscan_cancel = NULL,
	.fine_tune = NULL,
	.set_cvbs_amp_out = NULL,
	.get_atv_status = v4l2_get_tune_status,
	.set_afc = NULL
};

/****************************************************************************
 * Functions
 ***************************************************************************/
static AM_ErrorCode_t v4l2_open(AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para)
{
	char name[PATH_MAX];
	int fd, ret;

	snprintf(name, sizeof(name), "/dev/v4l2_frontend");

	fd = open(name, O_RDWR);
	if (fd == -1)
	{
		AM_DEBUG(1, "cannot open %s, error:%s", name, strerror(errno));
		return AM_FEND_ERR_CANNOT_OPEN;
	}
	AM_DEBUG(1, "open %s, %d.", name, fd);
	dev->drv_data = (void*) (long) fd;

	if (para->mode != -1)
	{
		ret = v4l2_set_mode(dev, para->mode);
		if (ret != AM_SUCCESS)
		{
			close(fd);
			return ret;
		}
	}
	else
	{
		ret = v4l2_set_mode(dev, 0);
		if (ret != AM_SUCCESS)
		{
			close(fd);
			return ret;
		}
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_set_mode(AM_FEND_Device_t *dev, int mode)
{
	int fd = (long)dev->drv_data;
	int ret;
	int fe_mode = SYS_UNDEFINED;

	AM_DEBUG(1, "set mode %d.\n", mode);

	if (mode == FE_ANALOG)
	{
		fe_mode = 1;
	} else {
		fe_mode = 0;
	}

	if (ioctl(fd, V4L2_SET_MODE, &fe_mode) == -1)
	{
		AM_DEBUG(1, "ioctl V4L2_SET_MODE failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_set_para(AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para)
{
	int fd = (long) dev->drv_data;

	struct v4l2_analog_parameters v4l2_para;

	v4l2_para.frequency = para->frequency;
	v4l2_para.audmode = para->u.analog.audmode;
	v4l2_para.soundsys = para->u.analog.soundsys;
	v4l2_para.std = para->u.analog.std;
	v4l2_para.flag = para->u.analog.flag;
	v4l2_para.afc_range = para->u.analog.afc_range;

	AM_DEBUG(1, "ioctl V4L2_SET_FRONTEND\n");
	if (ioctl(fd, V4L2_SET_FRONTEND, &v4l2_para) == -1)
	{
		AM_DEBUG(1, "ioctl V4L2_SET_FRONTEND failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}
	AM_DEBUG(1, "ioctl V4L2_SET_FRONTEND success \n");

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_get_para(AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para)
{
	int fd = (long) dev->drv_data;

	struct v4l2_analog_parameters v4l2_para;

	if (ioctl(fd, V4L2_GET_FRONTEND, &v4l2_para) == -1)
	{
		AM_DEBUG(1, "ioctl V4L2_GET_FRONTEND failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	para->frequency = v4l2_para.frequency;
	para->u.analog.audmode = v4l2_para.audmode;
	para->u.analog.soundsys = v4l2_para.soundsys;
	para->u.analog.std = v4l2_para.std;
	para->u.analog.flag = v4l2_para.flag;
	para->u.analog.afc_range = v4l2_para.afc_range;

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_set_prop (AM_FEND_Device_t *dev, const struct dtv_properties *prop)
{
	int fd = (long) dev->drv_data;
	struct v4l2_properties v4l2_prop;
	struct v4l2_property *property = NULL;
	int i = 0;

	property = (struct v4l2_property *) malloc(prop->num * sizeof(struct v4l2_property));
	if (property == NULL)
	{
		AM_DEBUG(1, "malloc failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	memset(&v4l2_prop, 0, sizeof(struct v4l2_properties));

	v4l2_prop.num = prop->num;
	v4l2_prop.props = property;

	for (i = 0; i < prop->num; ++i)
	{
		(v4l2_prop.props + i)->cmd = (prop->props + i)->cmd;
		(v4l2_prop.props + i)->data = (prop->props + i)->u.data;
	}

	AM_DEBUG(1, "V4L2_SET_PROPERTY cmd = 0x%x", prop->props->cmd);
	if (ioctl(fd, V4L2_SET_PROPERTY, &v4l2_prop) == -1)
	{
		AM_DEBUG(1, "ioctl V4L2_SET_PROPERTY failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	for (i = 0; i < prop->num; ++i)
	{
		(prop->props + i)->result = (v4l2_prop.props + i)->result;
	}

	if (property != NULL)
	{
		free(property);
		property = NULL;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_get_prop (AM_FEND_Device_t *dev, struct dtv_properties *prop)
{
	int fd = (long) dev->drv_data;
	struct v4l2_properties v4l2_prop;
	struct v4l2_property *property = NULL;
	int i = 0;

	property = (struct v4l2_property *) malloc(prop->num * sizeof(struct v4l2_property));
	if (property == NULL)
	{
		AM_DEBUG(1, "malloc failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	memset(&v4l2_prop, 0, sizeof(struct v4l2_properties));

	v4l2_prop.num = prop->num;
	v4l2_prop.props = property;

	for (i = 0; i < prop->num; ++i)
	{
		(v4l2_prop.props + i)->cmd = (prop->props + i)->cmd;
		(v4l2_prop.props + i)->data = (prop->props + i)->u.data;
	}

	AM_DEBUG(1, "V4L2_GET_PROPERTY cmd = 0x%x", prop->props->cmd);
	if (ioctl(fd, V4L2_GET_PROPERTY, &v4l2_prop) == -1)
	{
		AM_DEBUG(1, "ioctl V4L2_GET_PROPERTY failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	for (i = 0; i < prop->num; ++i)
	{
		(prop->props + i)->result = (v4l2_prop.props + i)->result;
		(prop->props + i)->u.data = (v4l2_prop.props + i)->data;
	}

	if (property != NULL)
	{
		free(property);
		property = NULL;
		v4l2_prop.props = NULL;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_get_status(AM_FEND_Device_t *dev, fe_status_t *status)
{
	int fd = (long) dev->drv_data;
	enum v4l2_status v4l2_st;

	if (ioctl(fd, V4L2_READ_STATUS, &v4l2_st) == -1)
	{
		AM_DEBUG(1, "ioctl V4L2_READ_STATUS failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	*status = (enum fe_status) v4l2_st;

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_wait_event(AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout)
{
	int fd = (long) dev->drv_data;
	struct pollfd pfd;
	int ret;
	struct v4l2_frontend_event v4l2_evt;

	pfd.fd = fd;
	pfd.events = POLLIN;

	ret = poll(&pfd, 1, timeout);
	if (ret != 1)
	{
		return AM_FEND_ERR_TIMEOUT;
	}

	if (ioctl(fd, V4L2_GET_EVENT, &v4l2_evt) == -1)
	{
		AM_DEBUG(1, "ioctl V4L2_GET_EVENT failed, error:%s", strerror(errno));
		return AM_FAILURE;
	}

	evt->status = (enum fe_status) v4l2_evt.status;
	evt->parameters.frequency = v4l2_evt.parameters.frequency;
	evt->parameters.u.analog.afc_range = v4l2_evt.parameters.afc_range;
	evt->parameters.u.analog.audmode = v4l2_evt.parameters.audmode;
	evt->parameters.u.analog.flag = v4l2_evt.parameters.flag;
	evt->parameters.u.analog.soundsys = v4l2_evt.parameters.soundsys;
	evt->parameters.u.analog.std = v4l2_evt.parameters.std;

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_close(AM_FEND_Device_t *dev)
{
	int fd = (long) dev->drv_data;

	close(fd);

	return AM_SUCCESS;
}

static AM_ErrorCode_t v4l2_get_tune_status(AM_FEND_Device_t *dev, atv_status_t *atv_status)
{
	int fd = (int) dev->drv_data;
	struct v4l2_tune_status status = { .lock = 0, .afc = 0, };

	if (ioctl(fd, V4L2_DETECT_TUNE, &status) == -1)
	{
		AM_DEBUG(1, "ioctl FE_READ_ANALOG_STATUS failed, errno: %s", strerror(errno));
		return AM_FAILURE;
	}

	atv_status->atv_lock = status.lock;
	atv_status->std = status.std;
	atv_status->audmode = status.audmode;
	atv_status->snr = status.snr;
	atv_status->afc = status.afc;

	return AM_SUCCESS;
}
