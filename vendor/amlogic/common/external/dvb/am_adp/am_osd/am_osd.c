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
 * \brief OSD模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-18: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <string.h>
#include <assert.h>
#include "am_osd_internal.h"
#include "../am_adp_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define OSD_DEV_COUNT      (2)

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef SDL_OSD
extern const AM_OSD_Driver_t sdl_osd_drv;
#else
extern const AM_OSD_Driver_t fb_osd_drv;
#endif

extern const AM_OSD_SurfaceOp_t sw_draw_op;
const AM_OSD_SurfaceOp_t *sw_surf_op = &sw_draw_op;

#ifdef SDL_OSD
const AM_OSD_SurfaceOp_t *hw_surf_op;
#else
extern const AM_OSD_SurfaceOp_t ge2d_draw_op;
const AM_OSD_SurfaceOp_t *hw_surf_op = &ge2d_draw_op;
#endif

const AM_OSD_SurfaceOp_t *opengl_surf_op;

#define OSD_GET_SURF_FUNC(flags, func, name)\
	AM_MACRO_BEGIN\
	func = NULL;\
	if((flags&AM_OSD_SURFACE_FL_OPENGL) && opengl_surf_op)\
		func = opengl_surf_op->name;\
	if(!func && (flags&AM_OSD_SURFACE_FL_HW) && hw_surf_op)\
		func = hw_surf_op->name;\
	if(!func && sw_surf_op)\
		func = sw_surf_op->name;\
	AM_MACRO_END

static AM_OSD_Device_t osd_devices[OSD_DEV_COUNT] =
{
#ifdef SDL_OSD
{
.drv = &sdl_osd_drv
},
{
.drv = &sdl_osd_drv
}
#else
{
.drv = &fb_osd_drv
},
{
.drv = &fb_osd_drv
}
#endif
};

/**\brief Openned counter*/
static int open_counter = 0;

/**\brief Pixel format types*/
static AM_OSD_PixelFormat_t osd_formats[AM_OSD_FMT_COUNT];

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief OSD release*/
static void osd_deinit(void)
{
	AM_OSD_PixelFormat_t *fmt = osd_formats;
	AM_OSD_PixelFormatType_t type;
	
	open_counter--;
	
	if(open_counter)
		return;
	
	if(opengl_surf_op && opengl_surf_op->deinit)
		opengl_surf_op->deinit();
	
	if(hw_surf_op && hw_surf_op->deinit)
		hw_surf_op->deinit();
	
	if(sw_surf_op && sw_surf_op->deinit)
		sw_surf_op->deinit();
	
	type = AM_OSD_FMT_PALETTE_256;
	if(fmt[type].palette.colors)
		free(fmt[type].palette.colors);
}

