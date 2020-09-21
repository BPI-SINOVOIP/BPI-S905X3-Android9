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
 * \brief DVBC搜索参数窗口
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "dtv.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/

static int dvbc_param_init (DTV_Window_t *win);
static int dvbc_param_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int dvbc_param_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int dvbc_param_on_timer (DTV_Window_t *win);
static int dvbc_param_on_show (DTV_Window_t *win);
static int dvbc_param_on_hide (DTV_Window_t *win);
static int dvbc_param_quit (DTV_Window_t *win);

static struct {
	DTV_Window_t win;
	int          focus;
} dvbc_param = {
{
.init     = dvbc_param_init,
.update   = dvbc_param_update,
.on_key   = dvbc_param_on_key,
.on_timer = dvbc_param_on_timer,
.on_show  = dvbc_param_on_show,
.on_hide  = dvbc_param_on_hide,
.quit     = dvbc_param_quit
}
};

#define MAX_LENGTH 64
#define MAX_ACTIVE_WINDOWS 3
#define MAX_COMB_ITEM 5

typedef enum 
{
	BASE_WIN=0,
	WIN_EDIT,
	WIN_BUTTON,
	WIN_COMBO,
	WIN_STATIC
}win_type_t;

typedef struct geometry
{
	int x;
	int y;
	int width;
	int height;
}geometry_t;

typedef struct dvb_win
{
	geometry_t geo;
	int back_color_fucus_out;
	int back_color_fucus_in;
	int text_color;
	char text_buff[MAX_LENGTH];
	win_type_t type;
	char isShow;
	int combox_index;
	
}dvb_win_t;

DTV_Window_t *dvbc_param_win = (DTV_Window_t*)&dvbc_param;

static dvb_win_t static_title;
static dvb_win_t static_freq;
static dvb_win_t static_sym;
static dvb_win_t static_qam;
static dvb_win_t edit_freq;
static dvb_win_t edit_sym;
static dvb_win_t comb_qam;
static dvb_win_t button_ok;
static dvb_win_t button_quit;

static int focus_id=0;

/****************************************************************************
 * Functions
 ***************************************************************************/

static int dvbc_param_init (DTV_Window_t *win)
{
	static_title.geo.x=50;
	static_title.geo.y=100;
	static_title.geo.width=100;
	static_title.geo.height=50;
	static_title.back_color_fucus_out=0xff00ff00;
	static_title.text_color=0xffffffff;
	static_title.isShow=1;
	strcpy(static_freq.text_buff,"searching parameters setting");

	static_freq.geo.x=20;
	static_freq.geo.y=160;
	static_freq.geo.width=50;
	static_freq.geo.height=30;
	static_freq.back_color_fucus_out=0xff00ff00;
	static_freq.back_color_fucus_in=0xff808080;
	static_freq.text_color=0xffffffff;
	static_freq.isShow=1;

	strcpy(static_freq.text_buff,"频率");

	static_sym.back_color_fucus_out=0xff00ff00;
	static_sym.back_color_fucus_in=0xff808080;
	static_sym.text_color=0xffffffff;
	static_sym.isShow=1;

	strcpy(static_sym.text_buff,"符号率");

	static_qam.back_color_fucus_out=0xff00ff00;
	static_qam.back_color_fucus_in=0xff808080;
	static_qam.text_color=0xffffffff;
	static_qam.isShow=1;

	strcpy(static_qam.text_buff,"调制");

	edit_freq.back_color_fucus_out=0xff00ff00;
	edit_freq.back_color_fucus_in=0xff808080;
	edit_freq.text_color=0x0;
	edit_freq.isShow=1;
	strcpy(edit_freq.text_buff,"419000");

	edit_sym.back_color_fucus_out=0xff00ff00;
	edit_sym.back_color_fucus_in=0xff808080;
	edit_sym.text_color=0x0;
	edit_sym.isShow=1;
	strcpy(edit_sym.text_buff,"6875");

	comb_qam.back_color_fucus_out=0xff00ff00;
	comb_qam.back_color_fucus_in=0xff808080;
	comb_qam.text_color=0x0;
	comb_qam.isShow=1;
	strcpy(comb_qam.text_buff,"QAM64");


	comb_qam.combox_index=2;


	focus_id=0;
	
	return 0;
}

