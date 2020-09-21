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
 * \brief 表格布局器
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-09: create the document
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

/**\brief 表格事件回调*/
static AM_ErrorCode_t table_event_cb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Table_t *tab = AM_GUI_TABLE(widget);
	AM_GUI_Widget_t *row;
	AM_GUI_TableCell_t *cell;
	AM_GUI_ThemeAttr_t attr;
	int c, max, tw, th, ch, y;
	
	/*分配并初始化单元格表*/
	if(!tab->items)
	{
		int r, i, j;
		
		tab->row = AM_MAX(tab->row, tab->curr_row);
		if(tab->row && tab->col)
		{
			tab->items = (AM_GUI_Widget_t**)malloc(tab->col*tab->row*sizeof(AM_GUI_Widget_t*));
			tab->min_size = (int*)malloc(sizeof(int)*tab->col);
			tab->max_size = (int*)malloc(sizeof(int)*tab->col);
		
			if(!tab->items || !tab->min_size || !tab->max_size)
			{
				AM_DEBUG(1, "not enough memory");
				if(tab->items)
					free(tab->items);
				if(tab->min_size)
					free(tab->min_size);
				if(tab->max_size)
					free(tab->max_size);
			
				return AM_GUI_ERR_NO_MEM;
			}
		
			memset(tab->min_size, 0, sizeof(int)*tab->col);
			memset(tab->max_size, 0, sizeof(int)*tab->col);
			memset(tab->items, 0, sizeof(AM_GUI_Widget_t*)*tab->col*tab->row);
		
			row = AM_GUI_WIDGET(tab)->child_head;
			for(r=0; r<tab->row; r++)
			{
				for(cell = AM_GUI_TABLE_CELL(row->child_head);
						cell;
						cell=AM_GUI_TABLE_CELL(AM_GUI_WIDGET(cell)->next))
				{
					for(i=0; i<cell->hspan; i++)
						for(j=0; j<cell->vspan; j++)
							tab->items[(r+j)*tab->col+cell->col+i] = AM_GUI_WIDGET(cell);
				}
			
				row = row->next;
			}
		}
	}
	
	/*处理事件*/
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
				int max_pw = 0, min_pw = 0, prefer_pw = 0;
			
				for(row = AM_GUI_WIDGET(tab)->child_head; row; row=row->next)
				{
					for(cell = AM_GUI_TABLE_CELL(row->child_head);
						cell;
						cell=AM_GUI_TABLE_CELL(AM_GUI_WIDGET(cell)->next))
					{
						int i;
						
						c = cell->col;
						for(i=0; i<cell->hspan; c++,i++)
						{
							tab->min_size[c] = AM_MAX(AM_GUI_WIDGET(cell)->min_w/cell->hspan, tab->min_size[c]);
							tab->max_size[c] = AM_MAX(AM_GUI_WIDGET(cell)->prefer_w/cell->hspan, tab->max_size[c]);
						}
					
						if((AM_GUI_WIDGET(cell)->ps.mask&AM_GUI_POS_SIZE_FL_WIDTH_PERCENT) && AM_GUI_WIDGET(cell)->ps.width)
						{
							min_pw = AM_MAX(min_pw, AM_GUI_WIDGET(cell)->min_w*100/AM_GUI_WIDGET(cell)->ps.width);
							max_pw = AM_MAX(max_pw, AM_GUI_WIDGET(cell)->max_w*100/AM_GUI_WIDGET(cell)->ps.width);
							prefer_pw = AM_MAX(prefer_pw, AM_GUI_WIDGET(cell)->prefer_w*100/AM_GUI_WIDGET(cell)->ps.width);
						}
					}
				}
			
				widget->min_w = 0;
				widget->max_w = 0;
				widget->prefer_w = 0;
				max = 0;
				for(c=0; c<tab->col; c++)
				{
					widget->min_w += tab->min_size[c];
					widget->prefer_w += tab->max_size[c];
					max = AM_MAX(max, tab->max_size[c]);
				}
				max *= tab->col;
				
				widget->min_w    = AM_MAX(widget->min_w, min_pw);
				widget->max_w    = AM_MAX(max, max_pw);
				widget->prefer_w = AM_MAX(widget->prefer_w, prefer_pw);
				
				if(tab->col)
				{
					AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_H_SEP, &attr);
					widget->max_w += attr.size*(tab->col-1);
					widget->min_w += attr.size*(tab->col-1);
					widget->prefer_w += attr.size*(tab->col-1);
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
				int max_ph = 0, prefer_ph = 0;
			
				widget->max_h = 0;
				max = 0;
			
				for(row = AM_GUI_WIDGET(tab)->child_head; row; row=row->next)
				{
					row->max_h = 0;
					row->prefer_h = 0;
					for(cell = AM_GUI_TABLE_CELL(row->child_head);
						cell;
						cell=AM_GUI_TABLE_CELL(AM_GUI_WIDGET(cell)->next))
					{
						row->prefer_h = AM_MAX(row->prefer_h, AM_GUI_WIDGET(cell)->prefer_h/cell->vspan);
						row->max_h = AM_MAX(row->max_h, AM_GUI_WIDGET(cell)->prefer_h/cell->vspan);
					}
					widget->prefer_h += row->prefer_h;
					max = AM_MAX(max, row->prefer_h);
				
					if((row->ps.mask&AM_GUI_POS_SIZE_FL_HEIGHT_PERCENT) && row->ps.height)
					{
						max_ph = AM_MAX(max_ph, row->max_h*100/row->ps.height);
						prefer_ph = AM_MAX(prefer_ph, row->prefer_h*100/row->ps.height);
					}
				}
				widget->prefer_h = AM_MAX(widget->prefer_h, prefer_ph);
				widget->max_h = AM_MAX(max*tab->row, max_ph);
				
				if(tab->row)
				{
					AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_V_SEP, &attr);
					widget->max_h += attr.size*(tab->row-1);
					widget->prefer_h += attr.size*(tab->row-1);
				}
			}
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_W:
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_H_SEP, &attr);
			
			for(row = AM_GUI_WIDGET(tab)->child_head; row; row=row->next)
			{
				int i, x, w, cw;
				
				tw = AM_GUI_WIDGET(tab)->rect.w;
				x  = 0;
				cw = tab->col?widget->max_w/tab->col:widget->max_w;
				
				row->rect.x = x;
				row->rect.w = widget->rect.w;
				
				for(cell = AM_GUI_TABLE_CELL(row->child_head);
					cell;
					cell=AM_GUI_TABLE_CELL(AM_GUI_WIDGET(cell)->next))
				{
					c = cell->col;
					w = 0;
					
					for(i=0; i<cell->hspan; i++,c++)
					{
						if(tw>=widget->max_w)
						{
							w += cw+(tw-widget->max_w)/tab->col; 
						}
						else if(tw>=widget->prefer_w)
						{
							w += tab->max_size[c]+(tw-widget->prefer_w)/tab->col;
						}
						else if(tw>=widget->min_w)
						{
							w += tab->min_size[c]+(tw-widget->min_w)/tab->col;
						}
						else
						{
							w += tab->min_size[c];
						}
					}
					
					AM_GUI_WIDGET(cell)->rect.x = x;
					AM_GUI_WIDGET(cell)->rect.w = w;
					
					x += w+attr.size;
				}
			}
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_H:
			y  = 0;
			th = AM_GUI_WIDGET(tab)->rect.h;
			ch = tab->row?AM_GUI_WIDGET(tab)->max_h/tab->row:AM_GUI_WIDGET(tab)->max_h;
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_V_SEP, &attr);
			
			for(row = AM_GUI_WIDGET(tab)->child_head; row; row=row->next)
			{
				int h;
				
				if(th>=widget->max_h)
				{
					h = ch+(th-widget->max_h)/tab->row;
				}
				else if(th>=widget->prefer_h)
				{
					h = row->prefer_h+(th-widget->prefer_h)/tab->row;
				}
				else
				{
					h = row->prefer_h;
				}
				
				row->rect.y = y;
				row->rect.h = h;
				
				y += h+attr.size;
			}
			
			for(row = AM_GUI_WIDGET(tab)->child_head; row; row=row->next)
			{
				for(cell = AM_GUI_TABLE_CELL(row->child_head);
					cell;
					cell=AM_GUI_TABLE_CELL(AM_GUI_WIDGET(cell)->next))
				{
					AM_GUI_Widget_t *tr;
					int i, h;
					
					h  = 0;
					tr = row;
					for(i=0; i<cell->vspan; i++) {
						h += tr->rect.h;
						tr = tr->next;
					}
					AM_GUI_WIDGET(cell)->rect.y = 0;
					AM_GUI_WIDGET(cell)->rect.h = h;
				}
			}
		break;
		case AM_GUI_WIDGET_EVT_DESTROY:
			AM_GUI_TableRelease(tab);
		break;
		default:
		return AM_GUI_ERR_UNKNOWN_EVENT;
	}
	
	return AM_SUCCESS;
}

