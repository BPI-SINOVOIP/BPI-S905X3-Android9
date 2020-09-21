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
 * \brief SDL OSD 模拟
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-18: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "../am_osd_internal.h"
#include <am_mem.h>
#include <am_thread.h>
#include <unistd.h>
#include <SDL/SDL.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define OSD_FPS         25
#define OSD_SCREEN_W    720
#define OSD_SCREEN_H    576
#define OSD_SCREEN_BPP  24

/****************************************************************************
 * Types definitions
 ***************************************************************************/

/**\brief OSD layer*/
typedef struct OSDLayer OSDLayer_t;
struct OSDLayer
{
	OSDLayer_t       *prev;
	OSDLayer_t       *next;
	int               dev_no;
	SDL_Surface      *surface;
	SDL_Surface      *dbuffer;
	AM_OSD_Surface_t *osd_surface;
	AM_OSD_Surface_t *disp_surface;
	AM_Bool_t         enable;
	uint8_t           alpha;
};

/**\brief OSD layers manager*/
typedef struct
{
	OSDLayer_t       *layers;
	pthread_mutex_t   lock;
	pthread_t         thread;
	SDL_Surface      *surface;
	int               init_counter;
	AM_Bool_t         running;
	AM_Bool_t         update;
	int               width;
	int               height;
} OSDMan_t;

/****************************************************************************
 * Static data
 ***************************************************************************/

static OSDMan_t osd_man;

static AM_ErrorCode_t sdl_open (AM_OSD_Device_t *dev, const AM_OSD_OpenPara_t *para, AM_OSD_Surface_t **s, AM_OSD_Surface_t **d);
static AM_ErrorCode_t sdl_enable (AM_OSD_Device_t *dev, AM_Bool_t enable);
static AM_ErrorCode_t sdl_update (AM_OSD_Device_t *dev, AM_OSD_Rect_t *rect);
static AM_ErrorCode_t sdl_set_palette (AM_OSD_Device_t *dev, AM_OSD_Color_t *col, int start, int count);
static AM_ErrorCode_t sdl_set_alpha (AM_OSD_Device_t *dev, uint8_t alpha);
static AM_ErrorCode_t sdl_close (AM_OSD_Device_t *dev);

const AM_OSD_Driver_t sdl_osd_drv = {
.open   = sdl_open,
.enable = sdl_enable,
.update = sdl_update,
.set_palette = sdl_set_palette,
.set_alpha   = sdl_set_alpha,
.close  = sdl_close
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

static void osd_update(void)
{
	OSDLayer_t *osd;
	SDL_Rect sr, dr;
	AM_Bool_t update = AM_FALSE;
	
	for(osd=osd_man.layers; osd; osd = osd->next)
	{
		if(!osd->enable)
			continue;
		
		sr.x = 0;
		sr.y = 0;
		sr.w = osd->surface->w;
		sr.h = osd->surface->h;
		dr.x = 0;
		dr.y = 0;
		dr.w = sr.w;
		dr.h = sr.h;
		SDL_SetAlpha(osd->surface, SDL_SRCALPHA, osd->alpha);
		SDL_BlitSurface(osd->surface, &sr, osd_man.surface, &dr);
		update = AM_TRUE;
	}
	
	if(update)
		SDL_Flip(osd_man.surface);
}

/**\brief OSD update thread*/
static void* osd_update_thread(void *arg)
{
	char *fps_env = getenv("AM_FPS");
	int fps, fdelay;
	
	if(fps_env)
	{
		if(sscanf(fps_env, "%d", &fps)!=1)
			fps = OSD_FPS;
	}
	else
	{
		fps = OSD_FPS;
	}
	
	fdelay = 1000/fps;
	
	osd_man.surface = SDL_SetVideoMode(osd_man.width, osd_man.height, OSD_SCREEN_BPP, 0);
	if(!osd_man.surface)
	{
		AM_DEBUG(1, "set video mode failed");
		return NULL;
	}
	
	while(osd_man.running)
	{
		pthread_mutex_lock(&osd_man.lock);
		
		if(osd_man.update && osd_man.layers)
		{
			osd_man.update = AM_FALSE;
			
			osd_update();
		}
		
		pthread_mutex_unlock(&osd_man.lock);
		
		usleep(fdelay*1000);
		
		SDL_PumpEvents();
	}
	
	return NULL;
}

/**\brief Create the OSD manager*/
static AM_ErrorCode_t osd_man_init(const AM_OSD_OpenPara_t *para)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	int w = OSD_SCREEN_W, h = OSD_SCREEN_H;
	
	memset(&osd_man, 0, sizeof(osd_man));
	pthread_mutex_init(&osd_man.lock, NULL);
	SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_EVENTTHREAD);
	
	if(para->enable_double_buffer)
	{
		w = para->output_width;
		h = para->output_height;
	}
	else
	{
		w = para->width;
		h = para->height;
	}
	
	osd_man.width  = w;
	osd_man.height = h;
	
	osd_man.update  = AM_TRUE;
	osd_man.running = AM_TRUE;
	if(pthread_create(&osd_man.thread, NULL, osd_update_thread, NULL))
	{
		AM_DEBUG(1, "create the osd update thread failed");
		goto error;
	}

	return ret;
