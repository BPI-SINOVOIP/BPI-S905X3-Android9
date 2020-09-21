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
 * \brief Frame buffer OSD
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-02: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_misc.h>
#include <am_vout.h>
#include "../am_osd_internal.h"
#include <am_mem.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <unistd.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define  FBIOPUT_OSD_SRCCOLORKEY        0x46fb
#define  FBIOPUT_OSD_SRCKEY_ENABLE      0x46fa
#define  FBIOPUT_OSD_SET_GBL_ALPHA      0x4500
#define  FBIOGET_OSD_GET_GBL_ALPHA      0x4501

#define  DISP_MODE_FILE      "/sys/class/display/mode"
#define  FB_BLANK_FILE       "/sys/class/graphics/fb%d/blank"

/****************************************************************************
 * Types definitions
 ***************************************************************************/
typedef struct
{
	int        fd;        /**< File descriptor*/
	char       name[32];  /**< Device name*/
	int        id;        /**< Device ID*/
	uint8_t   *buf;       /**< Pixel buffer*/
	int        buf_len;   /**< Length of the buffer*/
	AM_OSD_Surface_t    *draw_surf; /**< Draw surface*/
	AM_OSD_Surface_t    *disp_surf; /**< Display surface*/
} FBOSD_t;

/****************************************************************************
 * Static data
 ***************************************************************************/

static AM_ErrorCode_t fb_open (AM_OSD_Device_t *dev, const AM_OSD_OpenPara_t *para, AM_OSD_Surface_t **s, AM_OSD_Surface_t **d);
static AM_ErrorCode_t fb_enable (AM_OSD_Device_t *dev, AM_Bool_t enable);
static AM_ErrorCode_t fb_update (AM_OSD_Device_t *dev, AM_OSD_Rect_t *rect);
static AM_ErrorCode_t fb_set_palette (AM_OSD_Device_t *dev, AM_OSD_Color_t *col, int start, int count);
static AM_ErrorCode_t fb_set_alpha (AM_OSD_Device_t *dev, uint8_t alpha);
static AM_ErrorCode_t fb_close (AM_OSD_Device_t *dev);

const AM_OSD_Driver_t fb_osd_drv = {
.open   = fb_open,
.enable = fb_enable,
.update = fb_update,
.set_palette = fb_set_palette,
.set_alpha   = fb_set_alpha,
.close  = fb_close
};

#ifdef AMLINUX
extern AM_ErrorCode_t ge2d_set_coef(int type);
#endif

/**\brief 视频输出模式变化事件回调*/
static void av_vout_format_changed(int dev_no, int event_type, void *param, void *data)
{
	AM_OSD_Device_t *dev = (AM_OSD_Device_t*)data;
	int coef = 1;
	
	if(event_type==AM_VOUT_EVT_FORMAT_CHANGED)
	{
		AM_VOUT_Format_t fmt = (AM_VOUT_Format_t)param;
		int w, h;
		
		w = dev->vout_w;
		h = dev->vout_h;
		
		switch(fmt)
		{
			case AM_VOUT_FORMAT_576CVBS:
				w = 720;
				h = 576;
				coef = 3;
			break;
			case AM_VOUT_FORMAT_480CVBS:
				w = 720;
				h = 480;
				coef = 3;
			break;
			case AM_VOUT_FORMAT_576I:
				w = 720;
				h = 576;
				coef = 3;
			break;
			case AM_VOUT_FORMAT_576P:
				w = 720;
				h = 576;
				coef = 3;
			break;
			case AM_VOUT_FORMAT_480I:
				w = 720;
				h = 480;
				coef = 3;
			break;
			case AM_VOUT_FORMAT_480P:
				w = 720;
				h = 480;
				coef = 3;
			break;
			case AM_VOUT_FORMAT_720P:
				w = 1280;
				h = 720;
				coef = 1;
			break;
			case AM_VOUT_FORMAT_1080I:
				w = 1920;
				h = 1080;
				coef = 1;
			break;
			case AM_VOUT_FORMAT_1080P:
				w = 1920;
				h = 1080;
				coef = 1;
			break;
			default:
			break;
		}
		
		if((w!=dev->vout_w) || (h!=dev->vout_h))
		{
			dev->vout_w = w;
			dev->vout_h = h;
			
			AM_OSD_Update(dev->dev_no, NULL);
			
#ifdef AMLINUX
			ge2d_set_coef(coef);
			if(!dev->enable)
				fb_enable(dev, dev->enable);
#endif
		}
	}
}

/****************************************************************************
 * Functions
 ***************************************************************************/

