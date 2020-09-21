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
 * \brief 滚动条控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-12-01: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_mem.h>
#include <string.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 滚动条事件回调
 * \param[in] widget 滚动条控件指针
 * \param[in] evt 要处理的控件事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ScrollEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Scroll_t *scroll = AM_GUI_SCROLL(widget);
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_GUI_ThemeAttr_t attr;
	
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_GET_W:
			AM_GUI_WidgetEventCb(widget, evt);
			if(scroll->dir==AM_GUI_DIR_HORIZONTAL)
			{
				AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_WIDTH, &attr);
				widget->min_w = attr.size;
				widget->max_w = attr.size;
				widget->prefer_w = attr.size;
			}
		break;
		case AM_GUI_WIDGET_EVT_GET_H:
			AM_GUI_WidgetEventCb(widget, evt);
			if(scroll->dir==AM_GUI_DIR_VERTICAL)
			{
				AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_HEIGHT, &attr);
				widget->max_h = attr.size;
				widget->prefer_h = attr.size;
			}
		break;
		default:
			ret = AM_GUI_WidgetEventCb(widget, evt);
	}
	
	return ret;
}

/**\brief 分配一个滚动条控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param dir 滚动条方向
 * \param[out] scroll 返回新的滚动条控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ScrollCreate(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Scroll_t **scroll)
{
	AM_GUI_Scroll_t *s;
	AM_ErrorCode_t ret;
	
	assert(gui && scroll);
	
	s = (AM_GUI_Scroll_t*)malloc(sizeof(AM_GUI_Scroll_t));
	if(!s)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_ScrollInit(gui, dir, s);
	if(ret!=AM_SUCCESS)
	{
		free(s);
		return ret;
	}
	
	*scroll = s;
	return AM_SUCCESS;
}

/**\brief 初始化一个滚动条控件
 * \param[in] gui 控件所属的GUI控制器
 * \param dir 滚动条方向
 * \param[in] scroll 滚动条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ScrollInit(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Scroll_t *scroll)
{
	AM_ErrorCode_t ret;
	AM_GUI_PosSize_t ps;
	AM_GUI_ThemeAttr_t attr;
	
	assert(gui && scroll);
	
	ret = AM_GUI_WidgetInit(gui, AM_GUI_WIDGET(scroll));
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(scroll)->type = AM_GUI_WIDGET_SCROLL;
	AM_GUI_WIDGET(scroll)->cb   = AM_GUI_ScrollEventCb;
	scroll->dir = dir;
	scroll->min = 0;
	scroll->max = 100;
	scroll->page  = 10;
	scroll->value = 0;
	
	AM_GUI_POS_SIZE_INIT(&ps);
	
	if(dir==AM_GUI_DIR_VERTICAL)
	{
		AM_GUI_ThemeGetAttr(AM_GUI_WIDGET(scroll), AM_GUI_THEME_ATTR_WIDTH, &attr);
		AM_GUI_POS_SIZE_WIDTH(&ps, attr.size);
		AM_GUI_POS_SIZE_CENTER(&ps, 0);
		AM_GUI_POS_SIZE_V_EXPAND(&ps);
	}
	else
	{
		AM_GUI_ThemeGetAttr(AM_GUI_WIDGET(scroll), AM_GUI_THEME_ATTR_HEIGHT, &attr);
		AM_GUI_POS_SIZE_HEIGHT(&ps, attr.size);
		AM_GUI_POS_SIZE_MIDDLE(&ps, 0);
		AM_GUI_POS_SIZE_H_EXPAND(&ps);
	}
	
	AM_GUI_WidgetSetPosSize(AM_GUI_WIDGET(scroll), &ps);
	
	return AM_SUCCESS;
}

/**\brief 释放一个滚动条控件内部相关资源
 * \param[in] scroll 滚动条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ScrollRelease(AM_GUI_Scroll_t *scroll)
{
	assert(scroll);
	
	AM_GUI_WidgetRelease(AM_GUI_WIDGET(scroll));
	
	return AM_SUCCESS;
}

/**\brief 设定滚动条的取值范围
 * \param[in] scroll 滚动条控件
 * \param min 滚动条的最小值
 * \param max 滚动条的最大值
 * \param page 滚动条的页值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ScrollSetRange(AM_GUI_Scroll_t *scroll, int min, int max, int page)
{
	assert(scroll && (max>=min) && (page<=(max-min)));
	
	if((min!=scroll->min) || (max!=scroll->max) || (page!=scroll->page))
	{
		scroll->min  = min;
		scroll->max  = max;
		scroll->page = page;
		
		AM_GUI_WidgetUpdate(AM_GUI_WIDGET(scroll), NULL);
	}
	
	return AM_SUCCESS;
}

/**\brief 设定滚动条的当前值
 * \param[in] scroll 滚动条控件
 * \param value 滚动条当前值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ScrollSetValue(AM_GUI_Scroll_t *scroll, int value)
{
	assert(scroll);
	
	value = AM_MAX(scroll->min, value);
	value = AM_MIN(scroll->max-scroll->page, value);
	
	if(value!=scroll->value)
	{
		scroll->value = value;
		
		AM_GUI_WidgetUpdate(AM_GUI_WIDGET(scroll), NULL);
	}
	
	return AM_SUCCESS;
}

