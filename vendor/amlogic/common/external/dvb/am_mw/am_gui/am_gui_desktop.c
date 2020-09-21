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
 * \brief 桌面控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_mem.h>
#include <assert.h>
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 桌面事件回调*/
static AM_ErrorCode_t desktop_event_cb (AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Desktop_t *d = (AM_GUI_Desktop_t*)widget;
	
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_DRAW_BEGIN:
			AM_OSD_DrawFilledRect(evt->draw.surface, NULL, d->bg_pixel);
		break;
		default:
		return AM_GUI_ERR_UNKNOWN_EVENT;
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 分配一个新桌面控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] d 返回新的桌面控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_DesktopCreate(AM_GUI_t *gui, AM_GUI_Desktop_t **d)
{
	AM_GUI_Desktop_t *dt;
	AM_ErrorCode_t ret;
	
	assert(gui && d);
	
	dt = (AM_GUI_Desktop_t*)malloc(sizeof(AM_GUI_Desktop_t));
	if(!dt)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_DesktopInit(gui, dt);
	if(ret!=AM_SUCCESS)
	{
		free(dt);
		return ret;
	}
	
	*d = dt;
	return AM_SUCCESS;
}

/**\brief 初始化一个桌面控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] d 桌面控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_DesktopInit(AM_GUI_t *gui, AM_GUI_Desktop_t *d)
{
	AM_ErrorCode_t ret;
	AM_OSD_Color_t col;
	
	assert(gui && d);
	
	ret = AM_GUI_WidgetInit(gui, AM_GUI_WIDGET(d));
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(d)->cb = desktop_event_cb;
	
	col.r = 0;
	col.g = 0;
	col.b = 0;
	col.a = 0;
	AM_OSD_MapColor(gui->surface->format, &col, &d->bg_pixel);
	
	AM_GUI_WIDGET(d)->rect.x = 0;
	AM_GUI_WIDGET(d)->rect.y = 0;
	AM_GUI_WIDGET(d)->rect.w = gui->surface->width;
	AM_GUI_WIDGET(d)->rect.h = gui->surface->height;
	
	return AM_SUCCESS;
}

/**\brief 释放一个桌面控件内部相关资源
 * \param[in] d 桌面控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_DesktopRelease(AM_GUI_Desktop_t *d)
{
	return AM_GUI_WidgetRelease(AM_GUI_WIDGET(d));
}