static int dvbc_param_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	AM_FONT_TextPos_t pos;
	AM_OSD_Rect_t rect;	

	AM_OSD_Rect_t sr, dr;
	
	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.bg_img->width;
	sr.h = gfx.bg_img->height;
	
	dr.x = 0;
	dr.y = 0;
	dr.w = s->width;
	dr.h = s->height;

	
	AM_OSD_Blit(gfx.bg_img, &sr, s, &dr, NULL);

	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.title_search_img->width;
	sr.h = gfx.title_search_img->height;
	
	dr.x = 72;
	dr.y = 72;
	dr.w = gfx.title_search_img->width;
	dr.h = gfx.title_search_img->height;

	
	AM_OSD_Blit(gfx.title_search_img, &sr, s, &dr, NULL);

	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.search_prompt->width;
	sr.h = gfx.search_prompt->height;
	
	dr.x = 330;
	dr.y = 560;
	dr.w = gfx.search_prompt->width;
	dr.h = gfx.search_prompt->height;

	
	AM_OSD_Blit(gfx.search_prompt, &sr, s, &dr, NULL);


	pos.base=0;
	pos.rect.x=400;
	pos.rect.y=250;
	pos.rect.w=100;
	pos.rect.h=40;
	AM_FONT_Draw(gfx.def_font,s,static_freq.text_buff,strlen(static_freq.text_buff),&pos,static_freq.text_color);


	pos.rect.x=400;
	pos.rect.y=340;
	pos.rect.w=130;
	pos.rect.h=40;
	AM_FONT_Draw(gfx.def_font,s,static_sym.text_buff,strlen(static_sym.text_buff),&pos,static_sym.text_color);

	pos.rect.x=400;
	pos.rect.y=420;
	pos.rect.w=100;
	pos.rect.h=40;
	AM_FONT_Draw(gfx.def_font,s,static_qam.text_buff,strlen(static_qam.text_buff),&pos,static_qam.text_color);

	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.inp_focus_img->width;
	sr.h = gfx.inp_focus_img->height;
	
	dr.x = 550;
	dr.y = 240;
	dr.w = gfx.inp_focus_img->width;
	dr.h = gfx.inp_focus_img->height;

	if(focus_id==0)	
	AM_OSD_Blit(gfx.inp_focus_img, &sr, s, &dr, NULL);
	else
	AM_OSD_Blit(gfx.inp_img, &sr, s, &dr, NULL);

	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.inp_img->width;
	sr.h = gfx.inp_img->height;
	
	dr.x = 550;
	dr.y = 320;
	dr.w = gfx.inp_img->width;
	dr.h = gfx.inp_img->height;

	
	if(focus_id==1)	
	AM_OSD_Blit(gfx.inp_focus_img, &sr, s, &dr, NULL);
	else
	AM_OSD_Blit(gfx.inp_img, &sr, s, &dr, NULL);


	sr.x = 0;
	sr.y = 0;
	sr.w = gfx.inp_img->width;
	sr.h = gfx.inp_img->height;
	
	dr.x = 550;
	dr.y = 400;
	dr.w = gfx.inp_img->width;
	dr.h = gfx.inp_img->height;

	
	if(focus_id==2)	
	AM_OSD_Blit(gfx.inp_focus_img, &sr, s, &dr, NULL);
	else
	AM_OSD_Blit(gfx.inp_img, &sr, s, &dr, NULL);


	pos.rect.x=560;
	pos.rect.y=247;
	pos.rect.w=250;
	pos.rect.h=40;
	AM_FONT_Draw(gfx.def_font,s,edit_freq.text_buff,strlen(static_freq.text_buff),&pos,edit_freq.text_color);

	pos.rect.x=560;
	pos.rect.y=327;
	pos.rect.w=250;
	pos.rect.h=40;
	AM_FONT_Draw(gfx.def_font,s,edit_sym.text_buff,strlen(static_freq.text_buff),&pos,edit_freq.text_color);

	pos.rect.x=560;
	pos.rect.y=407;
	pos.rect.w=250;
	pos.rect.h=40;
	AM_FONT_Draw(gfx.def_font,s,comb_qam.text_buff,strlen(static_freq.text_buff),&pos,edit_freq.text_color);



	if(0)//(static_freq.isShow)	
	{

		rect.x=150;
		rect.y=100;
		rect.w=100;	
		rect.h=50;

		AM_OSD_DrawFilledRect(s,&rect,static_freq.text_color);
		rect.x=100;
		rect.y=160;
		rect.w=100;	
		rect.h=50;
		AM_OSD_DrawFilledRect(s,&rect,static_freq.text_color);
		rect.x=210;
		rect.y=160;
		rect.w=100;	
		rect.h=50;
		if(focus_id==0)
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_in);
		else
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_out);

		rect.x=100;
		rect.y=220;
		rect.w=100;	
		rect.h=50;
		AM_OSD_DrawFilledRect(s,&rect,static_freq.text_color);
		rect.x=210;
		rect.y=220;
		rect.w=100;	
		rect.h=50;
		if(focus_id==1)
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_in);
		else
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_out);

		rect.x=100;
		rect.y=280;
		rect.w=100;	
		rect.h=50;
		AM_OSD_DrawFilledRect(s,&rect,static_freq.text_color);
		rect.x=210;
		rect.y=280;
		rect.w=100;	
		rect.h=50;
		if(focus_id==2)
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_in);
		else
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_out);


		rect.x=100;
		rect.y=340;
		rect.w=100;	
		rect.h=50;
		if(focus_id==3)
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_in);
		else
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_out);


		rect.x=220;
		rect.y=340;
		rect.w=100;	
		rect.h=50;
		if(focus_id==4)
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_in);
		else
			AM_OSD_DrawFilledRect(s,&rect,static_freq.back_color_fucus_out);



		AM_FONT_Draw(gfx.def_font,s,static_freq.text_buff,strlen(static_freq.text_buff),&pos,static_freq.text_color);
	}
	return 0;
}


