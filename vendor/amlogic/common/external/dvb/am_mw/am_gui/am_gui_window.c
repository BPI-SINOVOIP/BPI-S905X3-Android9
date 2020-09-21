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
 * \brief 窗口控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-10: create the document
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

#define WIDGET_FOCUSABLE(w)\
	(((w)->flags&AM_GUI_WIDGET_FL_VISIBLE) &&\
	((w)->flags&AM_GUI_WIDGET_FL_FOCUSABLE) &&\
	!((w)->flags&AM_GUI_WIDGET_FL_DISABLED))

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief Find the focus in the widget tree*/
static AM_GUI_Widget_t* find_focus(AM_GUI_Widget_t *widget, AM_Bool_t reverse)
{
	AM_GUI_Widget_t *f, *c;
	
	if(widget->flags&AM_GUI_WIDGET_FL_DISABLED)
		return NULL;
	if(!(widget->flags&AM_GUI_WIDGET_FL_VISIBLE))
		return NULL;
	if(widget->flags&AM_GUI_WIDGET_FL_FOCUSABLE)
		return widget;
	
	if(reverse)
	{
		for(c=widget->child_tail; c; c=c->prev)
		{
			f = find_focus(c, reverse);
			if(f)
				return f;
		}
	}
	else
	{
		for(c=widget->child_head; c; c=c->next)
		{
			f = find_focus(c, reverse);
			if(f)
				return f;
		}
	}
	
	return NULL;
}

