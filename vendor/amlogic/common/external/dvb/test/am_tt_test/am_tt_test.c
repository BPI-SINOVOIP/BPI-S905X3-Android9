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
#include "am_font.h"
#include "am_tt.h"

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "stdlib.h"


#define FEND_DEV_NO 0
#define DMX_DEV_NO  0
#define AV_DEV_NO   0



typedef struct teletext_app_s
{
	AM_OSD_Surface_t *s;
	int osd_id;
	int fid;
	int osd_width;
	int osd_height;
	int output_width;
	int output_height;
	AM_FONT_Attr_t attr;
	AM_FONT_TextInfo_t info;
}teletext_app_t;

enum
{
    VTCOLOR_BLACK   = 0,
    VTCOLOR_BLUE     = 1,
    VTCOLOR_GREEN   = 2,
    VTCOLOR_CYAN  = 3,
    VTCOLOR_RED    = 4,
    VTCOLOR_MAGENTA = 5,
    VTCOLOR_YELLOW    = 6,
    VTCOLOR_WHITE   = 15,

};



static teletext_app_t tat;

static void teletext_display_update()
{
	AM_OSD_Surface_t *s1,*s2;
	AM_OSD_Rect_t r1,r2;
	//AM_OSD_Update(tat.osd_id,0);

	AM_OSD_GetSurface(1,&s1);
	AM_OSD_GetDisplaySurface(1,&s2);		
	r1.x=0;
	r1.y=0;
	r1.w=720;
	r1.h=576;
	r2.x=0;
	r2.y=0;
	r2.w=720;
	r2.h=576;
	AM_OSD_Blit(s1,&r1,s2,&r2,0);
}

static void teletext_display_fill_rectangle(int left, int top, unsigned int width, unsigned int height, unsigned int color)
{
	AM_OSD_Rect_t r;
	if(tat.s)
	{
		r.x = left;
		r.y = top;
		r.w = width;
		r.h = height;
		AM_OSD_DrawFilledRect(tat.s, &r, color);
	}
}

static void teletext_display_draw_text(int x, int y, unsigned int width, unsigned int height, unsigned short *text, int len, unsigned int color, unsigned int w_scale, unsigned int h_scale)
{
	AM_FONT_TextPos_t pos;
	char *tmp=(char *)text;
#if 0
	static char text_buffer[80];
	static char text_len=0;
	static int start_x=0;
	static int start_y=0;

	if(text_len==0)
	{
		start_x=x;
		start_y=y;
	}

	if(len==2)
	{
		text_buffer[text_len]=tmp[0];	
		text_buffer[text_len+1]=tmp[1];
		text_len+=2;
	}

	printf("len=%d %x %x\n",len,tmp[1],tmp[0]);

	if(tmp[1]==0x20&&tmp[0]==0&&len==2&&text_len>2)
	{
		pos.rect.x = start_x;
		pos.rect.w = width*text_len/2;
		pos.rect.y = start_y;
		pos.rect.h = height;
		pos.base   = 0;
		AM_FONT_Draw(tat.fid, tat.s, (char*)text_buffer, text_len, &pos, color);
		text_len=0;
		memset(text_buffer,0,80);
		start_x=0;
		start_y=0;
		printf("*****************8ok\n");

		return 0;

	}
#endif

	if(tat.s&&tat.fid>0)
	{
		pos.rect.x = x;
		pos.rect.w = width;
		pos.rect.y = y;
		pos.rect.h = height;
		pos.base   = 0;
		AM_FONT_Draw(tat.fid, tat.s, (char*)text, len, &pos, color);
	}

}

