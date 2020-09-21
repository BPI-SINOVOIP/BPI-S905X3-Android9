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
 * \brief 图片加载测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-18: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_img.h>
#include <am_av.h>
#include <string.h>

/****************************************************************************
 * Functions
 ***************************************************************************/

#define OSD_DEV_NO    0
#define AV_DEV_NO     0

int main(int argc, char **argv)
{
	AM_OSD_OpenPara_t para;
	AM_AV_OpenPara_t avp;
	AM_IMG_DecodePara_t dec;
	int i;
	char *name;
	AM_OSD_Surface_t *img;
	
	if(argc<2)
	{
		fprintf(stderr, "usage: am_img_test FILE...\n");
		return 1;
	}
	
	memset(&avp, 0, sizeof(avp));
	memset(&para, 0, sizeof(para));
	
	para.format = AM_OSD_FMT_COLOR_565;
	para.width  = 1280;
	para.height = 720;
	para.enable_double_buffer = AM_FALSE;
	
	dec.enable_hw = AM_TRUE;
	dec.hw_dev_no = AV_DEV_NO;
	dec.surface_flags = AM_OSD_SURFACE_FL_HW;
	
	AM_AV_Open(AV_DEV_NO, &avp);
	
	//AM_AV_StartFile(AV_DEV_NO, "/mnt/nfs/tmp/test.mp3", 0, 0);
	
	AM_OSD_Open(OSD_DEV_NO, &para);
	
	for(i=1; i<argc; i++)
	{
		name = argv[i];
		
		if(AM_IMG_Load(name, &dec, &img)==AM_SUCCESS)
		{
			AM_OSD_BlitPara_t bp;
			AM_OSD_Surface_t *screen;
			AM_OSD_Rect_t sr, dr;
	
			memset(&bp, 0, sizeof(bp));

			bp.enable_alpha = AM_FALSE;
			bp.enable_key   = AM_FALSE;
			
			AM_OSD_GetSurface(OSD_DEV_NO, &screen);
			
			sr.x = 0;
			sr.y = 0;
			sr.w = img->width;
			sr.h = img->height;
			
			dr.x = 0;
			dr.y = 0;
			dr.w = screen->width;
			dr.h = screen->height;
			
			printf("open image \"%s\" width %d height %d BPP %d\n", name, img->width, img->height, img->format->bytes_per_pixel);
			
			AM_OSD_DrawFilledRect(screen, &dr, 0);
			//AM_OSD_DrawFilledRect(img, &sr, 0xff00ff00);

			dr = sr;
			AM_OSD_Blit(img, &sr, screen, &dr, &bp);
			
			AM_OSD_Update(OSD_DEV_NO, NULL);
			//if(i!=argc-1)
				getchar();
			
			AM_OSD_DestroySurface(img);
		}
	}
	
	AM_OSD_Close(OSD_DEV_NO);
	AM_AV_Close(AV_DEV_NO);
	
	return 0;
}

