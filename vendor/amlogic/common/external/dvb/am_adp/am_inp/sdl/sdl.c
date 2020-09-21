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
 * \brief SDL输入设备
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-03: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "../am_inp_internal.h"
#include <am_time.h>
#include <am_thread.h>
#include <SDL/SDL.h>
#include <errno.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static function declaration
 ***************************************************************************/
static AM_ErrorCode_t sdl_open (AM_INP_Device_t *dev, const AM_INP_OpenPara_t *para);
static AM_ErrorCode_t sdl_close (AM_INP_Device_t *dev);
static AM_ErrorCode_t sdl_wait (AM_INP_Device_t *dev, struct input_event *event, int timeout);

/****************************************************************************
 * Static data
 ***************************************************************************/

/**\brief SDL输入设备驱动*/
const AM_INP_Driver_t sdl_drv =
{
.open =  sdl_open,
.close = sdl_close,
.wait =  sdl_wait
};

static pthread_mutex_t lock;
static pthread_cond_t  cond;
static SDL_Event       curr_event;

/****************************************************************************
 * Static functions
 ***************************************************************************/

static int evt_filter(const SDL_Event *event)
{
	if((event->type==SDL_KEYDOWN) || (event->type==SDL_KEYUP))
	{
		pthread_mutex_lock(&lock);
		curr_event = *event;
		pthread_mutex_unlock(&lock);
		pthread_cond_broadcast(&cond);
	}
	return 0;
}

static AM_ErrorCode_t sdl_open (AM_INP_Device_t *dev, const AM_INP_OpenPara_t *para)
{
	SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_EVENTTHREAD);
	
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
	curr_event.type = SDL_NOEVENT;
	
	SDL_SetEventFilter(evt_filter);
	return AM_SUCCESS;
}

static AM_ErrorCode_t sdl_close (AM_INP_Device_t *dev)
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_EVENTTHREAD);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sdl_wait (AM_INP_Device_t *dev, struct input_event *event, int timeout)
{
	SDL_Event sdl_evt;
	int ret = 0;
	
	pthread_mutex_lock(&lock);
	
	if(curr_event.type==SDL_NOEVENT)
	{
		if(timeout<0)
		{
			ret = pthread_cond_wait(&cond, &lock);
		}
		else
		{
			struct timespec ts;		
			AM_TIME_GetTimeSpecTimeout(timeout, &ts);
			ret = pthread_cond_timedwait(&cond, &lock, &ts);
		}
		
		if(ret==0)
		{
			sdl_evt = curr_event;
			curr_event.type = SDL_NOEVENT;
		}
	}
	
	pthread_mutex_unlock(&lock);
	
	if(ret==ETIMEDOUT)
		return AM_INP_ERR_TIMEOUT;
	else if(ret)
		return AM_FAILURE;
	
	event->type = EV_KEY;
	
	if(sdl_evt.type==SDL_KEYDOWN)
		event->value = 1;
	else
		event->value = 0;
	
	switch(sdl_evt.key.keysym.sym)
	{
		case SDLK_UP:
			event->code = AM_INP_KEY_UP;
		break;
		case SDLK_DOWN:
			event->code = AM_INP_KEY_DOWN;
		break;
		case SDLK_LEFT:
			event->code = AM_INP_KEY_LEFT;
		break;
		case SDLK_RIGHT:
			event->code = AM_INP_KEY_RIGHT;
		break;
		case SDLK_RETURN:
			event->code = AM_INP_KEY_OK;
		break;
		case SDLK_ESCAPE:
			event->code = AM_INP_KEY_CANCEL;
		break;
		case SDLK_p:
			event->code = AM_INP_KEY_POWER;
		break;
		default:
		break;
	}
	
	return AM_SUCCESS;
}

