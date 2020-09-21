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
 * \brief 软件绘图操作
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-19: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_mem.h>
#include <am_thread.h>
#include "am_osd_internal.h"

/****************************************************************************
 * Static data
 ***************************************************************************/

static AM_ErrorCode_t sw_create_surface (AM_OSD_Surface_t *s);
static AM_ErrorCode_t sw_destroy_surface (AM_OSD_Surface_t *s);
static AM_ErrorCode_t sw_pixel (AM_OSD_Surface_t *s, int x, int y, uint32_t pix);
static AM_ErrorCode_t sw_hline(AM_OSD_Surface_t *s, int x, int y, int w, uint32_t pix);
static AM_ErrorCode_t sw_vline(AM_OSD_Surface_t *s, int x, int y, int h, uint32_t pix);
static AM_ErrorCode_t sw_filled_rect (AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, uint32_t pix);
static AM_ErrorCode_t sw_blit (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para);

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

const AM_OSD_SurfaceOp_t sw_draw_op = {
.create_surface  = sw_create_surface,
.destroy_surface = sw_destroy_surface,
.pixel        = sw_pixel,
.hline        = sw_hline,
.vline        = sw_vline,
.filled_rect  = sw_filled_rect,
.blit         = sw_blit
};

#define MAX_W    1920
static int convx[MAX_W];

/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t sw_create_surface (AM_OSD_Surface_t *s)
{
	if(!s->buffer)
	{
		int size;
		
		if(AM_OSD_PIXEL_FORMAT_TYPE_IS_YUV(s->format->type))
			return AM_OSD_ERR_NOT_SUPPORTED;
		
		if(!s->pitch)
			s->pitch = ((s->format->bytes_per_pixel*s->width)+3)&~3;
		
		size = s->pitch*s->height;
		
		s->buffer = (uint8_t*)malloc(size);
		if(!s->buffer)
		{
			AM_DEBUG(1, "not enough memory");
			s->pitch = 0;
			return AM_OSD_ERR_NO_MEM;
		}
	}
	
	return AM_SUCCESS; 
}

