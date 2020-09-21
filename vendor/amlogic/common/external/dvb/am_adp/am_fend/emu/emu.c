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
 * \brief 文件模拟DVB前端设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-08: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_time.h>
#include "../am_fend_internal.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define EMU_FL_EVENT   (1)
#define EMU_FL_WAIT    (2)
 
/****************************************************************************
 * Typedef
 ***************************************************************************/
typedef struct
{
	char         name[PATH_MAX];
	int          flags;
	fe_status_t  status;
	struct dvb_frontend_parameters para;
	pthread_cond_t  cond;
	pthread_mutex_t lock;
} EMU_FEnd;

/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t emu_open (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para);
static AM_ErrorCode_t emu_set_para (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para);
static AM_ErrorCode_t emu_get_para (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para);
static AM_ErrorCode_t emu_get_status (AM_FEND_Device_t *dev, fe_status_t *status);
static AM_ErrorCode_t emu_get_snr (AM_FEND_Device_t *dev, int *snr);
static AM_ErrorCode_t emu_get_ber (AM_FEND_Device_t *dev, int *ber);
static AM_ErrorCode_t emu_get_strength (AM_FEND_Device_t *dev, int *strength);
static AM_ErrorCode_t emu_wait_event (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout);
static AM_ErrorCode_t emu_close (AM_FEND_Device_t *dev);

/****************************************************************************
 * Static data
 ***************************************************************************/
const AM_FEND_Driver_t emu_fend_drv =
{
.open = emu_open,
.set_para = emu_set_para,
.get_para = emu_get_para,
.get_status = emu_get_status,
.get_snr = emu_get_snr,
.get_ber = emu_get_ber,
.get_strength = emu_get_strength,
.wait_event = emu_wait_event,
.close = emu_close
};

/****************************************************************************
 * Functions
 ***************************************************************************/

static AM_ErrorCode_t emu_open (AM_FEND_Device_t *dev, const AM_FEND_OpenPara_t *para)
{
	EMU_FEnd *fe;
	
	fe = (EMU_FEnd*)AM_MEM_ALLOC_TYPE0(EMU_FEnd);
	if(!fe)
	{
		return AM_FEND_ERR_NO_MEM;
	}
	
	pthread_cond_init(&fe->cond, NULL);
	pthread_mutex_init(&fe->lock, NULL);
	
	dev->info.type= FE_QAM;
	snprintf(dev->info.name, sizeof(dev->info.name), "frontend%d", dev->dev_no);
	dev->info.frequency_min = 100000;
        dev->info.frequency_max = 999000;
        dev->info.frequency_stepsize = 1000;
        dev->info.frequency_tolerance = 0;
        dev->info.symbol_rate_min = 1000;
        dev->info.symbol_rate_max = 10000;
        dev->info.symbol_rate_tolerance = 0;
        dev->info.notifier_delay = 0;
        dev->info.caps = 0;

	dev->drv_data = fe;
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_set_para (AM_FEND_Device_t *dev, const struct dvb_frontend_parameters *para)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	int freq = para->frequency/1000;
	struct stat sbuf;
	
	snprintf(fe->name, sizeof(fe->name), "%d.ts", freq);
	
	pthread_mutex_lock(&fe->lock);
	if(stat(fe->name, &sbuf)==0)
	{
		fe->status = FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC|FE_HAS_LOCK;
		setenv("AM_TSFILE", fe->name, 1);
	}
	else
	{
		fe->status = FE_TIMEDOUT;
		unsetenv("AM_TSFILE");
	}
	
	fe->para = *para;
	
	fe->flags |= EMU_FL_EVENT;
	pthread_cond_broadcast(&fe->cond);
	
	pthread_mutex_unlock(&fe->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_get_para (AM_FEND_Device_t *dev, struct dvb_frontend_parameters *para)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	
	pthread_mutex_lock(&fe->lock);
	*para = fe->para;
	pthread_mutex_unlock(&fe->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_get_status (AM_FEND_Device_t *dev, fe_status_t *status)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	
	pthread_mutex_lock(&fe->lock);
	*status = fe->status;
	pthread_mutex_unlock(&fe->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_get_snr (AM_FEND_Device_t *dev, int *snr)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	
	pthread_mutex_lock(&fe->lock);
	*snr = (fe->status&FE_HAS_LOCK)?85:12;
	pthread_mutex_unlock(&fe->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_get_ber (AM_FEND_Device_t *dev, int *ber)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	
	pthread_mutex_lock(&fe->lock);
	*ber = (fe->status&FE_HAS_LOCK)?10:30;
	pthread_mutex_unlock(&fe->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_get_strength (AM_FEND_Device_t *dev, int *strength)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	
	pthread_mutex_lock(&fe->lock);
	*strength = (fe->status&FE_HAS_LOCK)?92:12;
	pthread_mutex_unlock(&fe->lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t emu_wait_event (AM_FEND_Device_t *dev, struct dvb_frontend_event *evt, int timeout)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	pthread_mutex_lock(&fe->lock);
	
	fe->flags |= EMU_FL_WAIT;
	
	if(timeout>=0)
	{
		struct timespec ts;
		int rc;

		if(fe->flags&EMU_FL_EVENT)
		{
			rc = 0;
		}
		else
		{
			AM_TIME_GetTimeSpecTimeout(timeout, &ts);
			rc = pthread_cond_timedwait(&fe->cond, &fe->lock, &ts);
		}
		if(rc==ETIMEDOUT)
		{
			ret = AM_FEND_ERR_TIMEOUT;
		}
		else if(rc!=0)
		{
			ret = AM_FAILURE;
		}
	}
	else
	{
		while(!(fe->flags&EMU_FL_EVENT))
		{
			pthread_cond_wait(&fe->cond, &fe->lock);
		}
	}
	
	if(ret>=0)
	{
		evt->status = fe->status;
		evt->parameters = fe->para;
		fe->flags &= ~(EMU_FL_EVENT|EMU_FL_WAIT);
	}
	
	pthread_mutex_unlock(&fe->lock);
	
	return ret;
}

static AM_ErrorCode_t emu_close (AM_FEND_Device_t *dev)
{
	EMU_FEnd *fe = (EMU_FEnd*)dev->drv_data;
	
	if(fe)
	{
		pthread_cond_destroy(&fe->cond);
		pthread_mutex_destroy(&fe->lock);
		AM_MEM_Free(fe);
	}
	
	return AM_SUCCESS;
}