/**\brief Move the focus in the window*/
static AM_GUI_Widget_t* move_focus(AM_GUI_Window_t *win, int key)
{
	AM_GUI_Widget_t *p, *f;
	AM_GUI_Box_t *box;
	AM_GUI_Table_t *tab;
	AM_GUI_TableCell_t *cell;
	AM_GUI_Widget_t *c = NULL, *cc = NULL, *l = NULL, *w = NULL, *t;
	int row, col, off;
	
	if(!win->focus)
	{
		f = find_focus(AM_GUI_WIDGET(win), AM_FALSE);
		return f;
	}
	
	for(p=win->focus; p && (p!=AM_GUI_WIDGET(win)); p=p->parent)
	{
		switch(p->type)
		{
			case AM_GUI_WIDGET_BOX:
				box = AM_GUI_BOX(p);
				l   = p;
				if(((key==AM_INP_KEY_LEFT) || (key==AM_INP_KEY_RIGHT)) && (box->dir==AM_GUI_DIR_HORIZONTAL))
				{
					if(key==AM_INP_KEY_LEFT)
					{
						for(t=c->prev; t; t=t->prev)
						{
							if((f = find_focus(t, AM_TRUE)))
								return f;
						}
					}
					else
					{
						for(t=c->next; t; t=t->next)
						{
							if((f = find_focus(t, AM_FALSE)))
								return f;
						}
					}
				}
				else if(((key==AM_INP_KEY_UP) || (key==AM_INP_KEY_DOWN)) && (box->dir==AM_GUI_DIR_VERTICAL))
				{
					if(key==AM_INP_KEY_UP)
					{
						for(t=c->prev; t; t=t->prev)
						{
							if((f = find_focus(t, AM_TRUE)))
								return f;
						}
					}
					else
					{
						for(t=c->next; t; t=t->next)
						{
							if((f = find_focus(t, AM_FALSE)))
								return f;
						}
					}
				}
			break;
			case AM_GUI_WIDGET_TABLE:
				tab  = AM_GUI_TABLE(p);
				l    = p;
				w    = cc;
				cell = AM_GUI_TABLE_CELL(cc);
				if(!tab->items)
					return NULL;
				
				switch(key)
				{
					case AM_INP_KEY_UP:
						for(row=cell->row-1; row>=0; row--)
						{
							off = row*tab->col+cell->col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
					case AM_INP_KEY_DOWN:
						for(row=cell->row+1; row<tab->row; row++)
						{
							off = row*tab->col+cell->col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
					case AM_INP_KEY_LEFT:
						for(col=cell->col-1; col>=0; col--)
						{
							off = cell->row*tab->col+col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
					case AM_INP_KEY_RIGHT:
						for(col=cell->col+1; col<tab->col; col++)
						{
							off = cell->row*tab->col+col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
				}
			break;
			default:
			break;
		}
		
		cc = c;
		c  = p;
	}
	
	if(l)
	{
		switch(l->type)
		{
			case AM_GUI_WIDGET_BOX:
				box = AM_GUI_BOX(l);
				if(((key==AM_INP_KEY_LEFT) || (key==AM_INP_KEY_RIGHT)) && (box->dir==AM_GUI_DIR_HORIZONTAL))
				{
					if(key==AM_INP_KEY_LEFT)
					{
						for(t=l->child_tail; t; t=t->prev)
						{
							if((f = find_focus(t, AM_TRUE)))
								return f;
						}
					}
					else
					{
						for(t=l->child_head; t; t=t->next)
						{
							if((f = find_focus(t, AM_FALSE)))
								return f;
						}
					}
				}
				else if(((key==AM_INP_KEY_UP) || (key==AM_INP_KEY_DOWN)) && (box->dir==AM_GUI_DIR_VERTICAL))
				{
					if(key==AM_INP_KEY_UP)
					{
						for(t=l->child_tail; t; t=t->prev)
						{
							if((f = find_focus(t, AM_TRUE)))
								return f;
						}
					}
					else
					{
						for(t=l->child_head; t; t=t->next)
						{
							if((f = find_focus(t, AM_FALSE)))
								return f;
						}
					}
				}
			break;
			case AM_GUI_WIDGET_TABLE:
				tab  = AM_GUI_TABLE(l);
				cell = AM_GUI_TABLE_CELL(w);
				if(!tab->items)
					return NULL;
				
				switch(key)
				{
					case AM_INP_KEY_UP:
						for(row=tab->row-1; row>=0; row--)
						{
							off = row*tab->col+cell->col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
					case AM_INP_KEY_DOWN:
						for(row=0; row<tab->row; row++)
						{
							off = row*tab->col+cell->col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
					case AM_INP_KEY_LEFT:
						for(col=tab->col-1; col>=0; col--)
						{
							off = cell->row*tab->col+col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
					case AM_INP_KEY_RIGHT:
						for(col=0; col<tab->col; col++)
						{
							off = cell->row*tab->col+col;
							if(tab->items[off] && tab->items[off]!=AM_GUI_WIDGET(cell))
							{
								if((f = find_focus(tab->items[off], AM_TRUE)))
									return f;
							}
						}
					break;
				}
			break;
			default:
			break;
		}
	}
	
	return NULL;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 分配一个新窗口控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] win 返回新的窗口控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WindowCreate(AM_GUI_t *gui, AM_GUI_Window_t **win)
{
	AM_GUI_Window_t *w;
	AM_ErrorCode_t ret;
	
	assert(gui && win);
	
	w = (AM_GUI_Window_t*)malloc(sizeof(AM_GUI_Window_t));
	if(!w)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_WindowInit(gui, w);
	if(ret!=AM_SUCCESS)
	{
		free(w);
		return ret;
	}
	
	*win = w;
	return AM_SUCCESS;
}

/**\brief 初始化一个窗口控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] win 窗口控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WindowInit(AM_GUI_t *gui, AM_GUI_Window_t *win)
{
	AM_ErrorCode_t ret;
	
	assert(gui && win);
	
	ret = AM_GUI_WidgetInit(gui, AM_GUI_WIDGET(win));
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(win)->type = AM_GUI_WIDGET_WINDOW;
	AM_GUI_WIDGET(win)->cb   = AM_GUI_WindowEventCb;
	AM_GUI_WIDGET(win)->flags |= AM_GUI_WIDGET_FL_WINDOW;
	
	win->caption = NULL;
	win->focus   = NULL;
	
	return AM_SUCCESS;
}

/**\brief 释放一个窗口内部相关资源
 * \param[in] win 窗口控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WindowRelease(AM_GUI_Window_t *win)
{
	assert(win);
	
	AM_GUI_WidgetRelease(AM_GUI_WIDGET(win));
	
	return AM_SUCCESS;
}

/**\brief 窗口控件事件回调
 * \param[in] widget 窗口控件
 * \param[in] evt 要处理的事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WindowEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Window_t *win = AM_GUI_WINDOW(widget);
	AM_GUI_Widget_t *f;
	
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_KEY_DOWN:
			if((evt->input.input.code==AM_INP_KEY_UP) ||
					(evt->input.input.code==AM_INP_KEY_DOWN) ||
					(evt->input.input.code==AM_INP_KEY_LEFT) ||
					(evt->input.input.code==AM_INP_KEY_RIGHT))
			{
				f = move_focus(win, evt->input.input.code);
				if(f && f!=win->focus)
				{
					AM_GUI_WindowSetFocus(win, f);
					return AM_SUCCESS;
				}
			}
			return AM_GUI_ERR_UNKNOWN_EVENT;
		break;
		default:
		return AM_GUI_WidgetEventCb(widget, evt);
	}
	
	return AM_SUCCESS;
}

/**\brief 设定窗口标题
 * \param[in] win 窗口控件
 * \param caption 标题文本
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WindowSetCaption(AM_GUI_Window_t *win, const char *caption)
{
	AM_ErrorCode_t ret;
	
	assert(win);
	
	if(!caption)
	{
		if(win->caption)
		{
			AM_GUI_WidgetDestroy(AM_GUI_WIDGET(win->caption));
			win->caption = NULL;
		}
	}
	else
	{
		if(win->caption)
		{
			AM_GUI_LabelSetText(win->caption, caption);
		}
		else
		{
			ret = AM_GUI_TextLabelCreate(AM_GUI_WIDGET(win)->gui, caption, &win->caption);
			if(ret==AM_SUCCESS)
			{
				AM_GUI_WidgetAppendChild(AM_GUI_WIDGET(win), AM_GUI_WIDGET(win->caption));
				AM_GUI_WidgetShow(AM_GUI_WIDGET(win->caption));
			}
		}
	}
	
	return AM_SUCCESS;
}

/**\brief 设定窗口中的一个控件为当前焦点
 * \param[in] win 窗口控件
 * \param[in] focus 当前焦点控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_WindowSetFocus(AM_GUI_Window_t *win, AM_GUI_Widget_t *focus)
{
	AM_GUI_Widget_t *p, *f, *old = NULL;
	AM_GUI_WidgetEvent_t evt;
	
	assert(win);
	
	if(focus && (focus->flags&AM_GUI_WIDGET_FL_FOCUSED))
		return AM_SUCCESS;
	
	if(focus)
		f = find_focus(focus, AM_FALSE);
	else
		f = find_focus(AM_GUI_WIDGET(win), AM_FALSE);
	
	if(f)
	{
		evt.type = AM_GUI_WIDGET_EVT_ENTER;
		
		for(p=f; p && (p!=AM_GUI_WIDGET(win)); p=p->parent)
		{
			if(p->flags&AM_GUI_WIDGET_FL_FOCUSED)
			{
				old = p;
				break;
			}
			p->flags |= AM_GUI_WIDGET_FL_FOCUSED;
			AM_GUI_WidgetSendEvent(p, &evt);
		}
	}
	
	if(win->focus)
	{
		evt.type = AM_GUI_WIDGET_EVT_LEAVE;
		
		for(p=win->focus; p && (p!=AM_GUI_WIDGET(win)); p=p->parent)
		{
			if(p==old)
				break;
			
			p->flags &= ~AM_GUI_WIDGET_FL_FOCUSED;
			AM_GUI_WidgetSendEvent(p, &evt);
		}
	}
	
	win->focus = f;
	
	return AM_SUCCESS;
}

