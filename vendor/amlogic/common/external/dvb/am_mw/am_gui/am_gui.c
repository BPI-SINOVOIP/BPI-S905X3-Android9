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
 * \brief GUI控制器
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_mem.h>
#include <am_time.h>
#include <am_inp.h>
#include <assert.h>
#include <string.h>
#include "../../am_adp/am_osd/am_osd_internal.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define GUI_INTERVAL    (40)

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 重绘控件树*/
static void gui_redraw_widget(AM_GUI_Widget_t *w, AM_OSD_Surface_t *s, AM_OSD_Rect_t *clip, int orgx, int orgy)
{
	AM_GUI_Widget_t *c;
	AM_OSD_Rect_t cc;
	int corgx, corgy;
	AM_GUI_WidgetEvent_t evt;
	
	AM_OSD_SetClip(s, clip);
	
	evt.type = AM_GUI_WIDGET_EVT_DRAW_BEGIN;
	evt.draw.surface = s;
	evt.draw.org_x   = orgx;
	evt.draw.org_y   = orgy;
	AM_GUI_WidgetSendEvent(w, &evt);
	
	for(c=w->child_tail; c; c=c->prev)
	{
		if(!(c->flags&AM_GUI_WIDGET_FL_VISIBLE))
			continue;
		
		corgx = orgx+c->rect.x;
		corgy = orgy+c->rect.y;
		
		cc.x = corgx;
		cc.y = corgy;
		cc.w = c->rect.w;
		cc.h = c->rect.h;
		
		if(!AM_OSD_ClipRect(clip, &cc, &cc))
			continue;
		
		gui_redraw_widget(c, s, &cc, corgx, corgy);
	}
	
	evt.type = AM_GUI_WIDGET_EVT_DRAW_END;
	AM_GUI_WidgetSendEvent(w, &evt);
}

/**\brief 重绘GUI*/
static AM_ErrorCode_t gui_redraw(AM_GUI_t *gui)
{
	if(!gui->update_rect.w || !gui->update_rect.h)
		return AM_SUCCESS;
	
	if(gui->desktop && (gui->desktop->flags&AM_GUI_WIDGET_FL_VISIBLE))
	{
		gui_redraw_widget(gui->desktop, gui->surface, &gui->update_rect,
				gui->desktop->rect.x, gui->desktop->rect.y);
	}
	
	AM_OSD_Update(gui->osd_dev_no, NULL);
	
	gui->update_rect.w = 0;
	gui->update_rect.h = 0;
	return AM_SUCCESS;
}

/**\brief 处理GUI输入事件*/
static AM_ErrorCode_t gui_input(AM_GUI_t *gui, struct input_event *event)
{
	AM_GUI_WidgetEvent_t evt;
	AM_GUI_Widget_t *win, *f;
	AM_ErrorCode_t ret;
	
	if(event->value)
		evt.type = AM_GUI_WIDGET_EVT_KEY_DOWN;
	else
		evt.type = AM_GUI_WIDGET_EVT_KEY_UP;
	evt.input.input = *event;
	
	if(!gui->desktop)
		return AM_SUCCESS;
	
	/*Send the event to each window*/
	for(win=gui->desktop->child_head; win; win=win->next)
	{
		if(!AM_GUI_WINDOW(win)->focus)
			AM_GUI_WindowSetFocus(AM_GUI_WINDOW(win), NULL);
		
		if(AM_GUI_WINDOW(win)->focus)
		{
			/*Send the event to the focus*/
			for(f=AM_GUI_WINDOW(win)->focus; f && f!=win; f=f->parent)
			{
				ret = AM_GUI_WidgetSendEvent(f, &evt);
				if(ret==AM_SUCCESS)
					return ret;
			}
		}
		
		ret = AM_GUI_WidgetSendEvent(win, &evt);
		if(ret==AM_SUCCESS)
			return ret;
	}
	
	ret = AM_GUI_WidgetSendEvent(gui->desktop, &evt);
	
	return ret;
}