static AM_ErrorCode_t fb_open (AM_OSD_Device_t *dev, const AM_OSD_OpenPara_t *para, AM_OSD_Surface_t **s, AM_OSD_Surface_t **d)
{
	FBOSD_t *fb = NULL;
	AM_ErrorCode_t ret = AM_SUCCESS;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	uint32_t bpp, mode, flags;
	uint8_t *buf;
	int pitch, coef = 1, output_w, output_h;
	char str[32];
	
	if(AM_FileRead(DISP_MODE_FILE, str, sizeof(str))==AM_SUCCESS)
	{
		if(!strncmp(str, "576cvbs", 7) || !strncmp(str, "576i", 4) || !strncmp(str, "576i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 576;
			coef = 3;
		}
		else if (!strncmp(str, "480cvbs", 7) || !strncmp(str, "480i", 4) || !strncmp(str, "480i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 480;
			coef = 3;
		}
		else if (!strncmp(str, "720p", 4))
		{
			dev->vout_w = 1280;
			dev->vout_h = 720;
			coef = 1;
		}
		else if (!strncmp(str, "1080i", 5) || !strncmp(str, "1080p", 5))
		{
			dev->vout_w = 1920;
			dev->vout_h = 1080;
			coef = 1;
		}
	}
	
	AM_EVT_Subscribe(dev->vout_dev_no, AM_VOUT_EVT_FORMAT_CHANGED, av_vout_format_changed, dev);
	
	switch(para->format)
	{
		case AM_OSD_FMT_PALETTE_256:
			mode = 8;
			bpp = 1;
		break;
		case AM_OSD_FMT_COLOR_4444:
			mode = 15;
			bpp = 2;
		break;
		case AM_OSD_FMT_COLOR_1555:
			mode = 14;
			bpp = 2;
		break;
		case AM_OSD_FMT_COLOR_565:
			mode = 16;
			bpp = 2;
		break;
		case AM_OSD_FMT_COLOR_888:
			mode = 24;
			bpp = 3;
		break;
		case AM_OSD_FMT_COLOR_8888:
			mode = 32;
			bpp = 4;
		break;
		default:
			AM_DEBUG(1, "do not support format %d", para->format);
			return AM_OSD_ERR_NOT_SUPPORTED;
		break;
	}
	
	fb = (FBOSD_t*)malloc(sizeof(FBOSD_t));
	if(!fb)
	{
		AM_DEBUG(1, "not enough memory");
		ret = AM_OSD_ERR_NO_MEM;
		goto error;
	}
	
	memset(fb, 0, sizeof(FBOSD_t));
	fb->fd = -1;
	fb->id = dev->dev_no;
	
#ifndef ANDROID
	snprintf(fb->name, sizeof(fb->name), "/dev/fb%d", dev->dev_no);
#else
	snprintf(fb->name, sizeof(fb->name), "/dev/graphics/fb%d", dev->dev_no);

#endif
	fb->fd = open(fb->name, O_RDWR);
	if(!fb->fd)
	{
		AM_DEBUG(1, "cannot open frame buffer \"%s\"", strerror(errno));
		ret = AM_OSD_ERR_CANNOT_OPEN;
		goto error;
	}
	
	if(ioctl(fb->fd, FBIOGET_VSCREENINFO, &var)==-1)
	{
		AM_DEBUG(1, "cannot get vscreen info");
		ret = AM_OSD_ERR_SYS;
		goto error;
	}
	
	if(!para->enable_double_buffer)
	{
		output_w = para->width;
		output_h = para->height;
		var.xres_virtual = para->width;
		var.yres_virtual = para->height;
	}
	else
	{
		output_w = para->output_width;
		output_h = para->output_height; 
		var.xres_virtual = para->width+para->output_width;
		var.yres_virtual = AM_MAX(para->height, para->output_height);
	}
	
	var.xres         = para->output_width;
	var.yres         = para->output_height;
	var.bits_per_pixel = mode;

	
	if(ioctl(fb->fd, FBIOPUT_VSCREENINFO, &var)==-1)
	{
		AM_DEBUG(1, "cannot put vscreen info");
		ret = AM_OSD_ERR_SYS;
		goto error;
	}
	
	if(ioctl(fb->fd, FBIOGET_FSCREENINFO, &fix)==-1)
	{
		AM_DEBUG(1, "cannot get fscreen info");
		ret = AM_OSD_ERR_SYS;
		goto error;
	}
	
	fb->buf_len = fix.smem_len;
	fb->buf = mmap(0, fix.smem_len, PROT_WRITE|PROT_READ, MAP_SHARED, fb->fd, 0);
	if(!fb->buf)
	{
		AM_DEBUG(1, "cannot mmap frame buffer");
		ret = AM_OSD_ERR_SYS;
		goto error;
	}
	
	AM_DEBUG(1, "map frame buffer @ %p length:%d", fb->buf, fb->buf_len);
	
	flags = AM_OSD_SURFACE_FL_HW|AM_OSD_SURFACE_FL_EXTERNAL|AM_OSD_SURFACE_FL_OSD;
	
	pitch = fix.line_length;
	buf = fb->buf;
	if(para->enable_double_buffer)
	{
		buf += output_w*bpp;
		flags |= AM_OSD_SURFACE_FL_DBUF;
	}
	ret = AM_OSD_CreateSurfaceFromBuffer(para->format, para->width, para->height, pitch, buf,
		flags, &fb->draw_surf);
	if(ret!=AM_SUCCESS)
		goto error;
	fb->draw_surf->drv_data = dev;
	
	if(para->enable_double_buffer)
	{
		buf = fb->buf;
		flags = AM_OSD_SURFACE_FL_HW|AM_OSD_SURFACE_FL_EXTERNAL|AM_OSD_SURFACE_FL_OSD;
		
		ret = AM_OSD_CreateSurfaceFromBuffer(para->format, output_w, output_h, pitch, buf,
			flags, &fb->disp_surf);
		if(ret!=AM_SUCCESS)
			goto error;
		fb->disp_surf->drv_data = dev;
		
		*d = fb->disp_surf;
	}
	else
	{
		*d = fb->draw_surf;
	}
	
	dev->drv_data = fb;
	
	*s = fb->draw_surf;
	
#ifdef AMLINUX
	ge2d_set_coef(coef);
	fb_enable(dev, dev->enable);
#endif
	return ret;
error:
	if(fb)
	{
		if(fb->draw_surf)
			AM_OSD_DestroySurface(fb->draw_surf);
		if(fb->disp_surf)
			AM_OSD_DestroySurface(fb->disp_surf);
		if(fb->buf)
			munmap(fb->buf, fb->buf_len);
		if(fb->fd!=-1)
			close(fb->fd);
		
		free(fb);
	}
	return ret;
}

static AM_ErrorCode_t fb_enable (AM_OSD_Device_t *dev, AM_Bool_t enable)
{
#ifdef AMLINUX
	FBOSD_t *fb = (FBOSD_t*)dev->drv_data;
	char buf[32];
	char *cmd = enable?"0":"1";
	
	snprintf(buf, sizeof(buf), FB_BLANK_FILE, dev->dev_no);
	return AM_FileEcho(buf, cmd);
#else
	return AM_OSD_ERR_NOT_SUPPORTED;
#endif
}

static AM_ErrorCode_t fb_update (AM_OSD_Device_t *dev, AM_OSD_Rect_t *rect)
{
#ifdef AMLINUX
	FBOSD_t *fb = (FBOSD_t*)dev->drv_data;
	AM_OSD_Rect_t sr, dr;
	AM_OSD_BlitPara_t blit_para;
	
	if(!fb->disp_surf)
		return AM_SUCCESS;
	
	sr.x = 0;
	sr.y = 0;
	sr.w = fb->draw_surf->width;
	sr.h = fb->draw_surf->height;
	
	dr.x = 0;
	dr.y = 0;
	dr.w = dev->vout_w;
	dr.h = dev->vout_h;
	
	blit_para.enable_alpha  = AM_FALSE;
	blit_para.enable_key    = AM_FALSE;
	blit_para.enable_op     = AM_FALSE;
	blit_para.enable_global_alpha = AM_FALSE;
	
	return AM_OSD_Blit(fb->draw_surf, &sr, fb->disp_surf, &dr, &blit_para);
#else
	return AM_OSD_ERR_NOT_SUPPORTED;
#endif
}

static AM_ErrorCode_t fb_set_palette (AM_OSD_Device_t *dev, AM_OSD_Color_t *col, int start, int count)
{
	FBOSD_t *fb = (FBOSD_t*)dev->drv_data;
	struct fb_cmap cmap;
	__u16  red[256];
	__u16  green[256];
	__u16  blue[256];
	__u16  transp[256];
	int i, rc;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	cmap.start = start;
	cmap.len   = count;
	cmap.red   = red;
	cmap.green = green;
	cmap.blue  = blue;
	cmap.transp= transp;
	
	for(i=start; i<start+count; i++)
	{
		red[i]    = col[i].r;
		green[i]  = col[i].g;
		blue[i]   = col[i].b;
		transp[i] = col[i].a;
	}
	
	rc = ioctl(fb->fd, FBIOPUTCMAP, &cmap);
	if(rc)
	{
		AM_DEBUG(1, "set global alpha failed \"%s\"", strerror(errno));
		ret = AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	return ret;
}

static AM_ErrorCode_t fb_set_alpha (AM_OSD_Device_t *dev, uint8_t alpha)
{
#ifdef AMLINUX
	FBOSD_t *fb = (FBOSD_t*)dev->drv_data;
	uint32_t av = alpha;
	int rc;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	rc = ioctl(fb->fd, FBIOPUT_OSD_SET_GBL_ALPHA, &av);
	if(rc)
	{
		AM_DEBUG(1, "set global alpha failed \"%s\"", strerror(errno));
		ret = AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	return ret;
#else
	return AM_OSD_ERR_NOT_SUPPORTED;
#endif
}

static AM_ErrorCode_t fb_close (AM_OSD_Device_t *dev)
{
	FBOSD_t *fb = (FBOSD_t*)dev->drv_data;
	
	/*注销事件*/
	AM_EVT_Unsubscribe(dev->vout_dev_no, AM_VOUT_EVT_FORMAT_CHANGED, av_vout_format_changed, dev);
	
	if(fb->buf)
		munmap(fb->buf, fb->buf_len);
	
	if(fb->draw_surf)
		AM_OSD_DestroySurface(fb->draw_surf);
	
	if(fb->disp_surf)
		AM_OSD_DestroySurface(fb->disp_surf);
	
	close(fb->fd);
	free(fb);
	
	return AM_SUCCESS;
}

