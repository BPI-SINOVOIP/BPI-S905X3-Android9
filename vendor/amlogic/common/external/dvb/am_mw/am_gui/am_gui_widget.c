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
 * \brief GUI基本控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_mem.h>
#include "../../am_adp/am_osd/am_osd_internal.h"
#include <string.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 设定控件的重绘标志*/
static void widget_update(AM_GUI_Widget_t *widget, AM_OSD_Rect_t *rect);

/**\brief 检测控件是否是窗口焦点，如果是，重新设定*/
static void widget_reset_focus(AM_GUI_Widget_t *widget)
{
	AM_GUI_Widget_t *p;
	AM_GUI_WidgetEvent_t evt;
	
	if(!(widget->flags&AM_GUI_WIDGET_FL_FOCUSED))
		return;
	
	evt.type = AM_GUI_WIDGET_EVT_LEAVE;
	for(p=widget; p && (p->parent!=widget->gui->desktop); p=p->parent)
	{
		p->flags &= ~AM_GUI_WIDGET_FL_FOCUSED;
		AM_GUI_WidgetSendEvent(p, &evt);
	}
	
	if(p->flags&AM_GUI_WIDGET_FL_WINDOW)
	{
		AM_GUI_WindowSetFocus(AM_GUI_WINDOW(p), NULL);
	}
}

/**\brief 将控件从控件树中移除*/
static void widget_remove(AM_GUI_Widget_t *widget)
{
	AM_GUI_Widget_t *p = widget->parent;
	
	if(widget->prev)
		widget->prev->next = widget->next;
	if(widget->next)
		widget->next->prev = widget->prev;
	if(widget->parent)
	{
		if(widget==widget->parent->child_head)
			widget->parent->child_head = widget->next;
		if(widget==widget->parent->child_tail)
			widget->parent->child_tail = widget->prev;
	}
	
	widget->parent = NULL;
	widget->prev   = NULL;
	widget->next   = NULL;
	
	if(p)
	{
		widget_reset_focus(p);
		widget_update(p, &widget->rect);
	}
}

/**\brief 释放控件树*/
static void widget_destroy(AM_GUI_Widget_t *w)
{
	AM_GUI_Widget_t *c, *nc;
	AM_GUI_WidgetEvent_t evt;
	
	evt.type = AM_GUI_WIDGET_EVT_DESTROY;
	AM_GUI_WidgetSendEvent(w, &evt);
	
	for(c=w->child_head; c; c=nc)
	{
		nc = c->next;
		widget_destroy(c);
	}
	
	free(w);
}

/**\brief 设定控件的重绘标志*/
static void widget_update(AM_GUI_Widget_t *widget, AM_OSD_Rect_t *rect)
{
	AM_GUI_t *gui = widget->gui;
	AM_GUI_Widget_t *p;
	AM_OSD_Rect_t real_rect, pr;
	
	if(!rect)
	{
		rect = &real_rect;
		rect->x = 0;
		rect->y = 0;
		rect->w = widget->rect.w;
		rect->h = widget->rect.h;
	}
	else
	{
		real_rect = *rect;
		rect = &real_rect;
	}
	
	for(p=widget; p; p=p->parent)
	{
		p->flags |= AM_GUI_WIDGET_FL_UPDATE;
		
		pr.x = 0;
		pr.y = 0;
		pr.w = p->rect.w;
		pr.h = p->rect.h;
		
		if(!AM_OSD_ClipRect(&pr, rect, rect))
			return;
		
		rect->x += p->rect.x;
		rect->y += p->rect.y;
		
		if(!(p->flags&AM_GUI_WIDGET_FL_VISIBLE))
			return;
	}
	
	if(!gui->update_rect.w || !gui->update_rect.h)
	{
		gui->update_rect = *rect;
	}
	else
	{
		AM_OSD_CollectRect(rect, &gui->update_rect, &gui->update_rect);
	}
}

/**\brief 计算控件树中每个控件的宽度*/
static void widget_get_w(AM_GUI_Widget_t *w)
{
	AM_GUI_WidgetEvent_t evt;
	AM_GUI_Widget_t *c;
	
	for(c=w->child_head; c; c=c->next)
	{
		widget_get_w(c);
	}
	
	evt.type = AM_GUI_WIDGET_EVT_GET_W;
	AM_GUI_WidgetSendEvent(w, &evt);
}

