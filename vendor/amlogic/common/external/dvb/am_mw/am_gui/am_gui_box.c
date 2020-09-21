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
 * \brief 布局框控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
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
 * Static functions
 ***************************************************************************/
static AM_ErrorCode_t box_event_cb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Box_t *box = AM_GUI_BOX(widget);
	AM_GUI_ThemeAttr_t attr;
	AM_GUI_Widget_t *c;
	int sep;
	
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
				int tmin = 0, tmax = 0, max = 0, cnt = 0, ecnt = 0, sw = 0;
				
				/*计算子控件宽度*/
				if(box->dir==AM_GUI_DIR_VERTICAL)
				{
					for(c=widget->child_head; c; c=c->next)
					{
						tmin = AM_MAX(tmin, c->min_w);
						tmax = AM_MAX(tmax, c->prefer_w);
					}
					
					widget->min_w = tmin;
					widget->max_w = widget->prefer_w = tmax;
				}
				else
				{
					int min_pw = 0, max_pw = 0;
					
					for(c=widget->child_head; c; c=c->next)
					{
						tmin += c->min_w;
						tmax += c->prefer_w;
						
						if((c->ps.mask&AM_GUI_POS_SIZE_FL_WIDTH_PERCENT) && c->ps.width)
						{
							min_pw = AM_MAX(min_pw, c->min_w*100/c->ps.width);
							max_pw = AM_MAX(max_pw, c->prefer_w*100/c->ps.width);
						}
						
						cnt++;
						
						if(c->ps.mask&AM_GUI_POS_SIZE_FL_H_EXPAND)
						{
							max = AM_MAX(max, c->prefer_w);
							ecnt++;
						}
						else
						{
							sw += c->prefer_w;
						}
					}
					
					max = max*ecnt+sw;
					
					if(cnt)
					{
						AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_H_SEP, &attr);
						tmin += (cnt-1)*attr.size;
						max  += (cnt-1)*attr.size;
						tmax += (cnt-1)*attr.size;
					}
					
					widget->min_w = AM_MAX(tmin, min_pw);
					widget->max_w = AM_MAX(max, max_pw);
					widget->prefer_w = AM_MAX(tmax, max_pw);
				}
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
				int tmax = 0, max = 0, cnt = 0, ecnt = 0, sw = 0;
				
				/*计算子控件高度*/
				if(box->dir==AM_GUI_DIR_VERTICAL)
				{
					int max_ph = 0, prefer_ph = 0;
					
					for(c=widget->child_head; c; c=c->next)
					{
						tmax += c->prefer_h;
						
						if((c->ps.mask&AM_GUI_POS_SIZE_FL_HEIGHT_PERCENT) && c->ps.height)
						{
							max_ph = AM_MAX(max_ph, c->max_h*100/c->ps.height);
							prefer_ph = AM_MAX(prefer_ph, c->prefer_h*100/c->ps.height);
						}
						
						cnt++;
						
						if(c->ps.mask&AM_GUI_POS_SIZE_FL_V_EXPAND)
						{
							max = AM_MAX(c->prefer_h, max);
							ecnt++;
						}
						else
						{
							sw += c->prefer_h;
						}
					}
					
					max = max*ecnt+sw;
					
					if(cnt)
					{
						AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_V_SEP, &attr);
						max  += (cnt-1)*attr.size;
						tmax += (cnt-1)*attr.size;
					}
					
					widget->prefer_h = AM_MAX(tmax, prefer_ph);
					widget->max_h = AM_MAX(max, max_ph);
				}
				else
				{
					for(c=widget->child_head; c; c=c->next)
					{
						tmax = AM_MAX(tmax, c->prefer_h);
					}
					
					widget->max_h = widget->prefer_h = tmax;
				}
			}
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_W:
			/*设定子控件的宽度*/
			if(box->dir==AM_GUI_DIR_VERTICAL)
			{
				int x, w;
				
				x = 0;
				w = widget->rect.w;
				
				for(c=widget->child_head; c; c=c->next)
				{
					c->rect.x = x;
					c->rect.w = w;
				}
			}
			else
			{
				int x, w, lw, ecnt = 0, pw = 0, mw = 0;
				
				AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_H_SEP, &attr);
				sep = attr.size;
				x = 0;
				w = widget->rect.w;
				lw = w;
				
				for(c=widget->child_head; c; c=c->next)
				{
					if(!(c->ps.mask&AM_GUI_POS_SIZE_FL_H_EXPAND))
					{
						if(w >= widget->prefer_w)
							c->rect.w = c->prefer_w;
						else
							c->rect.w = c->min_w;
						
						lw -= c->rect.w;
					}
					else
					{
						pw += c->prefer_w;
						mw += c->min_w;
						ecnt++;
					}
				}
				
				for(c=widget->child_head; c; c=c->next)
				{
					c->rect.x = x;
					if(c->ps.mask&AM_GUI_POS_SIZE_FL_H_EXPAND)
					{
						if(w >= widget->max_w)
							c->rect.w = lw/ecnt;
						else if(w >= widget->prefer_w)
							c->rect.w = c->prefer_w+(lw-pw)/ecnt;
						else if(w >= widget->min_w)
							c->rect.w = c->min_w+(lw-mw)/ecnt;
						else
							c->rect.w = c->min_w;
					}
					x += c->rect.w+sep;
				}
			}
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_H:
			/*设定子控件高度*/
			if(box->dir==AM_GUI_DIR_VERTICAL)
			{
				int y, h, lh, ecnt = 0, ph = 0;
				
				AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_V_SEP, &attr);
				sep = attr.size;
				y = 0;
				h = widget->rect.h;
				lh = h;
				
				for(c=widget->child_head; c; c=c->next)
				{
					if(!(c->ps.mask&AM_GUI_POS_SIZE_FL_V_EXPAND))
					{
						c->rect.h = c->prefer_h;
						lh -= c->rect.h;
					}
					else
					{
						ph += c->prefer_h;
						ecnt++;
					}
				}
				
				for(c=widget->child_head; c; c=c->next)
				{
					c->rect.y = y;
					if(c->ps.mask&AM_GUI_POS_SIZE_FL_V_EXPAND)
					{
						if(h >= widget->max_h)
							c->rect.h = lh/ecnt;
						else if(h >= widget->prefer_h)
							c->rect.h = c->prefer_h+(lh-ph)/ecnt;
						else
							c->rect.h = c->prefer_h;
					}
					y += c->rect.h+sep;
				}
			}
			else
			{
				int y, h;
				
				y = 0;
				h = widget->rect.h;
				
				for(c=widget->child_head; c; c=c->next)
				{
					c->rect.y = y;
					c->rect.h = h;
				}
			}
		break;
		case AM_GUI_WIDGET_EVT_DESTROY:
			AM_GUI_BoxRelease(box);
		break;
		default:
		return AM_GUI_ERR_UNKNOWN_EVENT;
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 分配一个新布局框控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] box 返回新的布局框控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_BoxCreate(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Box_t **box)
{
	AM_GUI_Box_t *b;
	AM_ErrorCode_t ret;
	
	assert(gui && box);
	
	b = (AM_GUI_Box_t*)malloc(sizeof(AM_GUI_Box_t));
	if(!b)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_BoxInit(gui, dir, b);
	if(ret!=AM_SUCCESS)
	{
		free(b);
		return ret;
	}
	
	*box = b;
	
	return AM_SUCCESS;
}

