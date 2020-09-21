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
 * \brief OSD模块测试
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-19: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_osd.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define OUTPUT_W 1920
#define OUTPUT_H 1080

#define PIXEL_COUNT    4
static uint32_t pixel[PIXEL_COUNT];
static int test_osd = 0;

static void update(AM_OSD_Surface_t *surf)
{
	AM_OSD_Surface_t *osd_surf;
	
	AM_OSD_GetSurface(test_osd, &osd_surf);
	if(osd_surf!=surf)
	{
		AM_OSD_Rect_t sr, dr;
		AM_OSD_BlitPara_t p;
		
		sr.x = 0;
		sr.y = 0;
		sr.w = surf->width;
		sr.h = surf->height;
		
		dr.x = 0;
		dr.y = 0;
		dr.w = osd_surf->width;
		dr.h = osd_surf->height;
		
		p.enable_alpha = AM_FALSE;
		p.enable_key   = AM_FALSE;
		
		AM_OSD_Blit(surf, &sr, osd_surf, &dr, &p);
	}
	AM_OSD_Update(test_osd, NULL);
}

static int clear(AM_OSD_Surface_t *surf)
{
	AM_OSD_Color_t col;
	uint32_t pix;
	AM_OSD_Rect_t rect;
	
	col.r = 0;
	col.g = 0;
	col.b = 0;
	col.a = 255;
	
	AM_OSD_MapColor(surf->format, &col, &pix);
	
	rect.x = 0;
	rect.y = 0;
	rect.w = surf->width;
	rect.h = surf->height;
	AM_OSD_DrawFilledRect(surf, &rect, pix);
	
	update(surf);
	
	return 0;
}

static int draw_pixel(AM_OSD_Surface_t *surf)
{
	int i, x, y, pix;
	
	for(i=0; i<10000; i++)
	{
		x = rand()%surf->width;
		y = rand()%surf->height;
		pix = rand()%PIXEL_COUNT;
		
		AM_OSD_DrawPixel(surf, x, y, pixel[pix]);
	}
	
	update(surf);
	
	return 0;
}

static int draw_hline(AM_OSD_Surface_t *surf)
{
	int i;
	
	for(i=0; i<surf->height; i++)
	{
		AM_OSD_DrawHLine(surf, 0, i, surf->width, pixel[i%PIXEL_COUNT]);
		update(surf);
	}
	
	return 0;
}

static int draw_vline(AM_OSD_Surface_t *surf)
{
	int i;
	
	for(i=0; i<surf->width; i++)
	{
		AM_OSD_DrawVLine(surf, i, 0, surf->height, pixel[i%PIXEL_COUNT]);
		update(surf);
	}
	
	return 0;
}

static int draw_rect(AM_OSD_Surface_t *surf)
{
	int i, pix;
	AM_OSD_Rect_t r;
	
	for(i=0; i<10000; i++)
	{
		r.x = rand()%surf->width;
		r.y = rand()%surf->height;
		r.w = rand()%250+1;
		r.h = rand()%250+1;
		pix = rand()%PIXEL_COUNT;
		AM_OSD_DrawRect(surf, &r, pixel[pix]);
		update(surf);
	}
	
	return 0;
}

static int draw_filled_rect(AM_OSD_Surface_t *surf)
{
	int i, pix;
	AM_OSD_Rect_t r;
	
	for(i=0; i<10000; i++)
	{
		r.x = rand()%surf->width;
		r.y = rand()%surf->height;
		r.w = rand()%250+1;
		r.h = rand()%250+1;
		pix = rand()%PIXEL_COUNT;
		AM_OSD_DrawFilledRect(surf, &r, pixel[pix]);
		update(surf);
	}
	
	return 0;
}

