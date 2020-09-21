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
 * \brief for ATV frontend interface
 *
 * \author nengwen.chen <nengwen.chen@amlogic.com>
 * \date 2018-04-16: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include "am_fend_internal.h"
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include "../am_adp_internal.h"
#include <am_vlfend.h>
#include <atv_frontend.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define FEND_DEV_COUNT      (1)
#define FEND_WAIT_TIMEOUT   (500)

#define M_BS_START_FREQ				(950)				/*The start RF frequency, 950MHz*/
#define M_BS_STOP_FREQ				(2150)				/*The stop RF frequency, 2150MHz*/
#define M_BS_MAX_SYMB				(45)
#define M_BS_MIN_SYMB				(2)

/****************************************************************************
 * Static data
 ***************************************************************************/

pthread_mutex_t v4l2_gAdpLock = PTHREAD_MUTEX_INITIALIZER;

extern const AM_FEND_Driver_t linux_v4l2_fend_drv;

static AM_FEND_Device_t vlfend_devices[FEND_DEV_COUNT] =
{
	[0] = {
		.dev_no = 0,
		.drv = &linux_v4l2_fend_drv,
	},
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief Gets the device structure pointer according to the device number. */
static AM_INLINE AM_ErrorCode_t vlfend_get_dev(int dev_no, AM_FEND_Device_t **dev)
{
	if ((dev_no < 0) || (dev_no >= FEND_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid vlfrontend device number %d, must in(%d~%d)",
				dev_no, 0, FEND_DEV_COUNT-1);
		return AM_FEND_ERR_INVALID_DEV_NO;
	}

	*dev = &vlfend_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief Obtain the device structure according to the device number and check whether the device is opened. */
static AM_INLINE AM_ErrorCode_t vlfend_get_openned_dev(int dev_no, AM_FEND_Device_t **dev)
{
	AM_TRY(vlfend_get_dev(dev_no, dev));

	if ((*dev)->open_count <= 0)
	{
		AM_DEBUG(1, "vlfrontend device %d has not been openned", dev_no);
		return AM_FEND_ERR_INVALID_DEV_NO;
	}

	return AM_SUCCESS;
}

/**\brief The atv frontend device monitors threads */
static void* vlfend_thread(void *arg)
{
	AM_FEND_Device_t *dev = (AM_FEND_Device_t*) arg;
	struct dvb_frontend_event evt;
	AM_ErrorCode_t ret = AM_FAILURE;

	while (dev->enable_thread)
	{
		/*when blind scan is start, we need stop fend thread read event*/
/*
		if (dev->enable_blindscan_thread)
		{
			usleep(100 * 1000);
			continue;
		}
*/
		if (dev->drv->wait_event)
		{
			ret = dev->drv->wait_event(dev, &evt, FEND_WAIT_TIMEOUT);
		}

		if (dev->enable_thread)
		{
			pthread_mutex_lock(&dev->lock);
			dev->flags |= VLFEND_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);

			if (ret == AM_SUCCESS)
			{
				AM_DEBUG(1, "vlfend_thread wait evt: %x\n", evt.status);

				if (dev->cb && dev->enable_cb)
				{
					dev->cb(dev->dev_no, &evt, dev->user_data);
				}

				if (dev->enable_cb)
				{
					AM_EVT_Signal(dev->dev_no, AM_VLFEND_EVT_STATUS_CHANGED, &evt);
				}

			}

			pthread_mutex_lock(&dev->lock);
			dev->flags &= ~VLFEND_FL_RUN_CB;
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);
		}
	}

	return NULL;
}