/**\brief GUI消息循环*/
static AM_ErrorCode_t gui_loop(AM_GUI_t *gui)
{
	AM_GUI_Timer_t *t, *nt;
	int now;
	int timeout = 0x7fffffff, diff;
	struct input_event event;
	AM_ErrorCode_t ret;
	
	while(gui->loop)
	{
		/*重绘屏幕*/
		gui_redraw(gui);
		
		/*计算超时时间*/
		AM_TIME_GetClock(&now);
		for(t=gui->timers; t; t=t->next)
		{
			if(t->delay!=-1)
				diff = t->start_time+t->delay-now;
			else
				diff = t->start_time+t->interval-now;
			
			if(diff<=0)
			{
				timeout = 0;
				break;
			}
			
			timeout = AM_MIN(timeout, diff);
		}
		
		if(timeout==0x7fffffff)
		{
			timeout = -1;
		}
		else
		{
			timeout = AM_MAX(timeout, GUI_INTERVAL);
		}
		
		/*等待输入事件*/
		ret = AM_INP_WaitEvent(gui->inp_dev_no, &event, timeout);
		if(ret==AM_SUCCESS)
		{
			if((event.value==1)&&(event.code==AM_INP_KEY_POWER))
				AM_GUI_Exit(gui);
			else
				gui_input(gui, &event);
		}
		
		/*检查定时器*/
		AM_TIME_GetClock(&now);
		for(t=gui->timers; t; t=nt)
		{
			nt = t->next;
			
			if(t->delay!=-1)
				diff = t->start_time+t->delay-now;
			else
				diff = t->start_time+t->interval-now;
			
			if(diff<=0)
			{
				if(t->cb)
					t->cb(t, t->data);
				t->delay = -1;
				
				if(t->times>0)
				{
					t->times--;
					if(t->times==0)
					{
						AM_GUI_TimerStop(t);
					}
				}
			}
		}
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 创建一个GUI管理器
 * \param[in] para GUI创建参数
 * \param[out] gui 返回新的GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_Create(const AM_GUI_CreatePara_t *para, AM_GUI_t **gui)
{
	AM_GUI_t *g;
	AM_ErrorCode_t ret;
	
	assert(para);
	
	g = (AM_GUI_t*)malloc(sizeof(AM_GUI_t));
	if(!g)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	memset(g, 0, sizeof(AM_GUI_t));
	
	ret = AM_OSD_GetSurface(para->osd_dev_no, &g->surface);
	if(ret!=AM_SUCCESS)
		goto error;
	
	g->osd_dev_no = para->osd_dev_no;
	g->inp_dev_no = para->inp_dev_no;
	g->update_rect.x = 0;
	g->update_rect.y = 0;
	g->update_rect.w = 0;
	g->update_rect.h = 0;
	
	ret = AM_GUI_ThemeInit(g);
	if(ret!=AM_SUCCESS)
		goto error;
	
	ret = AM_GUI_DesktopCreate(g, (AM_GUI_Desktop_t**)(&g->desktop));
	if(ret!=AM_SUCCESS)
		goto error;
	
	AM_GUI_WidgetShow(AM_GUI_WIDGET(g->desktop));
	
	*gui = g;
	
	return ret;
error:
	if(g)
		free(g);
	return ret;
}

/**\brief 释放一个GUI管理器
 * \param[in] gui GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_Destroy(AM_GUI_t *gui)
{
	assert(gui);
	
	if(gui->desktop)
		AM_GUI_WidgetDestroy(gui->desktop);
	
	AM_GUI_ThemeQuit(gui);
	
	free(gui);
	return AM_SUCCESS;
}

/**\brief 进入GUI管理器主循环
 * \param[in] gui GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_Main(AM_GUI_t *gui)
{
	assert(gui);
	
	gui->loop = AM_TRUE;
	
	gui_loop(gui);
	
	return AM_SUCCESS;
}

/**\brief 退出GUI管理器主循环
 * \param[in] gui GUI管理器 
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_Exit(AM_GUI_t *gui)
{
	assert(gui);
	
	gui->loop = AM_FALSE;
	return AM_SUCCESS;
}

/**\brief 取得GUI中的桌面控件
 * \param[in] gui GUI管理器
 * \param[out] desktop 返回桌面控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_GetDesktop(AM_GUI_t *gui, AM_GUI_Widget_t **desktop)
{
	assert(gui && desktop);
	
	*desktop = gui->desktop;
	return AM_SUCCESS;
}