/**\brief 计算控件树中每个控件的高度*/
static void widget_get_h(AM_GUI_Widget_t *w)
{
	AM_GUI_WidgetEvent_t evt;
	AM_GUI_Widget_t *c;
	
	for(c=w->child_head; c; c=c->next)
	{
		widget_get_h(c);
	}
	
	evt.type = AM_GUI_WIDGET_EVT_GET_H;
	AM_GUI_WidgetSendEvent(w, &evt);
}

/**\brief 设定一个控件的宽度*/
static void widget_set_w(AM_GUI_Widget_t *widget)
{
	AM_GUI_Widget_t *p = widget->parent;
	AM_GUI_ThemeAttr_t attr;
	int x = 0, l = 0, r = 0, pw = p?p->rect.w:widget->max_w, w;
	int set = 0;
	
	if(p)
	{
		AM_GUI_ThemeGetAttr(p, AM_GUI_THEME_ATTR_BORDER, &attr);
		pw -= attr.border.left+attr.border.right;
		x   = attr.border.left;
	}
	
	w = pw;
	
	if(widget->ps.mask&AM_GUI_POS_SIZE_FL_LEFT)
	{
		l = (widget->ps.mask&AM_GUI_POS_SIZE_FL_LEFT_PERCENT)?
			pw*widget->ps.left/100:widget->ps.left;
		set++;
	}
	if(widget->ps.mask&AM_GUI_POS_SIZE_FL_RIGHT)
	{
		r = (widget->ps.mask&AM_GUI_POS_SIZE_FL_RIGHT_PERCENT)?
			pw*widget->ps.right/100:widget->ps.right;
		set++;
	}
	if(widget->ps.mask&AM_GUI_POS_SIZE_FL_WIDTH)
	{
		w = (widget->ps.mask&AM_GUI_POS_SIZE_FL_WIDTH_PERCENT)?
			pw*widget->ps.width/100:widget->ps.width;
		set++;
	}
	else if(set==2)
	{
		w = pw-l-r;
	}
	else
	{
		if(pw>=widget->max_w)
			w = (widget->ps.mask&AM_GUI_POS_SIZE_FL_H_EXPAND)?pw:widget->max_w;
		else if(pw>=widget->prefer_w)
			w = (widget->ps.mask&AM_GUI_POS_SIZE_FL_H_EXPAND)?pw:widget->prefer_w;
		else
			w = pw;
	}
	
	if((widget->ps.mask&AM_GUI_POS_SIZE_FL_CENTER) || !set)
	{
		widget->rect.x = (pw-w)/2+l+x;
		widget->rect.w = w;
		return;
	}
	
	widget->rect.x = x + l;
	widget->rect.w = w;
}

/**\brief 设定一个控件的高度*/
static void widget_set_h(AM_GUI_Widget_t *widget)
{
	AM_GUI_Widget_t *p = widget->parent;
	AM_GUI_ThemeAttr_t attr;
	int y = 0, t = 0, b = 0, ph = p?p->rect.h:widget->max_h, h;
	int set = 0;
	
	if(p)
	{
		AM_GUI_ThemeGetAttr(p, AM_GUI_THEME_ATTR_BORDER, &attr);
		ph -= attr.border.top+attr.border.bottom;
		y   = attr.border.top;
	}
	
	h = ph;
	
	if(widget->ps.mask&AM_GUI_POS_SIZE_FL_TOP)
	{
		t = (widget->ps.mask&AM_GUI_POS_SIZE_FL_TOP_PERCENT)?
			ph*widget->ps.top/100:widget->ps.top;
		set++;
	}
	if(widget->ps.mask&AM_GUI_POS_SIZE_FL_BOTTOM)
	{
		b = (widget->ps.mask&AM_GUI_POS_SIZE_FL_BOTTOM_PERCENT)?
			ph*widget->ps.bottom/100:widget->ps.bottom;
		set++;
	}
	if(widget->ps.mask&AM_GUI_POS_SIZE_FL_HEIGHT)
	{
		h = (widget->ps.mask&AM_GUI_POS_SIZE_FL_HEIGHT_PERCENT)?
			ph*widget->ps.height/100:widget->ps.height;
		set++;
	}
	else if(set==2)
	{
		h = ph-t-b;
	}
	else
	{
		if(ph>=widget->max_h)
			h = (widget->ps.mask&AM_GUI_POS_SIZE_FL_V_EXPAND)?ph:widget->max_h;
		else if (ph>=widget->prefer_h)
			h = (widget->ps.mask&AM_GUI_POS_SIZE_FL_V_EXPAND)?ph:widget->prefer_h;
		else
			h = ph;
	}
	
	if((widget->ps.mask&AM_GUI_POS_SIZE_FL_MIDDLE) || !set)
	{
		widget->rect.y = (ph-h)/2+t+y;
		widget->rect.h = h;
		return;
	}
	
	widget->rect.y = y + t;
	widget->rect.h = h;
}