/**\brief 创建一个单元格控件*/
static AM_ErrorCode_t table_cell_create(AM_GUI_Table_t *tab, AM_GUI_Widget_t **cell, int row, int col, int hspan, int vspan)
{
	AM_GUI_TableCell_t *c;
	AM_ErrorCode_t ret;
	
	c = (AM_GUI_TableCell_t*)malloc(sizeof(AM_GUI_TableCell_t));
	if(!cell)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_WidgetInit(AM_GUI_WIDGET(tab)->gui, AM_GUI_WIDGET(c));
	if(ret!=AM_SUCCESS)
	{
		free(c);
		return ret;
	}
	
	AM_GUI_WIDGET(c)->type = AM_GUI_WIDGET_TABLE_CELL;
	
	c->row   = row;
	c->col   = col;
	c->hspan = hspan;
	c->vspan = vspan;
	
	*cell = AM_GUI_WIDGET(c);
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 分配一个新表格控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] tab 返回新的表格控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TableCreate(AM_GUI_t *gui, AM_GUI_Table_t **tab)
{
	AM_GUI_Table_t *t;
	AM_ErrorCode_t ret;
	
	assert(gui && tab);
	
	t = (AM_GUI_Table_t*)malloc(sizeof(AM_GUI_Table_t));
	if(!t)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_TableInit(gui, t);
	if(ret!=AM_SUCCESS)
	{
		free(t);
		return ret;
	}
	
	*tab = t;
	return AM_SUCCESS;
}

