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
 * \brief 图像文件加载模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-18: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_mem.h>
#include <am_debug.h>
#include <am_img.h>
#include <am_av.h>
#include <assert.h>
#include "image.h"

/****************************************************************************
 * Functions
 ***************************************************************************/

int GdImageBufferRead(buffer_t *src, void* buf, int size)
{
	if(src->size)
	{
		int ret = AM_MIN(size, src->size-src->offset);
		
		memcpy(buf, src->start+src->offset, ret);
		src->offset += ret;
		return ret;
	}
	else
	{
		return fread(buf, 1, size, (FILE*)src->start);
	}
}

int GdImageBufferGetChar(buffer_t *src)
{
	if(src->size)
	{
		int ret;
		
		if(src->offset<src->size)
		{
			ret = src->start[src->offset++];
			return ret;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return fgetc((FILE*)src->start);
	}
}

char* GdImageBufferGetString(buffer_t *src, void *buf, int size)
{
	if(src->size)
	{
		char *p = (char*)src->start+src->offset;
		char *d = buf;
		char *last = p+src->size;
		int left = size-1;
		
		if(p==last)
			return NULL;
		while((p!=last) && left)
		{
			*d++ = *p++;
			left--;
			if(p[-1]=='\n')
				break;
		}
		d[0] = 0;
		return buf;
	}
	else
	{
		return fgets(buf, size, (FILE*)src->start);
	}
}

int GdImageBufferSeekTo(buffer_t *src, int off)
{
	if(src->size)
	{
		off = AM_MAX(0, off);
		off = AM_MIN(off, src->size);
		src->offset = off;
		return off;
	}
	else
	{
		return fseek((FILE*)src->start, off, SEEK_SET);
	}
}

int GdComputeImagePitch(int bpp, int width, int *pitch, int *bytesperpixel)
{
	int pv, bv = 0, line;
	
	switch(bpp)
	{
		case 1:
			line = (width+7)/8;
			pv = (line+3)&~3;
		break;
		case 2:
			line = (width+3)/4;
			pv = (line+3)&~3;
		break;
		case 4:
			line = (width+1)/2;
			pv = (bv*width+3)&~3;
		break;
		case 8:
			bv = 1;
			pv = (bv*width+3)&~3;
		break;
		case 16:
			bv = 2;
			pv = (bv*width+3)&~3;
		break;
		case 24:
			bv = 3;
			pv = (bv*width+3)&~3;
		break;
		case 32:
			bv = 4;
			pv = (bv*width+3)&~3;
		break;
		default:
			return -1;
	}
	
	if(pitch)
		*pitch = pv;
	if(bytesperpixel)
		*bytesperpixel = bv;
	
	return 0;
}

AM_OSD_PixelFormatType_t GdGetPixelFormatType(int bpp, int bgr_mode)
{
	switch(bpp)
	{
		case 8:
			return AM_OSD_FMT_PALETTE_256;
		case 16:
			return bgr_mode?AM_OSD_FMT_COLOR_BGR_565:AM_OSD_FMT_COLOR_RGB_565;
		case 24:
			return bgr_mode?AM_OSD_FMT_COLOR_BGR_888:AM_OSD_FMT_COLOR_RGB_888;
		case 32:
			return bgr_mode?AM_OSD_FMT_COLOR_BGRA_8888:AM_OSD_FMT_COLOR_ARGB_8888;
		default:
			return -1;
	}
}

/****************************************************************************
 * API functions
 ***************************************************************************/

#ifdef IMG_BMP
/**\brief 加载BMP图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
AM_ErrorCode_t AM_IMG_LoadBMP(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img)
{
	FILE *fp;
	buffer_t src;
	int rc;
	extern int GdDecodeBMP(buffer_t *src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image);
	
	assert(name && img);
	
	fp = fopen(name, "r");
	if(!fp)
	{
		AM_DEBUG(1, "cannot open \"%s\"", name);
		return AM_IMG_ERR_CANNOT_OPEN_FILE;
	}
	
	src.size   = 0;
	src.offset = 0;
	src.start  = (unsigned char*)fp;
	rc = GdDecodeBMP(&src, para, img);
	if(rc!=1)
		AM_DEBUG(1, "cannot load BMP \"%s\"", name);
	
	fclose(fp);
	
	return (rc==1)?AM_SUCCESS:AM_IMG_ERR_FORMAT_MISMATCH;
}
#endif

#ifdef IMG_PNG
/**\brief 加载PNG图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
AM_ErrorCode_t AM_IMG_LoadPNG(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img)
{
	FILE *fp;
	buffer_t src;
	int rc;
	extern int GdDecodePNG(buffer_t * src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image);
	
	assert(name && img);
	
	fp = fopen(name, "r");
	if(!fp)
	{
		AM_DEBUG(1, "cannot open \"%s\"", name);
		return AM_IMG_ERR_CANNOT_OPEN_FILE;
	}
	
	src.size   = 0;
	src.offset = 0;
	src.start  = (unsigned char*)fp;
	rc = GdDecodePNG(&src, para, img);
	if(rc!=1)
		AM_DEBUG(1, "cannot load PNG \"%s\"", name);
		
	fclose(fp);
	
	return (rc==1)?AM_SUCCESS:AM_IMG_ERR_FORMAT_MISMATCH;
}
#endif

#ifdef IMG_GIF
/**\brief 加载GIF图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_gif.h)
 */
AM_ErrorCode_t AM_IMG_LoadGIF(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img)
{
	FILE *fp;
	buffer_t src;
	int rc;
	extern int GdDecodeGIF(buffer_t *src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image);
	
	assert(name && img);
	
	fp = fopen(name, "r");
	if(!fp)
	{
		AM_DEBUG(1, "cannot open \"%s\"", name);
		return AM_IMG_ERR_CANNOT_OPEN_FILE;
	}
	
	src.size   = 0;
	src.offset = 0;
	src.start  = (unsigned char*)fp;
	rc = GdDecodeGIF(&src, para, img);
	if(rc!=1)
		AM_DEBUG(1, "cannot load GIF \"%s\"", name);
	
	fclose(fp);
	
	return (rc==1)?AM_SUCCESS:AM_IMG_ERR_FORMAT_MISMATCH;
}
#endif

/**\brief 加载JPEG图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param flags 解码标志，AM_IMG_FL_HW表示用硬件进行解码
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
AM_ErrorCode_t AM_IMG_LoadJPEG(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img)
{
	AM_ErrorCode_t ret = AM_IMG_ERR_FORMAT_MISMATCH;
	
	assert(name && img);
	
	if(para && para->enable_hw)
	{
		AM_AV_SurfacePara_t s_para;
		
		s_para.angle  = AM_AV_JPEG_CLKWISE_0;
		s_para.option = 0;
		s_para.width  = 0;
		s_para.height = 0;
		
		ret = AM_AV_DecodeJPEG(para->hw_dev_no, name, &s_para, img);
	}
#ifdef IMG_JPEG
	if(ret!=AM_SUCCESS)
	{
		FILE *fp;
		buffer_t src;
		int rc;
		extern int GdDecodeJPEG(buffer_t* src, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image, int fast_grayscale);
		
		fp = fopen(name, "r");
		if(!fp)
		{
			AM_DEBUG(1, "cannot open \"%s\"", name);
			return AM_IMG_ERR_CANNOT_OPEN_FILE;
		}
	
		src.size   = 0;
		src.offset = 0;
		src.start  = (unsigned char*)fp;
		rc = GdDecodeJPEG(&src, para, img, 0);
		ret = (rc==1)?AM_SUCCESS:AM_IMG_ERR_FORMAT_MISMATCH;
		if(rc!=1)
			AM_DEBUG(1, "cannot load JPEG \"%s\"", name);
		
		fclose(fp);
	}
#endif

	return ret;
}

#ifdef IMG_TIFF
/**\brief 加载TIFF图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
AM_ErrorCode_t AM_IMG_LoadTIFF(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img)
{
	int rc;
	extern int GdDecodeTIFF(const char *path, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **image);
	
	assert(name && img);
	
	rc = GdDecodeTIFF(name, para, img);
	if(rc!=1)
		AM_DEBUG(1, "cannot load TIFF \"%s\"", name);
	
	return (rc==1)?AM_SUCCESS:AM_IMG_ERR_FORMAT_MISMATCH;
}
#endif

/**\brief 加载图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
AM_ErrorCode_t AM_IMG_Load(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img)
{
	char *ptr;
	
	assert(name && img);
	
	ptr = strrchr(name, '.');
	if(!ptr)
	{
		AM_DEBUG(1, "illegal image file name");
		return AM_IMG_ERR_FORMAT_MISMATCH;
	}
	
	ptr++;

#ifdef IMG_BMP
	if(!strcasecmp(ptr, "bmp") || !strcasecmp(ptr, "dib"))
	{
		return AM_IMG_LoadBMP(name, para, img);
	}
	else
#endif
#ifdef IMG_GIF
	if(!strcasecmp(ptr, "gif"))
	{
		return AM_IMG_LoadGIF(name, para, img);
	}
	else
#endif
#ifdef IMG_PNG
	if(!strcasecmp(ptr, "png"))
	{
		return AM_IMG_LoadPNG(name, para, img);
	}
	else
#endif
#ifdef IMG_TIFF
	if(!strcasecmp(ptr, "tiff") || !strcasecmp(ptr, "tif"))
	{
		return AM_IMG_LoadTIFF(name, para, img);
	}
	else
#endif
	if(!strcasecmp(ptr, "jpeg") || !strcasecmp(ptr, "jpg") || !strcasecmp(ptr, "jpe"))
	{
		return AM_IMG_LoadJPEG(name, para, img);
	}
	
	AM_DEBUG(1, "illegal image file name");
	return AM_IMG_ERR_FORMAT_MISMATCH;
}