/**\brief OSD initialize*/
static AM_ErrorCode_t osd_init(void)
{
	AM_OSD_PixelFormat_t *fmt = osd_formats;
	AM_OSD_PixelFormatType_t type;
	AM_OSD_Color_t *pal;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	open_counter++;
	if(open_counter!=1)
		return 0;
	
	type = AM_OSD_FMT_PALETTE_256;
	fmt[type].type                = type;
	fmt[type].bits_per_pixel      = 8;
	fmt[type].bytes_per_pixel     = 1;
	fmt[type].palette.color_count = 256;
	pal = (AM_OSD_Color_t*)malloc(sizeof(AM_OSD_Color_t)*fmt[type].palette.color_count);
	if(!pal)
	{
		AM_DEBUG(1, "not enough memory");
		ret = AM_OSD_ERR_NO_MEM;
		goto error;
	}
	memset(pal, 0, sizeof(AM_OSD_Color_t)*fmt[type].palette.color_count);
	fmt[type].palette.colors = pal;

#define FMT_BIT_MASK(am,ao,as,rm,ro,rs,gm,go,gs,bm,bo,bs)\
	AM_MACRO_BEGIN\
	fmt[type].a_mask  = am;\
	fmt[type].a_offset= ao;\
	fmt[type].a_shift = as;\
	fmt[type].r_mask  = rm;\
	fmt[type].r_offset= ro;\
	fmt[type].r_shift = rs;\
	fmt[type].g_mask  = gm;\
	fmt[type].g_offset= go;\
	fmt[type].g_shift = gs;\
	fmt[type].b_mask  = bm;\
	fmt[type].b_offset= bo;\
	fmt[type].b_shift = bs;\
	AM_MACRO_END

#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_ARGB_4444;
#else
	type = AM_OSD_FMT_COLOR_BGRA_4444;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 16;
	fmt[type].bytes_per_pixel     = 2;
	FMT_BIT_MASK(0xF000,12,4,0x0F00,8,4,0x00F0,4,4,0x000F,0,4);

#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_BGRA_4444;
#else
	type = AM_OSD_FMT_COLOR_ARGB_4444;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 16;
	fmt[type].bytes_per_pixel     = 2;
	FMT_BIT_MASK(0x000F,0,4,0x00F0,4,4,0x0F00,8,4,0xF000,12,4);

#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_ARGB_1555;
#else
	type = AM_OSD_FMT_COLOR_BGRA_5551;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 16;
	fmt[type].bytes_per_pixel     = 2;
	FMT_BIT_MASK(0x8000,15,7,0x7C00,10,3,0x03E0,5,3,0x001F,0,3);

#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_BGRA_5551;
#else
	type = AM_OSD_FMT_COLOR_ARGB_1555;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 16;
	fmt[type].bytes_per_pixel     = 2;
	FMT_BIT_MASK(0x0001,0,7,0x003E,1,3,0x07C0,6,3,0xF800,11,3);
	
#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_RGB_565;
#else
	type = AM_OSD_FMT_COLOR_BGR_565;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 16;
	fmt[type].bytes_per_pixel     = 2;
	FMT_BIT_MASK(0x0000,0,0,0xF800,11,3,0x07E0,5,2,0x001F,0,3);
	
#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_BGR_565;
#else
	type = AM_OSD_FMT_COLOR_RGB_565;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 16;
	fmt[type].bytes_per_pixel     = 2;
	FMT_BIT_MASK(0x0000,0,0,0x001F,0,3,0x07E0,5,2,0xF800,11,3);
	
#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_RGB_888;
#else
	type = AM_OSD_FMT_COLOR_BGR_888;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 24;
	fmt[type].bytes_per_pixel     = 3;
	FMT_BIT_MASK(0x000000,0,0,0xFF0000,16,0,0x00FF00,8,0,0x0000FF,0,0);
	
#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_BGR_888;
#else
	type = AM_OSD_FMT_COLOR_RGB_888;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 24;
	fmt[type].bytes_per_pixel     = 3;
	FMT_BIT_MASK(0x000000,0,0,0x0000FF,0,0,0x00FF00,8,0,0xFF0000,16,0);
	
#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_ARGB_8888;
#else
	type = AM_OSD_FMT_COLOR_BGRA_8888;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 32;
	fmt[type].bytes_per_pixel     = 4;
	FMT_BIT_MASK(0xFF000000,24,0,0x00FF0000,16,0,0x0000FF00,8,0,0x000000FF,0,0);

#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_BGRA_8888;
#else
	type = AM_OSD_FMT_COLOR_ARGB_8888;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 32;
	fmt[type].bytes_per_pixel     = 4;
	FMT_BIT_MASK(0x000000FF,0,0,0x0000FF00,8,0,0x00FF0000,16,0,0xFF000000,24,0);

#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_ABGR_8888;
#else
	type = AM_OSD_FMT_COLOR_RGBA_8888;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 32;
	fmt[type].bytes_per_pixel     = 4;
	FMT_BIT_MASK(0xFF000000,24,0,0x000000FF,0,0,0x0000FF00,8,0,0x00FF0000,16,0);

#if __BYTE_ORDER == __BIG_ENDIAN
	type = AM_OSD_FMT_COLOR_RGBA_8888;
#else
	type = AM_OSD_FMT_COLOR_ABGR_8888;
#endif
	fmt[type].type                = type;
	fmt[type].planes              = 1;
	fmt[type].bits_per_pixel      = 32;
	fmt[type].bytes_per_pixel     = 4;
	FMT_BIT_MASK(0x000000FF,0,0,0xFF000000,24,0,0x00FF0000,16,0,0x0000FF00,8,0);

	type = AM_OSD_FMT_YUV_420;
	fmt[type].type                = type;
	fmt[type].planes              = 3;
	fmt[type].bits_per_pixel      = 8;
	fmt[type].bytes_per_pixel     = 1;
	
	type = AM_OSD_FMT_YUV_422;
	fmt[type].type                = type;
	fmt[type].planes              = 3;
	fmt[type].bits_per_pixel      = 8;
	fmt[type].bytes_per_pixel     = 1;
	
	type = AM_OSD_FMT_YUV_444;
	fmt[type].type                = type;
	fmt[type].planes              = 3;
	fmt[type].bits_per_pixel      = 8;
	fmt[type].bytes_per_pixel     = 1;
	
	if(sw_surf_op && sw_surf_op->init)
		sw_surf_op->init();
	
	if(hw_surf_op && hw_surf_op->init)
		hw_surf_op->init();
	
	if(opengl_surf_op && opengl_surf_op->init)
		opengl_surf_op->init();
	
	return AM_SUCCESS;
error:
	osd_deinit();
	return ret;
}