static unsigned int teletext_display_convert_color(unsigned int index)
{
	AM_OSD_Color_t col;
	uint32_t pixel;
	switch(index)
	{
		case  VTCOLOR_BLACK   :
			col.r = 0;
			col.g = 0;
			col.b = 0;
			col.a = 255;
			break;
		case  VTCOLOR_RED     :
			col.r = 255;
			col.g = 0;
			col.b = 0;
			col.a = 255;
			break;
		case  VTCOLOR_GREEN   :
			col.r = 0;
			col.g = 255;
			col.b = 0;
			col.a = 255;
			break;
		case  VTCOLOR_YELLOW  :
			col.r = 255;
			col.g = 255;
			col.b = 0;
			col.a = 255;
			break;
		case  VTCOLOR_BLUE    :
			col.r = 0;
			col.g = 0;
			col.b = 255;
			col.a = 255;
			break;
		case  VTCOLOR_MAGENTA :
			col.r = 255;
			col.g = 0;
			col.b = 255;
			col.a = 255;
			break;
		case  VTCOLOR_CYAN    :
			col.r = 0;
			col.g = 255;
			col.b = 255;
			col.a = 255;
			break;
		case  VTCOLOR_WHITE   :
			col.r = 255;
			col.g = 255;
			col.b = 255;
			col.a = 255;
			break;
		default:
			col.r = 0;
			col.g = 0;
			col.b = 0;
			col.a = 255;

		      break;
	}
	AM_OSD_MapColor(tat.s->format, &col, &pixel);
	return pixel;


}

static unsigned int teletext_display_get_font_height(void)
{
	return 16;
	printf("tat.attr.width=%d\n",tat.attr.width);
	if(tat.fid>0)
		return	tat.attr.width;
	return 18;

}

static unsigned int teletext_display_get_font_max_width(void)
{
	return 16;
	printf("tat.attr.height=%d\n",tat.attr.height);
	if(tat.fid>0)
		return	tat.attr.height;
	return 18;
}