static void sighand(int signo) {}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief Open a atv frontend device
 * \param dev_no Frontend device number
 * \param[in] para device opening parameters
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values (see am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_Open(int dev_no, const AM_FEND_OpenPara_t *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	int rc;

	AM_TRY(vlfend_get_dev(dev_no, &dev));

	pthread_mutex_lock(&v4l2_gAdpLock);

	if (dev->open_count > 0)
	{
		AM_DEBUG(1, "vlfrontend device %d has already been openned", dev_no);
		dev->open_count++;
		ret = AM_SUCCESS;
		goto final;
	}

	if (dev->drv->open)
	{
		AM_TRY_FINAL(dev->drv->open(dev, para));
	}

	pthread_mutex_init(&dev->lock, NULL);
	pthread_cond_init(&dev->cond, NULL);

	dev->dev_no = dev_no;
	dev->open_count = 1;
	dev->enable_thread = AM_TRUE;
	dev->flags = 0;
	dev->enable_cb = AM_TRUE;
	dev->curr_mode = para->mode;

	rc = pthread_create(&dev->thread, NULL, vlfend_thread, dev);
	if (rc)
	{
		AM_DEBUG(1, "%s", strerror(rc));

		if (dev->drv->close)
		{
			dev->drv->close(dev);
		}
		pthread_mutex_destroy(&dev->lock);
		pthread_cond_destroy(&dev->cond);
		dev->open_count = 0;

		ret = AM_FEND_ERR_CANNOT_CREATE_THREAD;
		goto final;
	}

	{
		struct sigaction actions;
		memset(&actions, 0, sizeof(actions));
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = sighand;
		rc = sigaction(SIGALRM, &actions, NULL);
		if (rc != 0)
			AM_DEBUG(1, "sigaction: err=%d", errno);
	}

final:
	pthread_mutex_unlock(&v4l2_gAdpLock);

	return ret;
}

/**\brief Close a atv frontend device
 * \param dev_no Device number
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values (see am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_CloseEx(int dev_no, AM_Bool_t reset)
{
	AM_FEND_Device_t *dev;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&v4l2_gAdpLock);

	if (dev->open_count == 1)
	{
		int err = 0;

		dev->enable_cb = AM_FALSE;
		/*Stop the thread*/
		dev->enable_thread = AM_FALSE;
		err = pthread_kill(dev->thread, SIGALRM);
		if (err != 0)
			AM_DEBUG(1, "kill fail, err:%d", err);
		pthread_join(dev->thread, NULL);
		/*Release the device*/
		if (dev->drv->close)
		{
			if (reset && dev->drv->set_mode)
				dev->drv->set_mode(dev, FE_UNKNOWN);

			dev->drv->close(dev);
		}

		pthread_mutex_destroy(&dev->lock);
		pthread_cond_destroy(&dev->cond);
	}
	dev->open_count--;

	pthread_mutex_unlock(&v4l2_gAdpLock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_Close(int dev_no)
{
	return AM_VLFEND_CloseEx(dev_no, AM_TRUE);
}

/**\brief Set the atv frontend demodulation mode
 * \param dev_no Device number
 * \param mode Demodulation mode
 * \return
 *   - AM_SUCCESS succeed
 *   - Other valuesee (am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_SetMode(int dev_no, int mode)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));
	AM_DEBUG(1, "AM_VLFEND_SetMode %d", mode);
	if (!dev->drv->set_mode)
	{
		AM_DEBUG(1, "vlfronend %d no not support set_mode", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->set_mode(dev, mode);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Set atv frontend parameters
 * \param dev_no Device number
 * \param[in] para Frontend setting parameters
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_SetPara(int dev_no, const struct dvb_frontend_parameters *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(para);

	AM_DEBUG(1, "AM_VLFEND_SetPara\n");

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->set_para)
	{
		AM_DEBUG(1, "vlfronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->set_para(dev, para);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Gets the parameters set by the atv frontend device
 * \param dev_no Device number
 * \param[out] para Frontend setting parameters
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_GetPara(int dev_no, struct dvb_frontend_parameters *para)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(para);

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->get_para)
	{
		AM_DEBUG(1, "vlfronend %d no not support get_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->get_para(dev, para);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Set atv frontend properties
 * \param dev_no Device number
 * \param para Frontend properties setting parameters
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_SetProp(int dev_no, const struct dtv_properties *prop)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(prop);

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->set_prop)
	{
		AM_DEBUG(1, "vlfronend %d no not support set_prop", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->set_prop(dev, prop);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Get atv frontend properties
 * \param dev_no Device number
 * \param[out] para Frontend properties setting parameters see(atv_fend.h)
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_GetProp(int dev_no, struct dtv_properties *prop)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(prop);

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->get_prop)
	{
		AM_DEBUG(1, "vlfronend %d no not support get_prop", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->get_prop(dev, prop);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Gets the current lock state of the atv frontend device
 * \param dev_no device number
 * \param[out] status Returns the locked state of the frontend device
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_GetStatus(int dev_no, fe_status_t *status)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(status);

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->get_status)
	{
		AM_DEBUG(1, "vlfronend %d no not support get_status", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);

	ret = dev->drv->get_status(dev, status);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

AM_ErrorCode_t AM_VLFEND_GetAFC(int dev_no, int *afc)
{
	AM_ErrorCode_t ret = AM_SUCCESS;

	assert(afc);

	struct dtv_properties props;
	struct dtv_property prop;

	memset(&props, 0, sizeof(props));
	memset(&prop, 0, sizeof(prop));

	prop.cmd = V4L2_AFC;
	prop.u.data = 0;

	props.num = 1;
	props.props = &prop;

	ret = AM_VLFEND_GetProp(dev_no, &props);

	*afc = prop.u.data;

	return ret;
}

/**\brief Gets the frontend status monitoring callback function for the current registration
 * \param dev_no Device number
 * \param[out] cb Returns the registered status callback function
 * \param[out] user_data Returns the argument to the status callback function
 * \return
 *   - AM_SUCCESS succeed
 *   - Other value see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_GetCallback(int dev_no, AM_FEND_Callback_t *cb, void **user_data)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (cb)
	{
		*cb = dev->cb;
	}

	if (user_data)
	{
		*user_data = dev->user_data;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Register the frontend device status monitoring callback function
 * \param dev_no Device number
 * \param[in] cb State callback function
 * \param[in] user_data Parameter to the state callback function
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_SetCallback(int dev_no, AM_FEND_Callback_t cb, void *user_data)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (cb != dev->cb || user_data != dev->user_data)
	{
		if (dev->enable_thread && (dev->thread != pthread_self()))
		{
			/* Wait for the callback function to finish executing */
			while (dev->flags & VLFEND_FL_RUN_CB)
			{
				pthread_cond_wait(&dev->cond, &dev->lock);
			}
		}

		dev->cb = cb;
		dev->user_data = user_data;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Set the activity state of the frontend device state monitoring callback function
 * \param dev_no Device number
 * \param[in] enable_cb Allows or disables state callbacks
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_SetActionCallback(int dev_no, AM_Bool_t enable_cb)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	pthread_mutex_lock(&dev->lock);

	if (enable_cb != dev->enable_cb)
	{
#if 0
		if (dev->enable_thread && (dev->thread != pthread_self()))
		{
			/* Wait for the callback function to finish executing */
			while (dev->flags & VLFEND_FL_RUN_CB)
			{
				pthread_cond_wait(&dev->cond, &dev->lock);
			}
		}
#endif
		dev->enable_cb = enable_cb;
	}

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief AM_FEND_Lock's parameter to the callback function */
typedef struct {
	const struct dvb_frontend_parameters *para;
	fe_status_t                          *status;
	AM_FEND_Callback_t                    old_cb;
	void                                 *old_data;
} fend_lock_para_t;