/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t osd_get_dev(int dev_no, AM_OSD_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=OSD_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid osd device number %d, must in(%d~%d)", dev_no, 0, OSD_DEV_COUNT-1);
		return AM_OSD_ERR_INVALID_DEV_NO;
	}
	
	*dev = &osd_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t osd_get_openned_dev(int dev_no, AM_OSD_Device_t **dev)
{
	AM_TRY(osd_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "osd device %d has not been openned", dev_no);
		return AM_OSD_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 初始化OSD模块(不打开OSD设备，只初始化相关资源)
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Init(void)
{
	pthread_mutex_lock(&am_gAdpLock);
	osd_init();
	pthread_mutex_unlock(&am_gAdpLock);

	return AM_SUCCESS;
}

/**\brief 释放OSD模块
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Quit(void)
{
	pthread_mutex_lock(&am_gAdpLock);
	osd_deinit();
	pthread_mutex_unlock(&am_gAdpLock);

	return AM_SUCCESS;
}

/**\brief 打开一个OSD设备
 * \param dev_no OSD设备号
 * \param[in] para 设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Open(int dev_no, AM_OSD_OpenPara_t *para)
{
	AM_OSD_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(osd_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	/*Initialize the global data*/
	osd_init();
	
	if(dev->openned)
	{
		AM_DEBUG(1, "osd device already openned");
		ret = AM_OSD_ERR_BUSY;
		osd_deinit();
	}
	
	dev->dev_no      = dev_no;
	dev->vout_dev_no = para->vout_dev_no;
	dev->surface     = NULL;
	dev->display     = NULL;
	dev->enable      = AM_TRUE;
	
	if(ret==AM_SUCCESS)
	{
		dev->dev_no = dev_no;
		
		if(dev->drv->open)
		{
			ret = dev->drv->open(dev, para, &dev->surface, &dev->display);
			if(ret!=AM_SUCCESS)
				osd_deinit();
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		dev->openned = AM_TRUE;
		dev->para    = *para;
		dev->enable_double_buffer = para->enable_double_buffer;
	}
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭一个OSD设备
 * \param dev_no OSD设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Close(int dev_no)
{
	AM_OSD_Device_t *dev;
	
	AM_TRY(osd_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->drv->close)
	{
		dev->drv->close(dev);
	}
	
	dev->openned = AM_FALSE;
	osd_deinit();
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return AM_SUCCESS;
}

/**\brief 取得OSD设备的绘图表面
 * \param dev_no OSD设备号
 * \param[out] s 返回OSD绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_GetSurface(int dev_no, AM_OSD_Surface_t **s)
{
	AM_OSD_Device_t *dev;
	
	assert(s);
	
	AM_TRY(osd_get_openned_dev(dev_no, &dev));
	
	*s = dev->surface;
	return AM_SUCCESS;
}

/**\brief 取得OSD设备的显示绘图表面，如果OSD没有使用双缓冲，则返回的Surface和AM_OSD_GetSurface的返回值相同
 * \param dev_no OSD设备号
 * \param[out] s 返回OSD绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_GetDisplaySurface(int dev_no, AM_OSD_Surface_t **s)
{
	AM_OSD_Device_t *dev;
	
	assert(s);
	
	AM_TRY(osd_get_openned_dev(dev_no, &dev));
	
	*s = dev->display;
	return AM_SUCCESS;
}

/**\brief 显示OSD层
 * \param dev_no OSD设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Enable(int dev_no)
{
	AM_OSD_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(osd_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->enable)
	{
		AM_DEBUG(1, "do not support enable");
		ret = AM_OSD_ERR_NOT_SUPPORTED;
	}

	ret = dev->drv->enable(dev, AM_TRUE);
	if(ret==AM_SUCCESS)
	{
		dev->enable = AM_TRUE;
	}

	return ret;
}

/**\brief 不显示OSD层
 * \param dev_no OSD设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Disable(int dev_no)
{
	AM_OSD_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(osd_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->enable)
	{
		AM_DEBUG(1, "do not support enable");
		ret = AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	ret = dev->drv->enable(dev, AM_FALSE);
	if(ret==AM_SUCCESS)
	{
		dev->enable = AM_FALSE;
	}
	
	return ret;
}

/**\brief 双缓冲模式下，将缓冲区中的数据显示到OSD中
 * \param dev_no OSD设备号
 * \param[in] rect 要刷新的区域，NULL表示整个OSD
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Update(int dev_no, AM_OSD_Rect_t *rect)
{
	AM_OSD_Device_t *dev;
		
	AM_TRY(osd_get_openned_dev(dev_no, &dev));
	
	if(dev->drv->update)
	{
		AM_OSD_Rect_t ur;
		
		if(!rect)
		{
			rect = &ur;
			ur.x = 0;
			ur.y = 0;
			ur.w = dev->surface->width;
			ur.h = dev->surface->height;
		}
		
		dev->drv->update(dev, rect);
	}
	
	return AM_SUCCESS;
}

/**\brief 设定OSD的全局Alpha值
 * \param dev_no OSD设备号
 * \param alpha Alpha值(0~255)
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_SetGlobalAlpha(int dev_no, uint8_t alpha)
{
	AM_OSD_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
		
	AM_TRY(osd_get_openned_dev(dev_no, &dev));
	
	if(!dev->drv->set_alpha)
	{
		AM_DEBUG(1, "do not support set_alpha");
		ret = AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
	{
		ret = dev->drv->set_alpha(dev, alpha);
	}
	
	return ret;
}

/**\brief 取得图形模式相关参数
 * \param type 图形模式类型
 * \param[out] fmt 返回图形模式相关参数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_GetFormatPara(AM_OSD_PixelFormatType_t type, AM_OSD_PixelFormat_t **fmt)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	assert(fmt);
	
	if(type<0 || type>=AM_OSD_FMT_COUNT)
	{
		ret = AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	if(ret==AM_SUCCESS)
	{
		*fmt = &osd_formats[type];
	}
	
	return ret;
}

/**\brief 将颜色映射为像素值
 * \param[in] fmt 图形模式 
 * \param[in] col 颜色值
 * \param[out] pix 返回映射的像素值 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_MapColor(AM_OSD_PixelFormat_t *fmt, AM_OSD_Color_t *col, uint32_t *pix)
{
	assert(fmt && col && pix);
	
	if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(fmt->type))
	{
		AM_OSD_Color_t *pal = fmt->palette.colors;
		int i, cid=0;
		uint32_t diff, min=INT32_MAX;
		
		for(i=0; i<fmt->palette.color_count; i++)
		{
			diff = AM_ABS(col->r-pal->r)+AM_ABS(col->g-pal->g)+AM_ABS(col->b-pal->b)+AM_ABS(col->a-pal->a);
			if(!diff)
			{
				*pix = i;
				break;
			}
			if(diff<min)
			{
				min = diff;
				cid = i;
			}
		}
		
		*pix = cid;
	}
	else if(AM_OSD_PIXEL_FORMAT_TYPE_IS_YUV(fmt->type))
	{
		AM_DEBUG(1, "cannot map color to YUV pixel");
		return AM_OSD_ERR_NOT_SUPPORTED;
	}
	else
	{
		*pix = AM_COLOR_TO_RGB_PIXEL(fmt, col);
	}
	
	return AM_SUCCESS;
}

/**\brief 将像素映射为颜色
 * \param[in] fmt 图形模式 
 * \param[in] pix 像素值
 * \param[out] col 返回映射的颜色值 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_MapPixel(AM_OSD_PixelFormat_t *fmt, uint32_t pix, AM_OSD_Color_t *col)
{
	assert(fmt && col);
	
	if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(fmt->type))
	{
		if(pix>=fmt->palette.color_count)
		{
			AM_DEBUG(1, "illegal pixel, must in (0~%d)", fmt->palette.color_count-1);
			return AM_OSD_ERR_ILLEGAL_PIXEL;
		}
		
		*col = fmt->palette.colors[pix];
	}
	else if(AM_OSD_PIXEL_FORMAT_TYPE_IS_YUV(fmt->type))
	{
		AM_DEBUG(1, "cannot map YUV pixel to color");
		return AM_OSD_ERR_NOT_SUPPORTED;
	}
	else
	{
		AM_RGB_PIXEL_TO_COLOR(fmt, pix, col);
	}
	
	return AM_SUCCESS;
}

/**\brief 创建一个绘图表面
 * \param type 图形模式 
 * \param w 绘图表面宽度
 * \param h 绘图表面高度
 * \param flags 绘图表面的属性标志
 * \param[out] s 返回创建的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_CreateSurface(AM_OSD_PixelFormatType_t type, int w, int h, uint32_t flags, AM_OSD_Surface_t **s)
{
	AM_OSD_Surface_t *surf = NULL;
	AM_OSD_PixelFormat_t *fmt = NULL;
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s);
	AM_ErrorCode_t ret = AM_FAILURE;
	uint32_t real_flags = 0;
	
	assert(s);
	
	if((type<0) || (type>=AM_OSD_FMT_COUNT))
	{
		AM_DEBUG(1, "format type no dot support");
		return AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	/*Allocate memory*/
	surf = (AM_OSD_Surface_t*)malloc(sizeof(AM_OSD_Surface_t));
	if(!surf)
	{
		AM_DEBUG(1, "not enough memory");
		ret = AM_OSD_ERR_NO_MEM;
		goto error;
	}
	
	memset(surf, 0, sizeof(AM_OSD_Surface_t));
	
	if(flags&AM_OSD_SURFACE_FL_PRIV_FMT)
	{
		fmt = (AM_OSD_PixelFormat_t*)malloc(sizeof(AM_OSD_PixelFormat_t));
		if(!fmt)
		{
			AM_DEBUG(1, "not enough memory");
			ret = AM_OSD_ERR_NO_MEM;
			goto error;
		}
		*fmt = osd_formats[type];
		if(fmt->palette.color_count)
		{
			fmt->palette.colors = (AM_OSD_Color_t*)malloc(sizeof(AM_OSD_Color_t)*fmt->palette.color_count);
			if(!fmt->palette.colors)
			{
				AM_DEBUG(1, "not enough memory");
				ret = AM_OSD_ERR_NO_MEM;
				goto error;
			}
		}
		surf->format = fmt;
	}
	else
	{
		surf->format = &osd_formats[type];
	}
	surf->width  = w;
	surf->height = h;
	surf->clip.x = 0;
	surf->clip.y = 0;
	surf->clip.w = w;
	surf->clip.h = h;
	surf->color_key = 0;
	surf->alpha     = 0xFF;
	
	/*Invoke create surface function*/
	func = NULL;
	if((flags&AM_OSD_SURFACE_FL_OPENGL) && opengl_surf_op)
	{
		func = opengl_surf_op->create_surface;
		if(func)
		{
			ret = func(surf);
			if(ret==AM_SUCCESS)
				real_flags = AM_OSD_SURFACE_FL_OPENGL;
		}
	}
	
	if((ret!=AM_SUCCESS) && (flags&AM_OSD_SURFACE_FL_HW) && hw_surf_op)
	{
		func = hw_surf_op->create_surface;
		if(func)
		{
			ret = func(surf);
			if(ret==AM_SUCCESS)
				real_flags = AM_OSD_SURFACE_FL_HW;
		}
	}
	
	if((ret!=AM_SUCCESS) && sw_surf_op)
	{
		func = sw_surf_op->create_surface;
		if(func)
		{
			ret = func(surf);
		}
	}
	
	if(ret!=AM_SUCCESS)
		goto error;
	
	surf->flags = real_flags|(flags&~(AM_OSD_SURFACE_FL_OPENGL|AM_OSD_SURFACE_FL_HW));
	
	*s = surf;
	return AM_SUCCESS;
error:
	if(surf)
		free(surf);
	if(fmt)
	{
		if(fmt->palette.colors)
			free(fmt->palette.colors);
		free(fmt);
	}
	return ret;
}

/**\brief 从一个缓冲区创建一个绘图表面
 * \param type 图形模式 
 * \param w 绘图表面宽度
 * \param h 绘图表面高度
 * \param pitch 两行之间的字节数
 * \param buffer 缓冲区指针
 * \param flags 绘图表面的属性标志
 * \param[out] s 返回创建的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_CreateSurfaceFromBuffer(AM_OSD_PixelFormatType_t type, int w, int h, int pitch,
		uint8_t *buffer, uint32_t flags, AM_OSD_Surface_t **s)
{
	AM_OSD_Surface_t *surf = NULL;
	AM_OSD_PixelFormat_t *fmt = NULL;
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s);
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint32_t real_flags = 0;
	
	assert(s);
	
	if((type<0) || (type>=AM_OSD_FMT_COUNT))
	{
		AM_DEBUG(1, "format type no dot support");
		return AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	/*Allocate memory*/
	surf = (AM_OSD_Surface_t*)malloc(sizeof(AM_OSD_Surface_t));
	if(!surf)
	{
		AM_DEBUG(1, "not enough memory");
		ret = AM_OSD_ERR_NO_MEM;
		goto error;
	}
	
	memset(surf, 0, sizeof(AM_OSD_Surface_t));
	
	if(flags&AM_OSD_SURFACE_FL_PRIV_FMT)
	{
		fmt = (AM_OSD_PixelFormat_t*)malloc(sizeof(AM_OSD_PixelFormat_t));
		if(!fmt)
		{
			AM_DEBUG(1, "not enough memory");
			ret = AM_OSD_ERR_NO_MEM;
			goto error;
		}
		*fmt = osd_formats[type];
		if(fmt->palette.color_count)
		{
			fmt->palette.colors = (AM_OSD_Color_t*)malloc(sizeof(AM_OSD_Color_t)*fmt->palette.color_count);
			if(!fmt->palette.colors)
			{
				AM_DEBUG(1, "not enough memory");
				ret = AM_OSD_ERR_NO_MEM;
				goto error;
			}
		}
		surf->format = fmt;
	}
	else
	{
		surf->format = &osd_formats[type];
	}
	
	surf->width  = w;
	surf->height = h;
	surf->clip.x = 0;
	surf->clip.y = 0;
	surf->clip.w = w;
	surf->clip.h = h;
	surf->pitch  = pitch;
	surf->buffer = buffer;
	surf->color_key = 0;
	surf->alpha     = 0xFF;
	
	/*Invoke create surface function*/
	func = NULL;
	if((flags&AM_OSD_SURFACE_FL_OPENGL) && opengl_surf_op)
	{
		func = opengl_surf_op->create_surface;
		if(func) real_flags = AM_OSD_SURFACE_FL_OPENGL;
	}
	
	if(!func && (flags&AM_OSD_SURFACE_FL_HW) && hw_surf_op)
	{
		func = hw_surf_op->create_surface;
		if(func) real_flags = AM_OSD_SURFACE_FL_HW;
	}
	
	if(!func && sw_surf_op)
		func = sw_surf_op->create_surface;
	
	if(func)
	{
		ret = func(surf);
		if(ret!=AM_SUCCESS)
		{
			goto error;
		}
	}
	
	surf->flags = real_flags|(flags&~(AM_OSD_SURFACE_FL_OPENGL|AM_OSD_SURFACE_FL_HW));
	*s = surf;
	return AM_SUCCESS;
error:
	if(surf)
		free(surf);
	if(fmt)
	{
		if(fmt->palette.colors)
			free(fmt->palette.colors);
		free(fmt);
	}
	return ret;
}