error:
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	pthread_mutex_destroy(&osd_man.lock);
	return ret;
}

/**\brief Release the OSD manager*/
static void osd_man_deinit()
{
	osd_man.running = AM_FALSE;
	pthread_join(osd_man.thread, NULL);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	pthread_mutex_destroy(&osd_man.lock);
}

static AM_ErrorCode_t sdl_open (AM_OSD_Device_t *dev, const AM_OSD_OpenPara_t *para, AM_OSD_Surface_t **s, AM_OSD_Surface_t **d)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	OSDLayer_t *osd = NULL;
	AM_OSD_PixelFormat_t *fmt;
	AM_OSD_Surface_t *surf = NULL, *disp = NULL;
	uint8_t *buffer;
	int pitch;
	
	/*Initialize the OSD manager*/
	if(!osd_man.init_counter)
	{
		ret = osd_man_init(para);
		if(ret!=AM_SUCCESS)
			return ret;
		
		osd_man.init_counter++;
	}
	
	/*Allocate a new OSD layer*/
	osd = (OSDLayer_t*)malloc(sizeof(OSDLayer_t));
	if(!osd)
	{
		AM_DEBUG(1, "not enough memory");
		goto error;
	}
	
	osd->dev_no = dev->dev_no;
	osd->surface = NULL;
	osd->dbuffer = NULL;
	osd->alpha   = 0xFF;
	osd->enable  = AM_TRUE;
	osd->osd_surface  = NULL;
	osd->disp_surface = NULL;
	
	ret = AM_OSD_GetFormatPara(para->format, &fmt);
	if(ret<0)
		goto error;
	
	/*Create the surface*/
	osd->surface = SDL_AllocSurface(0, para->width, para->height, fmt->bits_per_pixel,
			fmt->r_mask, fmt->g_mask, fmt->b_mask, fmt->a_mask);
	if(!osd->surface)
	{
		AM_DEBUG(1, "cannot create surface");
		goto error;
	}
	buffer = osd->surface->pixels;
	pitch  = osd->surface->pitch;
	
	ret = AM_OSD_CreateSurfaceFromBuffer(para->format, para->width, para->height, pitch, buffer,
			AM_OSD_SURFACE_FL_EXTERNAL, &disp);
	if(ret!=AM_SUCCESS)
		goto error;
	osd->disp_surface = disp;
	*d = disp;
	
	/*Create the double buffer*/
	if(para->enable_double_buffer)
	{
		osd->dbuffer = SDL_AllocSurface(0, para->output_width, para->output_height, fmt->bits_per_pixel,
				fmt->r_mask, fmt->g_mask, fmt->b_mask, fmt->a_mask);
		if(!osd->surface)
		{
			AM_DEBUG(1, "cannot create surface");
			goto error;
		}
		buffer = osd->dbuffer->pixels;
		pitch  = osd->dbuffer->pitch;
		
		ret = AM_OSD_CreateSurfaceFromBuffer(para->format, para->width, para->height, pitch, buffer,
			AM_OSD_SURFACE_FL_EXTERNAL, &surf);
		if(ret!=AM_SUCCESS)
			goto error;
	
		osd->osd_surface = surf;
		*s = surf;
	}
	else
	{
		*s = disp;
	}
	
	/*Add the layer to the managed layer list*/
	pthread_mutex_lock(&osd_man.lock);
	
	if(!osd_man.layers)
	{
		osd->prev = NULL;
		osd->next = NULL;
		osd_man.layers = osd;
	}
	else
	{
		OSDLayer_t *prev = NULL, *tmp;
		
		for(prev=NULL,tmp=osd_man.layers; tmp; prev=tmp,tmp=tmp->next)
		{
			if(tmp->dev_no<osd->dev_no)
				break;
		}
		
		osd->prev = prev;
		osd->next = tmp;
		
		if(prev)
			prev->next = osd;
		else
			osd_man.layers = osd;
		if(tmp)
			tmp->prev = osd;
	}
	osd_man.update = AM_TRUE;
	
	pthread_mutex_unlock(&osd_man.lock);
	
	dev->drv_data = osd;
	
	return ret;