/**\brief AM_FEND_Lock's callback function */
static void vlfend_lock_cb(int dev_no, struct dvb_frontend_event *evt, void *user_data)
{
	AM_FEND_Device_t *dev = NULL;
	fend_lock_para_t *para = (fend_lock_para_t*) user_data;

	vlfend_get_openned_dev(dev_no, &dev);
/*
	if (!fend_para_equal(dev->curr_mode, &evt->parameters, para->para))
		return;
*/
	if (!evt->status)
		return;

	*para->status = evt->status;

	pthread_mutex_lock(&dev->lock);
	dev->flags &= ~VLFEND_FL_LOCK;
	pthread_mutex_unlock(&dev->lock);

	if (para->old_cb)
	{
		para->old_cb(dev_no, evt, para->old_data);
	}

	pthread_cond_broadcast(&dev->cond);
}

/**\brief Set parameters of frontend equipment and wait for the completion of parameter setting
 * \param dev_no Device number
 * \param[in] para Frontend setting parameter
 * \param[out] status Returns the state of the frontend device
 * \return
 *   - AM_SUCCESS succeed
 *   - Other value see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_Lock(int dev_no, const struct dvb_frontend_parameters *para, fe_status_t *status)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	fend_lock_para_t lockp;

	assert(para && status);

	AM_DEBUG(1, "AM_VLFEND_Lock\n");

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->set_para)
	{
		AM_DEBUG(1, "vlfronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	if (dev->thread == pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_VLFEND_Lock in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}

	pthread_mutex_lock(&dev->lock);

	/* Wait for the callback function to finish executing */
	while (dev->flags & VLFEND_FL_RUN_CB)
	{
		pthread_cond_wait(&dev->cond, &dev->lock);
	}
	AM_DEBUG(1, "AM_VLFEND_Lock line:%d\n", __LINE__);

	lockp.old_cb = dev->cb;
	lockp.old_data = dev->user_data;
	lockp.para = para;
	lockp.status = status;

	dev->cb = vlfend_lock_cb;
	dev->user_data = &lockp;
	dev->flags |= VLFEND_FL_LOCK;

	AM_DEBUG(1, "AM_VLFEND_Lock line:%d\n", __LINE__);
	ret = dev->drv->set_para(dev, para);

	AM_DEBUG(1, "AM_VLFEND_Lock line:%d,ret:%d\n", __LINE__, ret);
	if (ret == AM_SUCCESS)
	{
		/* Wait for the callback function to finish executing */
		while ((dev->flags & VLFEND_FL_RUN_CB) || (dev->flags & VLFEND_FL_LOCK))
		{
			pthread_cond_wait(&dev->cond, &dev->lock);
		}
	}
	AM_DEBUG(1, "AM_VLFEND_Lock line:%d\n", __LINE__);

	dev->cb = lockp.old_cb;
	dev->user_data = lockp.old_data;

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief Sets the detection interval for the frontend management thread
 * \param dev_no Device number
 * \param delay Interval time (milliseconds), 0 means no interval, and <0 means the frontend administrative thread is suspended
 * \return
 *   - AM_SUCCESS succeed
 *   - Other values see(am_fend.h)
 */
