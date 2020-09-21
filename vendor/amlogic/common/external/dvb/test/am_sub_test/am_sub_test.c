/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include "am_dmx.h"
#include "am_av.h"
#include "am_fend.h"
#include "stdio.h"
#include "am_osd.h"
#include "am_misc.h"
#include <stdlib.h>

#include "am_sub.h"
#define FEND_DEV_NO 0
#define DMX_DEV_NO  0
#define AV_DEV_NO   0
#define OSD_DEV_NO  0


typedef struct screen_para_s
{
	int output_width;
	int output_height;
	AM_OSD_Surface_t *surf;
	int osd_id;
}screen_para_t;
static screen_para_t spt;


static void update_display()
{
	AM_OSD_Update(spt.osd_id,0);
}

void show_subtitle(AM_SUB_Screen_t* screen)
{
#ifdef PAL_256
	unsigned int i=0;
	unsigned int j=0,k=0;
	int x=0,y=0;
	int x_off=0,y_off=0;
	AM_OSD_Palette_t palette;
	if(screen)
	{
		for(i=0;i<screen->region_num;i++)
		{
			palette.color_count=screen->region[i].entry;
			AM_OSD_SetPalette(spt.surf,(AM_OSD_Color_t*)screen->region[i].palette.colors,0,screen->region[i].palette.color_count);
			x=screen->region[i].left;
			y=screen->region[i].top;
			x_off=(spt.output_width-screen->width)/2+x;
			y_off=y*spt.output_height/screen->height;

			x=x_off;
			y=y_off;

			for(j=0;j<screen->region[i].height;j++)
			{
				for(k=0;k<screen->region[i].width;k++)
				{
					AM_OSD_DrawPixel(spt.surf,x+k,y+j,screen->region[i].buffer[j*screen->region[i].width+k]);
				
				}

			}

		}
	}
#else
	unsigned int i=0;
	unsigned int j=0,k=0;
	int x=0,y=0;
	int color_index=0;
	unsigned int pixel=0;
	AM_OSD_Color_t col;

	if(screen)
	{
		for(i=0;i<screen->region_num;i++)
		{
			x=screen->region[i].left;
			y=screen->region[i].top;
			for(j=0;j<screen->region[i].height;j++)
			{
				for(k=0;k<screen->region[i].width;k++)
				{
					color_index=screen->region[i].buffer[j*screen->region[i].width+k];
					if(color_index<screen->region[i].palette.color_count)
					{
						col=screen->region[i].palette.colors[color_index];
					}
					AM_OSD_MapColor(spt.surf->format, &col, &pixel);

					AM_OSD_DrawPixel(spt.surf,x+k,y+j,pixel);
				
				}

			}

		}
	}
	update_display();
#endif
}

void clean_subtitle(AM_SUB_Screen_t* screen)
{
#ifdef PAL_256
	unsigned int i=0;
	unsigned int j=0,k=0;
	int x=0,y=0;
	int x_off=0,y_off=0;
	AM_OSD_Palette_t palette;
	AM_FileEcho("/sys/class/graphics/fb0/blank","1");

	if(screen)
	{
		for(i=0;i<screen->region_num;i++)
		{
			palette.color_count=screen->region[i].entry;
			AM_OSD_SetPalette(spt.surf,(AM_OSD_Color_t*)screen->region[i].palette.colors,0,screen->region[i].palette.color_count);
			x=screen->region[i].left;
			y=screen->region[i].top;
			x_off=(spt.output_width-screen->width)/2+x;
			y_off=y*spt.output_height/screen->height;
			x=x_off;
			y=y_off;

			for(j=0;j<screen->region[i].height;j++)
			{
				for(k=0;k<screen->region[i].width;k++)
				{
					AM_OSD_DrawPixel(spt.surf,x+k,y+j,0);
				
				}

			}

		}
	}
#else
	unsigned int i=0;
	unsigned int j=0,k=0;
	int x=0,y=0;
	int color_index=0;
	unsigned int pixel=0;
	AM_OSD_Color_t col;

	if(screen)
	{
		for(i=0;i<screen->region_num;i++)
		{
			x=screen->region[i].left;
			y=screen->region[i].top;
			for(j=0;j<screen->region[i].height;j++)
			{
				for(k=0;k<screen->region[i].width;k++)
				{
					color_index=0;
					if(color_index<screen->region[i].palette.color_count)
					{
						col=screen->region[i].palette.colors[color_index];
					}
					AM_OSD_MapColor(spt.surf->format, &col, &pixel);

					AM_OSD_DrawPixel(spt.surf,x+k,y+j,pixel);
				
				}

			}

		}
	}

	//update_display();
	AM_FileEcho("/sys/class/graphics/fb0/blank","1");


#endif
}

static int get_display_mode(int *width,int *height)
{
	char buffer[16];
#define DISPLAY_MODE_PATH	"/sys/class/display/mode"
	if(!width||!height)
	{
		return -1;
	}
	AM_FileRead(DISPLAY_MODE_PATH,buffer,16);


	if(!strncmp(buffer,"720p",4))
	{
		*width=1280;
		*height=720;
		return 0;
	}
	else if(!strncmp(buffer,"1080i",5)||!strncmp(buffer,"1080p",5))
	{
		*width=1920;
		*height=1080;
		return 0;

	}
	else 
	{
		*width=720;
		*height=576;
		return 0;

	}
	
}