static int process_num_key(int key)
{
	char *pText=0;
	int textlen=0;
	char input='0';
	if(focus_id>1)	
	{
	
		if(key==AM_INP_KEY_LEFT)
		{
			comb_qam.combox_index--;
		}
		if(key==AM_INP_KEY_RIGHT)
		{
			comb_qam.combox_index++;

		}

		if(comb_qam.combox_index>=MAX_COMB_ITEM)
		{
			comb_qam.combox_index=0;
		}
		if(comb_qam.combox_index<0)
		{
			comb_qam.combox_index=MAX_COMB_ITEM-1;
		}



		switch(comb_qam.combox_index)
		{
			case 0:
			strcpy(comb_qam.text_buff,"QAM16");
			break;
			case 1:
			strcpy(comb_qam.text_buff,"QAM32");
			break;
			case 2:
			strcpy(comb_qam.text_buff,"QAM64");
			break;
			case 3:
			strcpy(comb_qam.text_buff,"QAM128");
			break;
			case 4:
			strcpy(comb_qam.text_buff,"QAM256");
			break;
			default:
			strcpy(comb_qam.text_buff,"QAM64");
			comb_qam.combox_index=2;
			break;
		}




		return 0;
	}
	if(focus_id)
	{
		pText=edit_sym.text_buff;
	}
	else 
		pText=edit_freq.text_buff;


	textlen=strlen(pText);

	if(key==AM_INP_KEY_LEFT)	
	{
		if(textlen>0)	
			pText[textlen-1]=0;
		return 0;
	}


	if(textlen>=13)
		return 0;
	



	switch(key)
	{
		case AM_INP_KEY_0:                        /**< 数字0*/
			
		case AM_INP_KEY_1:                        /**< 数字1*/
		case AM_INP_KEY_2:                        /**< 数字2*/
		case AM_INP_KEY_3:                        /**< 数字3*/
		case AM_INP_KEY_4:                        /**< 数字4*/
		case AM_INP_KEY_5:                        /**< 数字5*/
		case AM_INP_KEY_6:                        /**< 数字6*/
		case AM_INP_KEY_7:                        /**< 数字7*/
		case AM_INP_KEY_8:                        /**< 数字8*/
		case AM_INP_KEY_9:	
		input+=(key-AM_INP_KEY_0);
		pText[textlen]=input;
		pText[textlen+1]=0;
		break;
		
	}
	return 0;

}

static int start_search_now()
{
	struct dvb_frontend_parameters p;
	int handle;
	
	int freq,sym,qam;
	
	freq=atoi(edit_freq.text_buff);
	sym=atoi(edit_sym.text_buff)*1000;
	qam=comb_qam.combox_index+1;

	extern int enter_searching_progress(int freq,int sym,int qam);
	enter_searching_progress(freq,sym,qam);
	dtv_window_hide(dvbc_param_win);

	return 0;
}

static int dvbc_param_on_key (DTV_Window_t *win, AM_INP_Key_t key)
{
	int ret=0;
	switch(key)
	{
		case AM_INP_KEY_UP:

		focus_id--;
		if(focus_id<0)
		{
			focus_id=MAX_ACTIVE_WINDOWS-1;
		}
		ret=1;
		break;
		case AM_INP_KEY_DOWN:
		focus_id++;
		if(focus_id>=MAX_ACTIVE_WINDOWS)
		{
			focus_id=0;
		}
		ret=1;
		break;
		case  AM_INP_KEY_OK:
		start_search_now();
		ret=1;

		break;

		case  AM_INP_KEY_CANCEL:

		dtv_window_hide(dvbc_param_win);
		ret=1;
		
		break;

		default:
		process_num_key(key);
		ret=1;
		break;

	}

	return ret;
}

static int dvbc_param_on_timer (DTV_Window_t *win)
{
	return 0;
}

static int dvbc_param_on_show (DTV_Window_t *win)
{
	return 0;
}

static int dvbc_param_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int dvbc_param_quit (DTV_Window_t *win)
{
	return 0;
}