/**\brief 初始化一个布局框控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] box 布局框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_BoxInit(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Box_t *box)
{
	AM_ErrorCode_t ret;
	
	assert(gui && box);
	
	ret = AM_GUI_WidgetInit(gui, AM_GUI_WIDGET(box));
	if(ret!=AM_SUCCESS)
		return ret;
	
	box->dir   = dir;
	
	AM_GUI_WIDGET(box)->type = AM_GUI_WIDGET_BOX;
	AM_GUI_WIDGET(box)->cb = box_event_cb;
	
	return AM_SUCCESS;
}

/**\brief 释放一个布局框控件内部相关资源
 * \param[in] widget 布局框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_BoxRelease(AM_GUI_Box_t *box)
{
	assert(box);
	
	return AM_GUI_WidgetRelease(AM_GUI_WIDGET(box));
}

/**\brief 向布局框中添加一个控件
 * \param[in] box 布局框控件指针
 * \param[in] widget 添加的控件
 * \param flags 控件宽度标志
 * \param size 子控件的大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_BoxAppend(AM_GUI_Box_t *box, AM_GUI_Widget_t *widget, int flags, int size)
{
	AM_GUI_Widget_t *cell;
	AM_GUI_PosSize_t ps;
	AM_ErrorCode_t ret;
	
	assert(box && widget);
	
	ret = AM_GUI_WidgetCreate(AM_GUI_WIDGET(box)->gui, &cell);
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(cell)->type = AM_GUI_WIDGET_BOX_CELL;
	
	AM_GUI_POS_SIZE_INIT(&ps);
	if(box->dir==AM_GUI_DIR_VERTICAL)
	{
		if(flags&AM_GUI_BOX_FL_EXPAND)
			AM_GUI_POS_SIZE_V_EXPAND(&ps);
		else
			AM_GUI_POS_SIZE_V_SHRINK(&ps);
		if(flags&AM_GUI_BOX_FL_SIZE)
		{
			if(flags&AM_GUI_BOX_FL_PERCENT)
				AM_GUI_POS_SIZE_HEIGHT_PERCENT(&ps, size);
			else
				AM_GUI_POS_SIZE_HEIGHT(&ps, size);
		}
	}
	else
	{
		if(flags&AM_GUI_BOX_FL_EXPAND)
			AM_GUI_POS_SIZE_H_EXPAND(&ps);
		else
			AM_GUI_POS_SIZE_H_SHRINK(&ps);
		if(flags&AM_GUI_BOX_FL_SIZE)
		{
			if(flags&AM_GUI_BOX_FL_PERCENT)
				AM_GUI_POS_SIZE_HEIGHT_PERCENT(&ps, size);
			else
				AM_GUI_POS_SIZE_HEIGHT(&ps, size);
		}
	}
	
	AM_GUI_WidgetSetPosSize(cell, &ps);
	
	AM_GUI_WidgetAppendChild(AM_GUI_WIDGET(box), cell);
	AM_GUI_WidgetAppendChild(cell, widget);
	AM_GUI_WidgetShow(cell);
	
	return AM_SUCCESS;
}