unsigned int teletext_mosaic_convert_color(unsigned char alpha, unsigned char red,  unsigned char green,  unsigned char blue)
{
	AM_OSD_Color_t col;
	uint32_t pixel;
	col.a=alpha;
	col.r=red;
	col.g=green;
	col.b=blue;
	AM_OSD_MapColor(tat.s->format, &col, &pixel);
	return pixel;
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


int main(int argc,char **argv)
{
	char buf[256];
	AM_OSD_OpenPara_t para_osd;
	int page_code=0;
	int sub_code=0;
	char path[256];
	AM_FONT_TextPos_t pos;
	AM_DMX_OpenPara_t para_dmx;
	int freq=427000000;
	AM_AV_OpenPara_t para_av;
	int v_pid=879;
	int a_pid=100;
	int v_format=0;
	int a_format=0;
	int s_pid=874;
	int osd_id=1;
	int auto_play=1;
	int ret=0;
	
	AM_OSD_Color_t col;
	AM_OSD_Rect_t r;
	uint32_t pixel;
	int link_color;
	AM_TT_DrawOps_t ops;

	struct pollfd fds[1];

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
		freq = strtol(argv[6], NULL, 0);
	}


	strcpy(path,"/system/fonts/DroidSerif-Bold.ttf");

	if(argc>7)
	{
		freq = strtol(argv[7], NULL, 0);
		strcpy(path,argv[7]);
	}

	if(argc<8)
	{
		printf("v_pid a_pid  s_pid v_format a_format freqfont_path \n");
		printf("default value: 879 100 874 0 0  427000 /system/fonts/DroidSerif-Bold.ttf \n");
	}


	AM_DMX_Open(0,&para_dmx);
	memset(&para_av, 0, sizeof(para_dmx));
	AM_AV_Open(AV_DEV_NO, &para_av);
	
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);
	AM_AV_SetTSSource(AV_DEV_NO, AM_AV_TS_SRC_TS2);

	memset(&para_osd, 0, sizeof(para_osd));

	get_display_mode(&tat.output_width,&tat.output_height);
	para_osd.format = AM_OSD_FMT_COLOR_8888;
	para_osd.width  = 720;
	para_osd.height = 576;
	para_osd.output_width  = tat.output_width;
	para_osd.output_height = tat.output_height;
	para_osd.enable_double_buffer = AM_TRUE;
	tat.osd_id=osd_id;

	tat.s=0;	

	AM_OSD_Open(tat.osd_id, &para_osd);
	AM_OSD_GetSurface(tat.osd_id, &tat.s);

	AM_FONT_Init();

	tat.attr.name   = "default";
	tat.attr.width  = 0;
	tat.attr.height = 0;
	tat.attr.encoding = AM_FONT_ENC_UTF16;
	tat.attr.charset = AM_FONT_CHARSET_UNICODE;
	tat.attr.flags  = 0;
	

	AM_FONT_LoadFile(path, "freetype", &tat.attr);

	tat.attr.width  = 16;
	tat.attr.height = 16;
	tat.attr.encoding = AM_FONT_ENC_UTF16;

	AM_FONT_Create(&tat.attr, &tat.fid);



	col.r = 0;
	col.g = 0;
	col.b = 0;
	col.a = 0;
	AM_OSD_MapColor(tat.s->format, &col, &pixel);
	r.x = 0;
	r.y = 0;
	r.w = tat.s->width;
	r.h = tat.s->height;
	AM_OSD_DrawFilledRect(tat.s, &r, pixel);
	teletext_display_update();

	if(freq>0)
	{
		AM_FEND_OpenPara_t para_freq;
		struct dvb_frontend_parameters p;
		fe_status_t status;
		
		memset(&para_freq, 0, sizeof(para_freq));
		AM_FEND_Open(FEND_DEV_NO, &para_freq);
		
		p.frequency = freq;
		p.u.qam.symbol_rate = 6875000;
		p.u.qam.fec_inner = FEC_AUTO;
		p.u.qam.modulation = QAM_64;
		
		AM_FEND_Lock(FEND_DEV_NO, &p, &status);
		
	}

	AM_AV_StartTS(AV_DEV_NO,v_pid,a_pid,v_format,a_format);
	printf("*****************\n");

	AM_TT_Init(0x20000);
	printf("*****************\n");

	memset(&ops, 0, sizeof(ops));
	ops.fill_rect  = teletext_display_fill_rectangle;
	ops.conv_color = teletext_display_convert_color;
	ops.draw_text  = teletext_display_draw_text;
	ops.font_height = teletext_display_get_font_height;
	ops.font_width  = teletext_display_get_font_max_width;
	ops.disp_update = teletext_display_update;
	ops.mosaic_conv = teletext_mosaic_convert_color;
	AM_TT_RegisterDrawOps(&ops);
	printf("*****************\n");
	AM_TT_Start(0,s_pid);
	printf("*****************\n");


	int offset=0;
	while(1)
	{
		offset=0;
		memset(fds,0,sizeof(struct pollfd));
		fds[0].fd=1;
		fds[0].events=POLLIN;

		ret=poll(fds, 1, 2000);
		if(ret<=0)
		{
			if(auto_play)
				AM_TT_NextPage();
			continue;
		}

		while(1)
		{
			read(1,&buf[offset],1);
			//buf[offset]=getchar();
			if(buf[offset]!='\n'&&offset<255)
			{
				offset++;
				continue;
			}
			else
				break;
		}
		
		if(!strncmp(buf, "quit", 4))
		{
			AM_TT_Stop();
			AM_TT_Quit();
			AM_FONT_Destroy(tat.fid);
			AM_FONT_Quit();
			AM_OSD_Close(tat.osd_id);
			return 0;

		}
		if(!strncmp(buf, "subnext", 7))
		{
			AM_TT_NextSubPage();
		}
		if(!strncmp(buf, "next", 4))
		{
			AM_TT_NextPage();
		}
		if(!strncmp(buf, "pre", 3))
		{
			AM_TT_PreviousPage();
		}
		if(!strncmp(buf, "subpre", 6))
		{
			AM_TT_PreviousSubPage();
		}
		if(!strncmp(buf, "page:", 5))
		{
			printf("code:0x");
			scanf("%x",&page_code);
			printf("sub code:0x");
			scanf("%x",&sub_code);
			AM_TT_SetCurrentPageCode(page_code,sub_code);
		}
		if(!strncmp(buf, "link color", 10))
		{
			printf("link color(0~5):");
			scanf("%d",&link_color);
			AM_TT_PerformColorLink(link_color);
		}


		if(!strncmp(buf, "stop play", 9))
		{
			auto_play=0;	
		}
		if(!strncmp(buf, "start play", 10))
		{
			auto_play=1;	
		}
		if(!strncmp(buf, "clean", 5))
		{
			AM_FileEcho("/sys/class/graphics/fb0/blank","1");

		}


	}
}
#if 0
int main(int argc,char **argv)
{

	printf("testing\n");
	printf("testing\n");
	printf("testing\n");
	printf("testing\n");
	printf("testing\n");	
	printf("testing\n");
	return 0;
}
#endif