/**\brief 设定控件的宽度*/
static void widget_lay_out_w(AM_GUI_Widget_t *w)
{
	AM_GUI_WidgetEvent_t evt;
	AM_GUI_Widget_t *c;
	
	evt.type = AM_GUI_WIDGET_EVT_LAY_OUT_W;
	AM_GUI_WidgetSendEvent(w, &evt);
	
	for(c=w->child_head; c; c=c->next)
	{
		widget_lay_out_w(c);
	}
}

/**\brief 设定控件高度*/
static void widget_lay_out_h(AM_GUI_Widget_t *w)
{
	AM_GUI_WidgetEvent_t evt;
	AM_GUI_Widget_t *c;
	
	evt.type = AM_GUI_WIDGET_EVT_LAY_OUT_H;
	AM_GUI_WidgetSendEvent(w, &evt);
	
	for(c=w->child_head; c; c=c->next)
	{
		widget_lay_out_h(c);
	}
	
	AM_GUI_ThemeLayOut(w);
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 分配一个新控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] widget 返回新的控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetCreate(AM_GUI_t *gui, AM_GUI_Widget_t **widget)
{
	AM_GUI_Widget_t *w;
	AM_ErrorCode_t ret;
	
	assert(gui && widget);
	
	w = (AM_GUI_Widget_t*)malloc(sizeof(AM_GUI_Widget_t));
	if(!w)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_WidgetInit(gui, w);
	if(ret!=AM_SUCCESS)
	{
		free(w);
		return ret;
	}
	
	*widget = w;
	return AM_SUCCESS;
}