AM_ErrorCode_t AM_VLFEND_SetThreadDelay(int dev_no, int delay)
{
	AM_FEND_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->set_para)
	{
		AM_DEBUG(1, "vlfronend %d no not support set_para", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	if (dev->thread == pthread_self())
	{
		AM_DEBUG(1, "cannot invoke AM_VLFEND_Lock in callback");
		return AM_FEND_ERR_INVOKE_IN_CB;
	}

	pthread_mutex_lock(&dev->lock);

	if (dev->drv->set_delay)
		ret = dev->drv->set_delay(dev, delay);

	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief get frontend atv tune status
 * \param dev_no frontend device number
 * \param atv_status tune status value
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
AM_ErrorCode_t AM_VLFEND_GetAtvStatus(int dev_no, atv_status_t *atv_status)
{
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	if (!dev->drv->get_atv_status)
	{
		AM_DEBUG(1, "vlfronend %d no not support get atv status", dev_no);
		return AM_FEND_ERR_NOT_SUPPORTED;
	}

	pthread_mutex_lock(&dev->lock);
	ret = dev->drv->get_atv_status(dev, atv_status);
	pthread_mutex_unlock(&dev->lock);

	return ret;
}

/**\brief start frontend detect standard
 * \param dev_no frontend device number
 * \return
 *   - AM_SUCCESS On success
 *   - or error code
 */
AM_ErrorCode_t AM_VLFEND_DetectStandard(int dev_no)
{
	int fd = -1;
	AM_FEND_Device_t *dev = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;

	AM_TRY(vlfend_get_openned_dev(dev_no, &dev));

	fd = (int) dev->drv_data;

	pthread_mutex_lock(&dev->lock);
	if (ioctl(fd, V4L2_DETECT_STANDARD) == -1)
	{
		pthread_mutex_unlock(&dev->lock);
		AM_DEBUG(1, "ioctl V4L2_DETECT_AUDIO failed, errno: %s", strerror(errno));
		return AM_FAILURE;
	}
	pthread_mutex_unlock(&dev->lock);

	return AM_SUCCESS;
}