static AM_ErrorCode_t sw_destroy_surface (AM_OSD_Surface_t *s)
{
	if(s->buffer && !(s->flags&AM_OSD_SURFACE_FL_EXTERNAL))
	{
		free(s->buffer);
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sw_pixel (AM_OSD_Surface_t *s, int x, int y, uint32_t pix)
{
	if(AM_OSD_ClipPoint(&s->clip, x, y))
	{
		uint8_t *ptr = s->buffer+s->pitch*y+x*s->format->bytes_per_pixel;
		
		switch(s->format->bytes_per_pixel)
		{
			case 1:
				*ptr = pix;
			break;
			case 2:
				*(uint16_t*)ptr = pix;
			break;
			case 3:
				AM_PUT_3BPP_PIXEL(ptr,pix);
				ptr += 3;
			break;
			case 4:
				*(uint32_t*)ptr = pix;
			break;
			default:
			break;
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sw_vline(AM_OSD_Surface_t *s, int x, int y, int h, uint32_t pix)
{
	if(AM_OSD_ClipVLine(&s->clip, x, y, h, &y, &h))
	{
		uint8_t *ptr = s->buffer+s->pitch*y+x*s->format->bytes_per_pixel;
		
		switch(s->format->bytes_per_pixel)
		{
			case 1:
				while(h--)
				{
					*ptr = pix;
					ptr += s->pitch;
				}
			break;
			case 2:
				while(h--)
				{
					*(uint16_t*)ptr = pix;
					ptr += s->pitch;
				}
			break;
			case 3:
				while(h--)
				{
					AM_PUT_3BPP_PIXEL(ptr,pix);
					ptr += s->pitch;
				}
			break;
			case 4:
				while(h--)
				{
					*(uint32_t*)ptr = pix;
					ptr += s->pitch;
				}
			break;
			default:
			break;
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sw_hline(AM_OSD_Surface_t *s, int x, int y, int w, uint32_t pix)
{
	if(AM_OSD_ClipHLine(&s->clip, x, y, w, &x, &w))
	{
		uint8_t *ptr = s->buffer+s->pitch*y+x*s->format->bytes_per_pixel;
		
		switch(s->format->bytes_per_pixel)
		{
			case 1:
				memset(ptr, pix, w);
			break;
			case 2:
				while(w--)
				{
					*(uint16_t*)ptr = pix;
					ptr += 2;
				}
			break;
			case 3:
				while(w--)
				{
					AM_PUT_3BPP_PIXEL(ptr,pix);
					ptr += 3;
				}
			break;
			case 4:
				while(w--)
				{
					*(uint32_t*)ptr = pix;
					ptr += 4;
				}
			break;
			default:
			break;
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sw_filled_rect (AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, uint32_t pix)
{
	AM_OSD_Rect_t dr;
	
	if(AM_OSD_ClipRect(&s->clip, rect, &dr))
	{
		uint8_t *pline = s->buffer+s->pitch*dr.y+dr.x*s->format->bytes_per_pixel;
		uint8_t *ptr;
		int dh=dr.h, dw;
		
		switch(s->format->bytes_per_pixel)
		{
			case 1:
				while(dh--)
				{
					memset(pline, pix, dr.w);
					pline += s->pitch;
				}
			break;
			case 2:
				while(dh--)
				{
					dw  = dr.w;
					ptr = pline;
					while(dw--)
					{
						*(uint16_t*)ptr = pix;
						ptr += 2;
					}
					pline += s->pitch;
				}
			break;
			case 3:
				while(dh--)
				{
					dw  = dr.w;
					ptr = pline;
					while(dw--)
					{
						AM_PUT_3BPP_PIXEL(ptr,pix);
						ptr += 3;
					}
					pline += s->pitch;
				}
			break;
			case 4:
				while(dh--)
				{
					dw  = dr.w;
					ptr = pline;
					while(dw--)
					{
						*(uint32_t*)ptr = pix;
						ptr += 4;
					}
					pline += s->pitch;
				}
			break;
			default:
			break;
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sw_normal_blit (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para)
{
	AM_OSD_Rect_t tmpr, dsr, ddr;
	uint8_t *psline, *pdline;
	uint8_t *ps, *pd;
	
	tmpr.x = 0;
	tmpr.y = 0;
	tmpr.w = src->width;
	tmpr.h = src->height;
	if(!AM_OSD_ClipRect(&tmpr, sr, &dsr))
		return AM_SUCCESS;
	
	tmpr.x = dsr.x+dr->x-sr->x;
	tmpr.y = dsr.y+dr->y-sr->y;
	tmpr.w = dsr.w;
	tmpr.h = dsr.h;
	if(!AM_OSD_ClipRect(&dst->clip, &tmpr, &ddr))
		return AM_SUCCESS;
	
	dsr.x = ddr.x+sr->x-dr->x;
	dsr.y = ddr.y+sr->y-dr->y;
	dsr.w = ddr.w;
	dsr.h = ddr.h;
	
	psline = src->buffer+src->pitch*dsr.y+dsr.x*src->format->bytes_per_pixel;
	pdline = dst->buffer+dst->pitch*ddr.y+ddr.x*dst->format->bytes_per_pixel;
	
	if((src->format->type==dst->format->type) && !para->enable_alpha && !para->enable_key)
	{
		int len = dsr.w*src->format->bytes_per_pixel;
		
		while(dsr.h--)
		{
			memcpy(pdline, psline, len);
			psline += src->pitch;
			pdline += dst->pitch;
		}
	}
	else
	{
		int dw;
		
		while(dsr.h--)
		{
			dw = dsr.w;
			ps = psline;
			pd = pdline;
			
			while(dw--)
			{
				AM_OSD_Color_t col;
				uint32_t pix;
				
				if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(src->format->type))
				{
					pix = *ps++;
					if(para->enable_key && (pix==para->key))
						goto next_pixel;
					if(pix<src->format->palette.color_count)
					{
						col = src->format->palette.colors[pix];
					}
					else
					{
						col.r = 0;
						col.g = 0;
						col.b = 0;
						col.a = 0xFF;
					}
				}
				else
				{
					switch(src->format->bytes_per_pixel)
					{
						case 1:
							pix = *ps++;
							if(para->enable_key && (pix==para->key))
								goto next_pixel;
						break;
						break;
						case 2:
							pix = *(uint16_t*)ps;
							ps += 2;
							if(para->enable_key && (pix==para->key))
								goto next_pixel;
						break;
						case 3:
							pix = AM_GET_3BPP_PIXEL(ps);
							ps += 3;
							if(para->enable_key && (pix==para->key))
								goto next_pixel;
						break;
						case 4:
							pix = *(uint32_t*)ps;
							ps += 4;
							if(para->enable_key && (pix==para->key))
								goto next_pixel;
						break;
						default:
							pix = 0;
						break;
					}
					
					AM_RGB_PIXEL_TO_COLOR(src->format,pix,&col);
				}
				
				if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(dst->format->type))
				{
					*pd = pix;
				}
				else
				{
					if(para->enable_alpha)
					{
						uint32_t dp;
						AM_OSD_Color_t dc;
						
						if(col.a==0)
						{
							goto next_pixel;
						}
						else if(col.a!=0xFF)
						{
							switch(dst->format->bytes_per_pixel)
							{
								case 1:
									dp = *pd;
								break;
								case 2:
									dp = *(uint16_t*)pd;
								break;
								case 3:
									dp = AM_GET_3BPP_PIXEL(pd);
								break;
								case 4:
									dp = *(uint32_t*)pd;
								break;
								default:
									dp = 0;
								break;
							}
							
							AM_RGB_PIXEL_TO_COLOR(dst->format,dp,&dc);
							
							col.r = (col.r*col.a+dc.r*(255-col.a))>>8;
							col.g = (col.g*col.a+dc.g*(255-col.a))>>8;
							col.b = (col.b*col.a+dc.b*(255-col.a))>>8;
							col.a = AM_MAX(col.a, dc.a);
						}
					}
					pix = AM_COLOR_TO_RGB_PIXEL(dst->format, &col);
					switch(dst->format->bytes_per_pixel)
					{
						case 1:
							*pd = pix;
						break;
						case 2:
							*(uint16_t*)pd = pix;
						break;
						case 3:
							AM_PUT_3BPP_PIXEL(pd, pix);
						break;
						case 4:
							*(uint32_t*)pd = pix;
						break;
						default:
						break;
					}
				}
next_pixel:
				pd += dst->format->bytes_per_pixel;
			}
			
			psline += src->pitch;
			pdline += dst->pitch;
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sw_stretch_blit (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para)
{
	AM_OSD_Rect_t tmpr, dsr, ddr;
	uint8_t *psline, *pdline;
	uint8_t *ps, *pd;
	int i, dw, x, y;
	
	tmpr.x = 0;
	tmpr.y = 0;
	tmpr.w = src->width;
	tmpr.h = src->height;
	if(!AM_OSD_ClipRect(&tmpr, sr, &dsr))
		return AM_SUCCESS;
	
	tmpr.x = (dsr.x-sr->x)*dr->w/sr->w+dr->x;
	tmpr.y = (dsr.y-sr->y)*dr->h/sr->h+dr->y;
	tmpr.w = dsr.w*dr->w/sr->w;
	tmpr.h = dsr.h*dr->h/sr->h;
	if(!AM_OSD_ClipRect(&dst->clip, &tmpr, &ddr))
		return AM_SUCCESS;
	
	dsr.x = (ddr.x-dr->x)*sr->w/dr->w+sr->x;
	dsr.y = (ddr.y-dr->y)*sr->h/dr->h+sr->y;
	dsr.w = ddr.w*sr->w/dr->w;
	dsr.h = ddr.h*sr->h/dr->h;
	
	if(ddr.w>MAX_W)
	{
		AM_DEBUG(1, "too big surface");
		return AM_SUCCESS;
	}
	
	//AM_DEBUG(1, "SW: %d %d %d %d, %d %d %d %d", dsr.x, dsr.y, dsr.w, dsr.h, ddr.x, ddr.y, ddr.w, ddr.h);
	
	pdline = dst->buffer+dst->pitch*ddr.y+ddr.x*dst->format->bytes_per_pixel;
	
	pthread_mutex_lock(&lock);
	
	for(i=0; i<ddr.w; i++)
		convx[i] = (i*sr->w/dr->w+dsr.x)*src->format->bytes_per_pixel;
	
	y = 0;
	while(ddr.h--)
	{
		psline = src->buffer+(y*sr->h/dr->h+dsr.y)*src->pitch;
		pd = pdline;
		dw = ddr.w;
		x  = 0;
		
		while(dw--)
		{
			uint32_t pix;
			AM_OSD_Color_t col;
			
			ps = psline+convx[x];
			
			if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(src->format->type))
			{
				pix = *ps;
				if(para->enable_key && (pix==para->key))
					goto next_pixel;
				if(pix<src->format->palette.color_count)
				{
					col = src->format->palette.colors[pix];
				}
				else
				{
					col.r = 0;
					col.g = 0;
					col.b = 0;
					col.a = 0xFF;
				}
			}
			else
			{
				switch(src->format->bytes_per_pixel)
				{
					case 1:
						pix = *ps;
						if(para->enable_key && (pix==para->key))
							goto next_pixel;
					break;
					case 2:
						pix = *(uint16_t*)ps;
						if(para->enable_key && (pix==para->key))
							goto next_pixel;
					break;
					case 3:
						pix = AM_GET_3BPP_PIXEL(ps);
						if(para->enable_key && (pix==para->key))
							goto next_pixel;
					break;
					case 4:
						pix = *(uint32_t*)ps;
						if(para->enable_key && (pix==para->key))
							goto next_pixel;
					break;
					default:
						pix = 0;
					break;
				}
				
				AM_RGB_PIXEL_TO_COLOR(src->format,pix,&col);
			}
			
			if(AM_OSD_PIXEL_FORMAT_TYPE_IS_PALETTE(dst->format->type))
			{
				*pd = pix;
			}
			else
			{
				if(para->enable_alpha)
				{
					uint32_t dp;
					AM_OSD_Color_t dc;
					
					if(col.a==0)
					{
						goto next_pixel;
					}
					else if(col.a!=0xFF)
					{
						switch(dst->format->bytes_per_pixel)
						{
							case 2:
								dp = *(uint16_t*)pd;
							break;
							case 3:
								dp = AM_GET_3BPP_PIXEL(pd);
							break;
							case 4:
								dp = *(uint32_t*)pd;
							break;
							default:
								dp = 0;
							break;
						}
					
						AM_RGB_PIXEL_TO_COLOR(dst->format,dp,&dc);
						
						col.r = (col.r*col.a+dc.r*(255-col.a))>>8;
						col.g = (col.g*col.a+dc.g*(255-col.a))>>8;
						col.b = (col.b*col.a+dc.b*(255-col.a))>>8;
						col.a = AM_MAX(col.a, dc.a);
					}
				}
				pix = AM_COLOR_TO_RGB_PIXEL(dst->format, &col);
				switch(dst->format->bytes_per_pixel)
				{
					case 1:
						*pd = pix;
					break;
					case 2:
						*(uint16_t*)pd = pix;
					break;
					case 3:
						AM_PUT_3BPP_PIXEL(pd, pix);
					break;
					case 4:
						*(uint32_t*)pd = pix;
					break;
					default:
					break;
				}
			}
next_pixel:
			pd += dst->format->bytes_per_pixel;
			x++;
		}
		
		pdline += dst->pitch;
		y++;
	}
	
	pthread_mutex_unlock(&lock);
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t sw_blit (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para)
{
	if((sr->w==dr->w) && (sr->h==dr->h))
	{
		return sw_normal_blit(src, sr, dst, dr, para);
	}
	else
	{
		return sw_stretch_blit(src, sr, dst, dr, para);
	}
}