/**\brief 初始化一个控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetInit(AM_GUI_t *gui, AM_GUI_Widget_t *widget)
{
	assert(gui && widget);
	
	widget->type   = AM_GUI_WIDGET_UNKNOWN;
	widget->theme_id = 0;
	widget->gui    = gui;
	widget->parent = NULL;
	widget->prev   = NULL;
	widget->next   = NULL;
	widget->child_head = NULL;
	widget->child_tail = NULL;
	widget->rect.x = 0;
	widget->rect.y = 0;
	widget->rect.w = 0;
	widget->rect.h = 0;
	widget->flags  = 0;
	widget->cb     = NULL;
	widget->user_cb= NULL;
	widget->min_w  = 0;
	widget->max_w  = 0;
	widget->max_h  = 0;
	widget->prefer_w = 0;
	widget->prefer_h = 0;
	widget->cb  = AM_GUI_WidgetEventCb;
	
	AM_GUI_POS_SIZE_INIT(&widget->ps);
	
	return AM_SUCCESS;
}

/**\brief 释放一个控件内部相关资源
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetRelease(AM_GUI_Widget_t *widget)
{
	assert(widget);
	
	return AM_SUCCESS;
}

/**\brief 释放一个控件内部资源并释放内存
 * \param[in] widget 要释放的控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetDestroy(AM_GUI_Widget_t *widget)
{
	assert(widget);
	
	widget_remove(widget);
	widget_destroy(widget);
	
	return AM_SUCCESS;
}

/**\brief 将一个控件加入父控件中
 * \param[in] parent 父控件
 * \param[in] child 子控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetAppendChild(AM_GUI_Widget_t *parent, AM_GUI_Widget_t *child)
{
	assert(child);
	
	if(!parent)
		parent = child->gui->desktop;
	
	if((parent==parent->gui->desktop) && !AM_GUI_WidgetIsWindow(child))
	{
		AM_DEBUG(1, "cannot append widget which is not a window to the desktop");
		return AM_FAILURE;
	}
	
	widget_remove(child);
	
	child->parent = parent;
	if(parent->child_head)
	{
		child->prev = parent->child_tail;
		parent->child_tail->next = child;
		parent->child_tail = child;
	}
	else
	{
		parent->child_head = child;
		parent->child_tail = child;
	}
	
	return AM_SUCCESS;
}

/**\brief 显示一个控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetShow(AM_GUI_Widget_t *widget)
{
	AM_GUI_WidgetEvent_t evt;
	
	assert(widget);
	
	if(!(widget->flags&AM_GUI_WIDGET_FL_VISIBLE))
	{
		widget->flags |= AM_GUI_WIDGET_FL_VISIBLE;
		
		evt.type = AM_GUI_WIDGET_EVT_SHOW;
		AM_GUI_WidgetSendEvent(widget, &evt);
		
		widget_update(widget, NULL);
	}
	
	return AM_SUCCESS;
}

/**\brief 隐藏一个控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetHide(AM_GUI_Widget_t *widget)
{
	AM_GUI_WidgetEvent_t evt;
	
	assert(widget);
	
	if(widget->flags&AM_GUI_WIDGET_FL_VISIBLE)
	{
		widget->flags &= ~AM_GUI_WIDGET_FL_VISIBLE;
		
		evt.type = AM_GUI_WIDGET_EVT_SHOW;
		AM_GUI_WidgetSendEvent(widget, &evt);
		
		if(widget->parent)
			widget_update(widget->parent, &widget->rect);
		
		widget_reset_focus(widget);
	}
	
	return AM_SUCCESS;
}

/**\brief 使能控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetEnable(AM_GUI_Widget_t *widget)
{
	AM_GUI_WidgetEvent_t evt;
	
	assert(widget);
	
	if(widget->flags&AM_GUI_WIDGET_FL_DISABLED)
	{
		widget->flags &= ~AM_GUI_WIDGET_FL_DISABLED;
		
		evt.type = AM_GUI_WIDGET_EVT_ENABLE;
		AM_GUI_WidgetSendEvent(widget, &evt);
		
		widget_update(widget, NULL);
	}
	
	return AM_SUCCESS;
}

/**\brief 禁用控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetDisable(AM_GUI_Widget_t *widget)
{
	AM_GUI_WidgetEvent_t evt;
	
	assert(widget);
	
	if(!(widget->flags&AM_GUI_WIDGET_FL_DISABLED))
	{
		widget->flags |= AM_GUI_WIDGET_FL_DISABLED;
		
		evt.type = AM_GUI_WIDGET_EVT_DISABLE;
		AM_GUI_WidgetSendEvent(widget, &evt);
		
		widget_update(widget, NULL);
		
		widget_reset_focus(widget);
	}
	
	return AM_SUCCESS;
}

/**\brief 设定控件的位置和长宽信息
 * \param[in] widget 控件指针
 * \param[in] ps 位置和长宽信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetSetPosSize(AM_GUI_Widget_t *widget, const AM_GUI_PosSize_t *ps)
{
	assert(widget && ps);
	
	widget->ps = *ps;
	
	return AM_SUCCESS;
}

/**\brief 重新设定子控件布局
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetLayOut(AM_GUI_Widget_t *widget)
{
	assert(widget);
	
	if(!widget->parent)
		return AM_FAILURE;
	
	widget_get_w(widget);
	//AM_DEBUG(1, "GETW %d %d %d", widget->min_w, widget->prefer_w, widget->max_w);
	
	widget_set_w(widget);
	//AM_DEBUG(1, "SETW %d %d", widget->rect.x, widget->rect.w);
	
	widget_lay_out_w(widget);
	
	widget_get_h(widget);
	//AM_DEBUG(1, "GETH %d %d", widget->prefer_h, widget->max_h);
	
	widget_set_h(widget);
	//AM_DEBUG(1, "SETH %d %d", widget->rect.y, widget->rect.h);
	
	widget_lay_out_h(widget);
	
	return AM_SUCCESS;
}

/**\brief 控件缺省事件回调
 * \param[in] widget 控件
 * \param[in] evt 要处理的事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Widget_t *c;
	int max, min, prefer, pmax, pmin, pprefer;
	AM_GUI_ThemeAttr_t attr;
	
	assert(widget && evt);
	
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_GET_W:
			if((widget->ps.mask&AM_GUI_POS_SIZE_FL_WIDTH) && !(widget->ps.mask&AM_GUI_POS_SIZE_FL_WIDTH_PERCENT))
			{
				/*固定宽度*/
				widget->min_w = widget->max_w = widget->prefer_w = widget->ps.width;
			}
			else
			{
				max = 0;
				min = 0;
				prefer = 0;
				pmax   = 0;
				pmin   = 0;
				pprefer= 0;
				for(c=widget->child_head; c; c=c->next)
				{
					max = AM_MAX(c->max_w, max);
					min = AM_MAX(c->min_w, min);
					prefer = AM_MAX(c->prefer_w, prefer);
					if((c->ps.mask&AM_GUI_POS_SIZE_FL_WIDTH_PERCENT) && c->ps.width)
					{
						pmin = AM_MAX(pmin, c->ps.width*c->min_w/100);
						pmax = AM_MAX(pmax, c->ps.width*c->max_w/100);
						pprefer = AM_MAX(pprefer, c->ps.width*c->prefer_w/100);
					}
				}
				
				AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_BORDER, &attr);
				widget->min_w = AM_MAX(min+attr.border.left+attr.border.right, pmin);
				widget->max_w = AM_MAX(max+attr.border.left+attr.border.right, pmax);
				widget->prefer_w = AM_MAX(prefer+attr.border.left+attr.border.right, pprefer);
			}
		break;
		case AM_GUI_WIDGET_EVT_GET_H:
			if((widget->ps.mask&AM_GUI_POS_SIZE_FL_HEIGHT) && !(widget->ps.mask&AM_GUI_POS_SIZE_FL_HEIGHT_PERCENT))
			{
				/*固定高度*/
				widget->max_h = widget->prefer_h = widget->ps.height;
			}
			else
			{
				max = 0;
				prefer  = 0;
				pmax    = 0;
				pprefer = 0;
				for(c=widget->child_head; c; c=c->next)
				{
					max = AM_MAX(c->max_h, max);
					prefer = AM_MAX(c->prefer_h, prefer);
					if((c->ps.mask&AM_GUI_POS_SIZE_FL_HEIGHT_PERCENT) && c->ps.height)
					{
						pmax = AM_MAX(pmax, c->ps.height*c->max_h/100);
						pprefer = AM_MAX(pprefer, c->ps.height*c->prefer_h/100);
					}
				}
				
				AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_BORDER, &attr);
				
				widget->max_h = AM_MAX(max+attr.border.top+attr.border.bottom, pmax);
				widget->prefer_h = AM_MAX(prefer+attr.border.top+attr.border.bottom, pprefer);
			}
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_W:
			for(c=widget->child_head; c; c=c->next)
			{
				widget_set_w(c);
			}
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_H:
			for(c=widget->child_head; c; c=c->next)
			{
				widget_set_h(c);
			}
		break;
		case AM_GUI_WIDGET_EVT_DESTROY:
			AM_GUI_WidgetRelease(widget);
		break;
		case AM_GUI_WIDGET_EVT_DRAW_BEGIN:
			AM_GUI_ThemeDrawBegin(widget, evt->draw.surface, evt->draw.org_x, evt->draw.org_y);
		break;
		case AM_GUI_WIDGET_EVT_DRAW_END:
			AM_GUI_ThemeDrawEnd(widget, evt->draw.surface, evt->draw.org_x, evt->draw.org_y);
		break;
		default:
		return AM_GUI_ERR_UNKNOWN_EVENT;
	}
	
	return AM_SUCCESS;
}

/**\brief 设定用户事件回调
 * \param[in] widget 控件指针
 * \param cb 回调函数指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetSetUserCallback(AM_GUI_Widget_t *widget, AM_GUI_WidgetEventCallback_t cb)
{
	assert(widget);
	
	widget->user_cb = cb;
	return AM_SUCCESS;
}

/**\brief 重新绘制控件
 * \param[in] widget 控件指针
 * \param[in] rect 需要重绘的区域
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WidgetUpdate(AM_GUI_Widget_t *widget, AM_OSD_Rect_t *rect)
{
	assert(widget);
	
	widget_update(widget, rect);
	return AM_SUCCESS;
}