static int blit(AM_OSD_Surface_t *surf)
{
	AM_OSD_Rect_t s, d;
	AM_OSD_BlitPara_t para;
	AM_OSD_Color_t col;
	uint32_t pix;
	
	col.r = 255;
	col.g = 0;
	col.b = 0;
	col.a = 255;
	AM_OSD_MapColor(surf->format, &col, &pix);
	
	s.x = 50;
	s.y = 50;
	s.w = 100;
	s.h = 100;
	AM_OSD_DrawFilledRect(surf, &s, pix);
	
	d.x = 200;
	d.y = 50;
	d.w = 100;
	d.h = 100;
	
	para.enable_alpha = AM_FALSE;
	para.enable_key   = AM_FALSE;
	para.enable_global_alpha = AM_FALSE;
	para.enable_op    = AM_FALSE;
	
	AM_OSD_Blit(surf, &s, surf, &d, &para);
	
	d.x = 350;
	d.y = 50;
	d.w = 150;
	d.h = 150;
	
	AM_OSD_Blit(surf, &s, surf, &d, &para);
	
	d.x = 550;
	d.y = 50;
	d.w = 50;
	d.h = 50;
	
	AM_OSD_Blit(surf, &s, surf, &d, &para);
	
	col.r = 255;
	col.g = 0;
	col.b = 0;
	col.a = 128;
	AM_OSD_MapColor(surf->format, &col, &pix);
	
	s.x = 50;
	s.y = 200;
	s.w = 100;
	s.h = 100;
	AM_OSD_DrawFilledRect(surf, &s, pix);
	
	col.r = 0;
	col.g = 255;
	col.b = 0;
	col.a = 255;
	AM_OSD_MapColor(surf->format, &col, &pix);
	
	d.x = 200;
	d.y = 200;
	d.w = 100;
	d.h = 100;
	AM_OSD_DrawFilledRect(surf, &d, pix);
	
	d.x = 220;
	d.y = 220;
	d.w = 100;
	d.h = 100;
	
	para.enable_alpha = AM_TRUE;
	para.enable_key   = AM_FALSE;
	
	AM_OSD_Blit(surf, &s, surf, &d, &para);
	
	update(surf);
	
	return 0;
	
}

static int do_test(AM_OSD_Surface_t *surf)
{
	AM_OSD_Color_t col;
	
	col.r = 255;
	col.g = 0;
	col.b = 0;
	col.a = 255;
	AM_OSD_MapColor(surf->format, &col, &pixel[0]);
	col.r = 0;
	col.g = 255;
	col.b = 0;
	col.a = 255;
	AM_OSD_MapColor(surf->format, &col, &pixel[1]);
	col.r = 0;
	col.g = 0;
	col.b = 255;
	col.a = 255;
	AM_OSD_MapColor(surf->format, &col, &pixel[2]);
	col.r = 255;
	col.g = 255;
	col.b = 255;
	col.a = 255;
	AM_OSD_MapColor(surf->format, &col, &pixel[3]);
	
	clear(surf);
	draw_pixel(surf);
	
	getchar();
	
	clear(surf);
	draw_hline(surf);
	
	getchar();
	
	clear(surf);
	draw_vline(surf);
	
	getchar();
	
	clear(surf);
	draw_rect(surf);
	
	getchar();
	
	clear(surf);
	draw_filled_rect(surf);
	
	getchar();
	
	clear(surf);
	blit(surf);
	getchar();
	return 0;
}

static int osd_test(int id, AM_OSD_PixelFormatType_t fmt, int w, int h, AM_Bool_t draw_in_surf)
{
	AM_OSD_OpenPara_t para;
	AM_OSD_Surface_t *surf;
	
	memset(&para, 0, sizeof(para));
	
	para.format = fmt;
	para.width  = w;
	para.height = h;
	para.output_width  = OUTPUT_W;
	para.output_height = OUTPUT_H;
	para.enable_double_buffer = AM_FALSE;
	
	AM_TRY(AM_OSD_Open(id, &para));
	
	AM_OSD_Enable(id);
	AM_OSD_GetSurface(id, &surf);
	
	test_osd = id;
	
	if(draw_in_surf)
	{
		AM_OSD_Surface_t *draw_surf;
		
		AM_OSD_CreateSurface(fmt, w, h, AM_OSD_SURFACE_FL_HW, &draw_surf);
		do_test(draw_surf);
		AM_OSD_DestroySurface(draw_surf);
	}
	else
	{
		do_test(surf);
	}
	
	AM_OSD_Close(id);
	return 0;
}

int main(int argc, char **argv)
{
	osd_test(0, AM_OSD_FMT_COLOR_8888, 1920, 1080, 0);
	return 0;
}