static int clean_osd()
{
#ifdef PAL_256
	int i=0,j=0;

	AM_OSD_Palette_t palette;
	AM_OSD_Color_t colors;
	colors.r=0;
	colors.g=0;
	colors.b=0;
	colors.a=0;
	palette.colors=&colors;

	AM_OSD_SetPalette(spt.surf,&colors,0,1);

	for(i=0;i<spt.output_height;i++)
		for(j=0;j<spt.output_width;j++)
		{
			AM_OSD_DrawPixel(spt.surf,j,i,0);

		}

	return 0;
#else
	int i=0;
	int j=0;
	unsigned int pixel=0;
	AM_OSD_Color_t col;
	col.r=0;
	col.g=0;
	col.b=0;
	col.a=0;
	AM_OSD_MapColor(spt.surf->format, &col, &pixel);

	for(i=0;i<spt.output_height;i++)
		for(j=0;j<spt.output_width;j++)
		{
			AM_OSD_DrawPixel(spt.surf,j,i,pixel);

		}
	update_display();
	return 0;
#endif
}




int main(int argc,char **argv)
{
	char buf[256];
	AM_DMX_OpenPara_t para;
	int freq=427000000;
	AM_AV_OpenPara_t para1;
	int v_pid=600;
	int a_pid=100;
	int v_format=0;
	int a_format=0;
	int s_pid=603;
	int page_id=1;
	AM_OSD_OpenPara_t para_osd;


	if(argc>1)
	{
		v_pid = strtol(argv[1], NULL, 0);
	}
	if(argc>2)
	{
		a_pid = strtol(argv[2], NULL, 0);
	}
	if(argc>3)
	{
		s_pid = strtol(argv[3], NULL, 0);
	}
	if(argc>4)
	{
		v_format = strtol(argv[4], NULL, 0);
	}
	if(argc>5)
	{
		a_format = strtol(argv[5], NULL, 0);
	}
	if(argc>6)
	{
		page_id = strtol(argv[6], NULL, 0);
	}
	if(argc>7)
	{
		freq = strtol(argv[7], NULL, 0);
	}

	if(argc<8)
	{
		printf("v_pid a_pid  s_pid v_format a_format page_id freq \n");
		printf("default value 600 100 603 0 0 1 427000\n");
	}


	memset(&para_osd, 0, sizeof(para_osd));
	get_display_mode(&spt.output_width,&spt.output_height);
	para_osd.format = AM_OSD_FMT_COLOR_8888;
	para_osd.width  =720;
	para_osd.height = 576;
	para_osd.output_width  = spt.output_width;
	para_osd.output_height = spt.output_height;
	para_osd.enable_double_buffer = AM_TRUE;

	spt.osd_id=1;	
	AM_OSD_Open(spt.osd_id, &para_osd);
	AM_OSD_Enable(spt.osd_id);
	AM_OSD_GetSurface(spt.osd_id, &spt.surf);
	clean_osd();

	AM_FileEcho("/sys/class/graphics/fb0/blank","1");


	AM_DMX_Open(0,&para);
	memset(&para, 0, sizeof(para1));
	AM_AV_Open(AV_DEV_NO, &para1);
	
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_TS2);
	if(freq>0)
	{
		AM_FEND_OpenPara_t para;
		struct dvb_frontend_parameters p;
		fe_status_t status;
		
		memset(&para, 0, sizeof(para));
		AM_FEND_Open(FEND_DEV_NO, &para);
		
		p.frequency = freq;
		p.u.qam.symbol_rate = 6875000;
		p.u.qam.fec_inner = FEC_AUTO;
		p.u.qam.modulation = QAM_64;
		
		AM_FEND_Lock(FEND_DEV_NO, &p, &status);
		
	}

	AM_AV_StartTS(AV_DEV_NO,v_pid,a_pid,v_format,a_format);
	AM_SUB_Init(0x20000);	
	AM_SUB_RegisterUpdateCbs(show_subtitle,clean_subtitle);
	AM_SUB_Start(0,s_pid,page_id,1);

	while(1)
	{
			
		if(gets(buf))
		{
			if(!strncmp(buf, "quit", 4))
			{
				AM_SUB_Stop();
				AM_SUB_Quit();
				clean_osd();
				return 0;
			}
			if(!strncmp(buf, "show", 4))
			{
				AM_SUB_Show(AM_TRUE);
			}
			if(!strncmp(buf, "hide", 4))
			{
				AM_SUB_Show(AM_FALSE);
			}
			if(!strncmp(buf, "change", 4))
			{
				printf("v_pid:");
				scanf("%d",&v_pid);
				printf("a_pid:");
				scanf("%d",&a_pid);
				printf("s_pid:");
				scanf("%d",&s_pid);
				printf("v_format");
				scanf("%d",&v_format);
				printf("a_format");
				scanf("%d",&a_format);
				AM_SUB_Stop();
				AM_AV_StartTS(AV_DEV_NO,v_pid,a_pid,v_format,a_format);
				AM_SUB_Start(0,s_pid,page_id,1);



			}


		}
	}
	return 0;
}
