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
 * \brief ËäÇÁõÆÂàóË°®Á™óÂè£
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-25: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include "dtv.h"
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static data
 ***************************************************************************/

static int prog_list_init (DTV_Window_t *win);
static int prog_list_update (DTV_Window_t *win, AM_OSD_Surface_t *s);
static int prog_list_on_key (DTV_Window_t *win, AM_INP_Key_t key);
static int prog_list_on_timer (DTV_Window_t *win);
static int prog_list_on_show (DTV_Window_t *win);
static int prog_list_on_hide (DTV_Window_t *win);
static int prog_list_quit (DTV_Window_t *win);

static struct {
	DTV_Window_t win;
	int          focus;
	int 		 index;
} prog_list = {
{
.init     = prog_list_init,
.update   = prog_list_update,
.on_key   = prog_list_on_key,
.on_timer = prog_list_on_timer,
.on_show  = prog_list_on_show,
.on_hide  = prog_list_on_hide,
.quit     = prog_list_quit
}
};

DTV_Window_t *prog_list_win = (DTV_Window_t*)&prog_list;



#define PROLIST_ITEM_COUNT 10

/****************************************************************************
 * Functions
 ***************************************************************************/

static int prog_list_init (DTV_Window_t *win)
{
	return 0;
}

static int prog_list_update (DTV_Window_t *win, AM_OSD_Surface_t *s)
{
	AM_OSD_Rect_t sr, dr;
	int i =0 ,tmp;
	char *prolist="øøøø";

	if(program_count > 0 )
	{
		uint32_t pixel_test;
		AM_OSD_Color_t col;
		AM_FONT_TextPos_t pos;	

		col.r = 255;
		col.g = 255;
		col.b = 255;
		col.a = 255;
		AM_OSD_MapColor(s->format, &col, &pixel_test);
/*
		pos.rect.x = dr.x+10;
		pos.rect.w = dr.w;
		pos.rect.y = 100-8;
		pos.rect.h = dr.h;
		pos.base   = pos.rect.h;
		AM_FONT_Draw(gfx.def_font,s,prolist,strlen(prolist),&pos,pixel_test);
*/

		sr.x = 0;
		sr.y = 0;
		sr.w = gfx.list_frame_img->width;
		sr.h = gfx.list_frame_img->height;
		
		dr.x = 200;
		dr.y = 92;
		dr.w = gfx.list_frame_img->width;
		dr.h = gfx.list_frame_img->height;

		
		AM_OSD_Blit(gfx.list_frame_img, &sr, s, &dr, NULL);
	
		//AM_DEBUG(1,"focus = %d\n",prog_list.focus);
		
		sr.x = 0;
		sr.y = 0;
		sr.w = gfx.list_focus_img->width;
		sr.h = gfx.list_focus_img->height;
		
		dr.x = 200+3;
		dr.y = 100 + prog_list.focus * gfx.list_focus_img->height;
		dr.w = gfx.list_focus_img->width;
		dr.h = gfx.list_focus_img->height;

		AM_OSD_Blit(gfx.list_focus_img, &sr, s, &dr, NULL);
	
		
		char str[8];
		int temp1,temp2;
		temp2 = (prog_list.index/PROLIST_ITEM_COUNT)*PROLIST_ITEM_COUNT;

		//AM_DEBUG(1,"temp2 = %d\n",temp2);
			
		if(temp2 + PROLIST_ITEM_COUNT  > program_count)
		{
			temp1 = program_count;
		}
		else
		{
			temp1 = temp2 + PROLIST_ITEM_COUNT;
		}
		//AM_DEBUG(1,"temp1 = %d\n",temp1);

		for (i= temp2;i<temp1;i++)
		{
#if 0			
			printf("%d. [%s]:vid_pid %d, aud_pid %d\n", programs[i].chan_num, \
											programs[i].name,\
											programs[i].video_pid,\
											programs[i].audio_pid);
#endif
			//program NO.
			pos.rect.x = dr.x+10;
			pos.rect.w = dr.w;
			pos.rect.y = 100 + (i-temp2+1)* gfx.list_focus_img->height -8;
			pos.rect.h = dr.h;
			pos.base   = pos.rect.h;


			sprintf(str,"%d",programs[i].chan_num);
			AM_FONT_Draw(gfx.def_font,s,str,strlen(str),&pos,pixel_test);

			//program name
			pos.rect.x = dr.x+80;
			AM_FONT_Draw(gfx.def_font,s,programs[i].name,strlen(programs[i].name),&pos,pixel_test);
		}

	}
	else
	{
		printf("No program!\n");
	}

	return 0;
}

static int prog_list_on_key (DTV_Window_t *win, AM_INP_Key_t key)
{
	int ret = 0;
	
	switch(key)
	{
		case AM_INP_KEY_CANCEL:
			dtv_window_hide(win);
			ret = 1;
		break;
		case AM_INP_KEY_UP:
			
			prog_list.index -- ;
			if(prog_list.index < 0)
			{
				prog_list.index = program_count-1;
				prog_list.focus = (prog_list.index%PROLIST_ITEM_COUNT);

			}
			else
			{
				prog_list.focus--;
				if(prog_list.focus<0)
					prog_list.focus = PROLIST_ITEM_COUNT-1;
			}
			
			ret = 1;
		break;
		case AM_INP_KEY_DOWN:
			prog_list.index ++ ;
			if(prog_list.index >= program_count)
			{
				prog_list.index = 0;	
				prog_list.focus = 0;
			}
			else
			{
				prog_list.focus++;
				if(prog_list.focus>=PROLIST_ITEM_COUNT)
					prog_list.focus = 0;
			}
			ret = 1;
		break;
		case AM_INP_KEY_OK:
			dtv_program_play_by_channel_no(prog_list.index+1);
		break;
		default:
		break;
	}
	
	return ret;
}

static int prog_list_on_timer (DTV_Window_t *win)
{
	return 0;
}

static int prog_list_on_show (DTV_Window_t *win)
{
/*	int i =0 ;

	program_count = 6;
	curr_program = 3;
	for(i=0;i< program_count;i++)
	{
		programs[i].chan_num = i+1;
	}
*/	
	if(curr_program == -1)
		curr_program = 0;

	prog_list.focus = curr_program%PROLIST_ITEM_COUNT;	
	prog_list.index = curr_program;
	
	//AM_DEBUG(1,"focus = %d\n",prog_list.focus);
	return 0;
}

static int prog_list_on_hide (DTV_Window_t *win)
{
	return 0;
}

static int prog_list_quit (DTV_Window_t *win)
{
	return 0;
}