error:
	if(surf)
		AM_OSD_DestroySurface(surf);
	if(disp)
		AM_OSD_DestroySurface(disp);
	if(osd)
	{
		if(osd->surface)
			SDL_FreeSurface(osd->surface);
		if(osd->dbuffer)
			SDL_FreeSurface(osd->dbuffer);
		free(osd);
	}
	osd_man.init_counter--;
	if(!osd_man.init_counter)
	{
		osd_man_deinit();
	}
	return ret;
}

static AM_ErrorCode_t sdl_enable (AM_OSD_Device_t *dev, AM_Bool_t enable)
{
	OSDLayer_t *osd = (OSDLayer_t*)dev->drv_data;
	
	pthread_mutex_lock(&osd_man.lock);
	
	osd->enable = enable;
	osd_man.update = AM_TRUE;
	
	pthread_mutex_unlock(&osd_man.lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sdl_update (AM_OSD_Device_t *dev, AM_OSD_Rect_t *rect)
{
	OSDLayer_t *osd = (OSDLayer_t*)dev->drv_data;
	
	pthread_mutex_lock(&osd_man.lock);
	
	if(osd->dbuffer)
	{
		SDL_Rect sr, dr;
		
		sr.x = 0;
		sr.y = 0;
		sr.w = osd->dbuffer->w;
		sr.h = osd->dbuffer->h;
		dr.x = 0;
		dr.y = 0;
		dr.w = osd->surface->w;
		dr.h = osd->surface->h;
		
		SDL_SetAlpha(osd->dbuffer, 0, 0);
		
		SDL_BlitSurface(osd->dbuffer, &sr, osd->surface, &dr);
	}
	
	osd_man.update = AM_TRUE;
	
	pthread_mutex_unlock(&osd_man.lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sdl_set_palette (AM_OSD_Device_t *dev, AM_OSD_Color_t *col, int start, int count)
{
	OSDLayer_t *osd = (OSDLayer_t*)dev->drv_data;
	SDL_Color pal[256];
	int i;
	
	for(i=0; i<count; i++)
	{
		pal[i].r = col[i].r;
		pal[i].g = col[i].g;
		pal[i].b = col[i].b;
		pal[i].unused = col[i].a;
	}
	
	pthread_mutex_lock(&osd_man.lock);
	
	SDL_SetPalette(osd->surface, SDL_LOGPAL, pal, start, count);
	
	if(osd->dbuffer)
		SDL_SetPalette(osd->dbuffer, SDL_LOGPAL, pal, start, count);
	
	pthread_mutex_unlock(&osd_man.lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sdl_set_alpha (AM_OSD_Device_t *dev, uint8_t alpha)
{
	OSDLayer_t *osd = (OSDLayer_t*)dev->drv_data;
	
	pthread_mutex_lock(&osd_man.lock);
	
	osd->alpha = alpha;
	osd_man.update = AM_TRUE;
	
	pthread_mutex_unlock(&osd_man.lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sdl_close (AM_OSD_Device_t *dev)
{
	OSDLayer_t *osd = (OSDLayer_t*)dev->drv_data;
	
	pthread_mutex_lock(&osd_man.lock);
	
	if(osd->prev)
		osd->prev->next = osd->next;
	else
		osd_man.layers = osd->next;
	if(osd->next)
		osd->next->prev = osd->prev;
	
	osd_man.update = AM_TRUE;
	
	pthread_mutex_unlock(&osd_man.lock);
	
	if(osd->disp_surface)
		AM_OSD_DestroySurface(osd->disp_surface);
	if(osd->osd_surface)
		AM_OSD_DestroySurface(osd->osd_surface);
	if(osd->surface)
		SDL_FreeSurface(osd->surface);
	if(osd->dbuffer)
		SDL_FreeSurface(osd->dbuffer);
	
	free(osd);
	
	osd_man.init_counter--;
	if(!osd_man.init_counter)
	{
		osd_man_deinit();
	}
	
	return AM_SUCCESS;
}