/**\brief 销毁一个绘图表面
 * \param[in,out] s 要销毁的绘图表面 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_DestroySurface(AM_OSD_Surface_t *s)
{
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s);
	
	assert(s);
	
	func = NULL;
	if(s->flags&AM_OSD_SURFACE_FL_OPENGL)
	{
		if(opengl_surf_op)
			func = opengl_surf_op->destroy_surface;
	}
	else if(s->flags&AM_OSD_SURFACE_FL_HW)
	{
		if(hw_surf_op)
			func = hw_surf_op->destroy_surface;
	}
	else
	{
		if(sw_surf_op)
			func = sw_surf_op->destroy_surface;
	}
	
	if(func)
		func(s);
	
	if(s->flags&AM_OSD_SURFACE_FL_PRIV_FMT)
	{
		if(s->format)
		{
			if(s->format->palette.colors)
				free(s->format->palette.colors);
			free(s->format);
		}
	}
	
	free(s);
	return AM_SUCCESS;
}

/**\brief 设定绘图剪切区，绘图只对剪切区内部的像素生效
 * \param[in] s 绘图表面
 * \param[in] clip 剪切区矩形,NULL表示取消剪切区
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_SetClip(AM_OSD_Surface_t *surf, AM_OSD_Rect_t *clip)
{
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s, AM_OSD_Rect_t *clip);
	
	assert(surf);
	
	if(clip)
	{
		surf->clip = *clip;
	}
	else
	{
		surf->clip.x = 0;
		surf->clip.y = 0;
		surf->clip.w = surf->width;
		surf->clip.h = surf->height;
	}
	
	OSD_GET_SURF_FUNC(surf->flags, func, set_clip);
	if(func)
		func(surf, &surf->clip);
	
	return AM_SUCCESS;
}

/**\brief 设定调色板，此操作只对调色板模式的表面有效
 * \param[in] s 绘图表面
 * \param[in] pal 设定颜色数组
 * \param start 要设定的调色板起始项
 * \param count 要设定的颜色数目
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_SetPalette(AM_OSD_Surface_t *surf, AM_OSD_Color_t *pal, int start, int count)
{
	AM_OSD_PixelFormat_t *fmt;
	int first, last;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(surf && pal);
	
	fmt = surf->format;
	if(!AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(fmt->type))
	{
		AM_DEBUG(1, "not a palette pixel format");
		return AM_OSD_ERR_NOT_SUPPORTED;
	}
	
	first = AM_MAX(0, start);
	last  = AM_MIN(fmt->palette.color_count, start+count);
	
	start = first;
	count = last-first;
	
	if(count)
	{
		memcpy(fmt->palette.colors+start, pal, count*sizeof(AM_OSD_Color_t));
		
		if(surf->flags&AM_OSD_SURFACE_FL_OSD)
		{
			AM_OSD_Device_t *dev = (AM_OSD_Device_t*)surf->drv_data;
			
			if(!dev->drv->set_palette)
			{
				AM_DEBUG(1, "do not support set_palette");
				return AM_OSD_ERR_NOT_SUPPORTED;
			}
			
			ret = dev->drv->set_palette(dev, pal, start, count);
		}
	}
	
	return ret;
}

/**\brief 在绘图表面上画点
 * \param[in] s 绘图表面
 * \param x 点的X坐标
 * \param y 点的Y坐标
 * \param pix 点的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_DrawPixel(AM_OSD_Surface_t *surf, int x, int y, uint32_t pix)
{
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s, int x, int y, uint32_t pix);
	
	assert(surf);
	
	func = NULL;
	
	if(sw_surf_op)
		func = sw_surf_op->pixel;
	
	if(func)
		func(surf, x, y, pix);
	
	return AM_SUCCESS;
}

/**\brief 在绘图表面上画水平线
 * \param[in] s 绘图表面
 * \param x 左顶点的X坐标
 * \param y 左顶点的Y坐标
 * \param w 水平线宽度
 * \param pix 直线的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_DrawHLine(AM_OSD_Surface_t *surf, int x, int y, int w, uint32_t pix)
{
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s, int x, int y, int w, uint32_t pix);
	
	assert(surf);
	
	OSD_GET_SURF_FUNC(surf->flags, func, hline);
	
	if(func)
		func(surf, x, y, w, pix);
	
	return AM_SUCCESS;
}

/**\brief 在绘图表面上画垂直线
 * \param[in] s 绘图表面
 * \param x 上顶点的X坐标
 * \param y 上顶点的Y坐标
 * \param h 垂直线高度
 * \param pix 直线的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_DrawVLine(AM_OSD_Surface_t *surf, int x, int y, int h, uint32_t pix)
{
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s, int x, int y, int h, uint32_t pix);
	
	assert(surf);
	
	OSD_GET_SURF_FUNC(surf->flags, func, vline);
	
	if(func)
		func(surf, x, y, h, pix);
	
	return AM_SUCCESS;
}

/**\brief 在绘图表面上画矩形
 * \param[in] s 绘图表面
 * \param[in] rect 矩形区 
 * \param pix 矩形边的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_DrawRect(AM_OSD_Surface_t *surf, AM_OSD_Rect_t *rect, uint32_t pix)
{
	AM_OSD_Rect_t real_rect;
	
	assert(surf);
	
	if(!rect)
	{
		rect = &real_rect;
		rect->x = 0;
		rect->y = 0;
		rect->w = surf->width;
		rect->h = surf->height;
	}
	
	AM_OSD_DrawHLine(surf, rect->x, rect->y, rect->w, pix);
	AM_OSD_DrawHLine(surf, rect->x, rect->y+rect->h-1, rect->w, pix);
	AM_OSD_DrawVLine(surf, rect->x, rect->y, rect->h, pix);
	AM_OSD_DrawVLine(surf, rect->x+rect->w-1, rect->y, rect->h, pix);
	
	return AM_SUCCESS;
}

/**\brief 在绘图表面上画填充矩形
 * \param[in] s 绘图表面
 * \param[in] rect 矩形区 
 * \param pix 填充像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_DrawFilledRect(AM_OSD_Surface_t *surf, AM_OSD_Rect_t *rect, uint32_t pix)
{
	AM_OSD_Rect_t real_rect;
	
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, uint32_t pix);
	
	assert(surf);
	
	if(!rect)
	{
		rect = &real_rect;
		rect->x = 0;
		rect->y = 0;
		rect->w = surf->width;
		rect->h = surf->height;
	}
	
	if((rect->w>=30) && (rect->h>=30))
	{
		OSD_GET_SURF_FUNC(surf->flags, func, filled_rect);
	}
	else
	{
		func = sw_surf_op->filled_rect;
	}
	
	if(func)
		func(surf, rect, pix);
	
	return AM_SUCCESS;
}

/**\brief Blit操作
 * \param[in] src 源绘图表面
 * \param[in] sr 源矩形区 
 * \param[in] dst 目标绘图表面
 * \param[in] dr 目标矩形区
 * \param[in] para Blit参数,如果para==NULL,将根据src的flags标志设定参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_osd.h)
 */