/**\brief 初始化一个表格控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] tab 表格控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TableInit(AM_GUI_t *gui, AM_GUI_Table_t *tab)
{
	AM_ErrorCode_t ret;
	
	assert(gui && tab);
	
	ret = AM_GUI_WidgetInit(gui, AM_GUI_WIDGET(tab));
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(tab)->cb = table_event_cb;
	AM_GUI_WIDGET(tab)->type = AM_GUI_WIDGET_TABLE;
	
	tab->curr_row = 0;
	tab->row = 0;
	tab->col = 0;
	tab->items = NULL;
	tab->min_size = NULL;
	tab->max_size = NULL;
	
	return AM_SUCCESS;
}

/**\brief 释放一个表格控件内部相关资源
 * \param[in] tab 表格控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TableRelease(AM_GUI_Table_t *tab)
{
	assert(tab);
	
	if(tab->items)
		free(tab->items);
	if(tab->min_size)
		free(tab->min_size);
	if(tab->max_size)
		free(tab->max_size);
	
	AM_GUI_WidgetRelease(AM_GUI_WIDGET(tab));
	
	return AM_SUCCESS;
}

/**\brief 向表格中添加一行
 * \param[in] tab 表格控件指针
 * \param flags 行高度标志
 * \param height 行占用的高度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TableAppendRow(AM_GUI_Table_t *tab, int flags, int height)
{
	AM_GUI_Widget_t *row;
	AM_GUI_PosSize_t ps;
	AM_ErrorCode_t ret;
	
	assert(tab);
	
	ret = AM_GUI_WidgetCreate(AM_GUI_WIDGET(tab)->gui, &row);
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(row)->type = AM_GUI_WIDGET_TABLE_ROW;
	AM_GUI_WIDGET(row)->cb   = NULL;
	
	AM_GUI_POS_SIZE_INIT(&ps);
	
	if(flags&AM_GUI_TABLE_ROW_FL_EXPAND)
		AM_GUI_POS_SIZE_V_EXPAND(&ps);
	else
		AM_GUI_POS_SIZE_V_SHRINK(&ps);
	
	if(flags&AM_GUI_TABLE_ROW_FL_SIZE)
	{
		if(flags&AM_GUI_TABLE_ROW_FL_PERCENT)
			AM_GUI_POS_SIZE_HEIGHT_PERCENT(&ps, height);
		else
			AM_GUI_POS_SIZE_HEIGHT(&ps, height);
	}
	
	AM_GUI_WidgetSetPosSize(row, &ps);
	
	AM_GUI_WidgetAppendChild(AM_GUI_WIDGET(tab), row);
	AM_GUI_WidgetShow(row);
	
	tab->curr_row++;
	
	return AM_SUCCESS;
}

/**\brief 向表格中增加一个单元格
 * \param[in] tab 表格控件指针
 * \param[in] widget 添加的控件
 * \param flags 控件宽度标志
 * \param width 控件占用的宽度
 * \param col 控件开始的列数
 * \param hspan 控件占用的列数
 * \param vspan 控件所占的行数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TableAppendCell(AM_GUI_Table_t *tab, AM_GUI_Widget_t *widget, int flags, int width, int col, int hspan, int vspan)
{
	AM_GUI_Widget_t *row;
	AM_GUI_Widget_t *cell;
	AM_GUI_PosSize_t ps;
	AM_ErrorCode_t ret;
	
	assert(tab && widget);
	
	row = AM_GUI_WIDGET(tab)->child_tail;
	if(!row)
	{
		AM_DEBUG(1, "no row in the table");
		return AM_FAILURE;
	}
	
	ret = table_cell_create(tab, &cell, tab->curr_row-1, col, hspan, vspan);
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_POS_SIZE_INIT(&ps);
	
	if(flags&AM_GUI_TABLE_CELL_FL_EXPAND)
		AM_GUI_POS_SIZE_H_EXPAND(&ps);
	else
		AM_GUI_POS_SIZE_H_SHRINK(&ps);
	
	if(flags&AM_GUI_TABLE_CELL_FL_SIZE)
	{
		if(flags&AM_GUI_TABLE_CELL_FL_PERCENT)
			AM_GUI_POS_SIZE_WIDTH_PERCENT(&ps, width);
		else
			AM_GUI_POS_SIZE_WIDTH(&ps, width);
	}
	
	AM_GUI_WidgetSetPosSize(cell, &ps);
	
	AM_GUI_WidgetAppendChild(cell, widget);
	AM_GUI_WidgetAppendChild(row, cell);
	AM_GUI_WidgetShow(cell);
	
	tab->row = AM_MAX(tab->curr_row+vspan-1, tab->row);
	tab->col = AM_MAX(tab->col, col+hspan);
	
	return AM_SUCCESS;
}