AM_ErrorCode_t AM_OSD_Blit(AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para)
{
	AM_OSD_Rect_t srect, drect;
	AM_OSD_BlitPara_t real_para;
	uint32_t flags;
	AM_ErrorCode_t (*func) (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para);
	
	assert(src && dst);
	
	if(!sr)
	{
		sr = &srect;
		sr->x = 0;
		sr->y = 0;
		sr->w = src->width;
		sr->h = src->height;
	}
	
	if(!dr)
	{
		dr = &drect;
		dr->x = 0;
		dr->y = 0;
		dr->w = dst->width;
		dr->h = dst->height;
	}
	
	if(!dr->w || !dr->h || !sr->w || !sr->h)
		return AM_SUCCESS;
	
	if(!para)
	{
		para = &real_para;
		if(src->flags&AM_OSD_SURFACE_FL_GLOBAL_ALPHA)
		{
			real_para.enable_global_alpha = AM_TRUE;
			real_para.alpha        = src->alpha;
		}
		else
			real_para.enable_global_alpha = AM_FALSE;
		
		real_para.enable_alpha = (src->flags&AM_OSD_SURFACE_FL_ALPHA)?AM_TRUE:AM_FALSE;
		real_para.enable_op    = AM_FALSE;
		
		if(src->flags&AM_OSD_SURFACE_FL_COLOR_KEY)
		{
			real_para.enable_key = AM_TRUE;
			real_para.key = src->color_key;
		}
		else
			real_para.enable_key = AM_FALSE;
	}
	
	flags = src->flags & dst->flags;
	
	func = NULL;
	if((flags&AM_OSD_SURFACE_FL_OPENGL) && opengl_surf_op)
		func = opengl_surf_op->blit;
	if(!func && (flags&AM_OSD_SURFACE_FL_HW) && hw_surf_op &&
			(sr->w/dr->w<15 || sr->h/dr->h<15))
		func = hw_surf_op->blit;
	if(!func && sw_surf_op)
		func = sw_surf_op->blit;
	
	if(func)
		func(src, sr, dst, dr, para);
	
	return AM_SUCCESS;
}
