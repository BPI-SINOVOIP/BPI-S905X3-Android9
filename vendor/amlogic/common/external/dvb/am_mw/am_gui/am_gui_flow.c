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
 * \brief 流数据
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-09: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_mem.h>
#include <am_font.h>
#include <assert.h>
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief 向流中添加一个元素*/
static void flow_append_item(AM_GUI_Flow_t *flow, AM_GUI_FlowItem_t *item)
{
	item->next = NULL;
	if(flow->item_tail)
		flow->item_tail->next = item;
	else
		flow->item_head = item;
	flow->item_tail = item;
	
	flow->reset = AM_TRUE;
}

/**\brief 向流中添加一行*/
static AM_ErrorCode_t flow_append_row(AM_GUI_Flow_t *flow, AM_GUI_FlowItem_t *begin, int begin_pos, AM_GUI_FlowItem_t *end, int end_pos,
		int w, int h)
{
	AM_GUI_FlowRow_t *row;
	
	row = (AM_GUI_FlowRow_t*)malloc(sizeof(AM_GUI_FlowRow_t));
	if(!row)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	row->width = w;
	row->height= h;
	row->begin = begin;
	row->end   = end;
	row->begin_pos = begin_pos;
	row->end_pos   = end_pos;
	row->next  = NULL;
	
	if(flow->row_tail)
		flow->row_tail->next = row;
	else
		flow->row_head = row;
	flow->row_tail = row;
	
	return AM_SUCCESS;
}

/**\brief 绘制一行*/
static void flow_draw_row(AM_GUI_Flow_t *flow, AM_GUI_FlowItem_t *begin, int begin_pos, AM_GUI_FlowItem_t *end, int end_pos, AM_OSD_Surface_t *s,
		int x, int y, int row_height, int font_id, uint32_t pixel)
{
	AM_GUI_FlowItem_t *item;
	AM_GUI_FlowText_t *text;
	AM_GUI_FlowImage_t *img;
	AM_GUI_FlowSpace_t *sp;
	AM_FONT_TextInfo_t info;
	AM_FONT_TextPos_t pos;
	AM_OSD_Rect_t sr, dr;
	char *begin_ptr, *end_ptr;
	
	item=begin;
	while(1)
	{
		switch(item->type)
		{
			case AM_GUI_FLOW_ITEM_TEXT:
				text = (AM_GUI_FlowText_t*)item;
				begin_ptr  = text->text;
				end_ptr    = begin_ptr+text->len;
				info.max_width = 0;
				
				if(item==begin)
					begin_ptr = text->text+begin_pos;
				if(item==end)
					end_ptr = text->text+end_pos;
				
				if(AM_FONT_Size(font_id, begin_ptr, end_ptr-begin_ptr, &info)==AM_SUCCESS)
				{
					pos.rect.x = x;
					pos.rect.y = y;
					pos.rect.w = info.width;
					pos.rect.h = row_height;
					pos.base = info.descent;
					AM_FONT_Draw(font_id, s, begin_ptr, end_ptr-begin_ptr, &pos, pixel);
					x += info.width;
				}
			break;
			case AM_GUI_FLOW_ITEM_IMAGE:
				img = (AM_GUI_FlowImage_t*)item;
				sr.x = 0;
				sr.y = 0;
				sr.w = img->image->width;
				sr.h = img->image->height;
				dr.x = x;
				dr.y = y+row_height-img->height;
				dr.w = img->width;
				dr.h = img->height;
				AM_OSD_Blit(img->image, &sr, s, &dr, NULL);
				x += img->width;
			break;
			case AM_GUI_FLOW_ITEM_NEWLINE:
			break;
			case AM_GUI_FLOW_ITEM_SPACE:
				sp = (AM_GUI_FlowSpace_t*)item;
				x += sp->width;
			break;
		}
		if(item==end)
			break;
		item=item->next;
	}
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 初始化一个流对象
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowInit(AM_GUI_t *gui, AM_GUI_Flow_t *flow)
{
	assert(gui && flow);
	
	memset(flow, 0, sizeof(AM_GUI_Flow_t));
	
	flow->gui   = gui;
	flow->reset = AM_TRUE;
	flow->halign = AM_GUI_H_ALIGN_CENTER;
	flow->valign = AM_GUI_V_ALIGN_MIDDLE;
	
	return AM_SUCCESS;
}

/**\brief 释放一个流对象
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowRelease(AM_GUI_Flow_t *flow)
{
	return AM_GUI_FlowClear(flow);
}

/**\brief 向流对象中添加文本
 * \param[in] flow 流对象
 * \param[in] text 文本字符串
 * \param len 字符串长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowAppendText(AM_GUI_Flow_t *flow, const char *text, int len)
{
	AM_GUI_FlowText_t *t;
	
	assert(flow);
	
	if(!text || !len)
		return AM_SUCCESS;
	
	t = (AM_GUI_FlowText_t*)malloc(sizeof(AM_GUI_FlowText_t));
	if(!t)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	t->text = malloc(len);
	if(!t->text)
	{
		AM_DEBUG(1, "not enough memory");
		free(t);
		return AM_GUI_ERR_NO_MEM;
	}
	
	memcpy(t->text, text, len);
	t->item.type = AM_GUI_FLOW_ITEM_TEXT;
	t->len     = len;
	
	flow_append_item(flow, (AM_GUI_FlowItem_t*)t);
	
	return AM_SUCCESS;
}

/**\brief 向流对象中添加图片
 * \param[in] flow 流对象
 * \param[in] image 图片
 * \param width 图片显示宽度
 * \param height 图片显示高度
 * \param need_free 是否需要释放图片
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowAppendImage(AM_GUI_Flow_t *flow, AM_OSD_Surface_t *image, int width, int height, AM_Bool_t need_free)
{
	AM_GUI_FlowImage_t *img;
	
	assert(flow);
	
	if(!image)
		return AM_SUCCESS;
	
	img = (AM_GUI_FlowImage_t*)malloc(sizeof(AM_GUI_FlowImage_t));
	if(!img)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	img->image = image;
	img->width = (width>=0)?width:image->width;
	img->height= (height>=0)?height:image->height;
	img->need_free = need_free;
	img->item.type = AM_GUI_FLOW_ITEM_IMAGE;
	
	flow_append_item(flow, (AM_GUI_FlowItem_t*)img);
	
	return AM_SUCCESS;
}

/**\brief 向流对象中添加换行标志
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowAppendNewLine(AM_GUI_Flow_t *flow)
{
	AM_GUI_FlowItem_t *item;
	
	assert(flow);
	
	item = (AM_GUI_FlowItem_t*)malloc(sizeof(AM_GUI_FlowItem_t));
	if(!item)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	item->type = AM_GUI_FLOW_ITEM_NEWLINE;
	
	flow_append_item(flow, item);
	
	return AM_SUCCESS;
}

/**\brief 向流对象中添加空白区域
 * \param[in] flow 流对象
 * \param width 空白区域宽度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowAppendSpace(AM_GUI_Flow_t *flow, int width)
{
	AM_GUI_FlowSpace_t *sp;
	
	assert(flow);
	
	sp = (AM_GUI_FlowSpace_t*)malloc(sizeof(AM_GUI_FlowSpace_t));
	if(!sp)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	sp->item.type = AM_GUI_FLOW_ITEM_SPACE;
	sp->width = width;
	
	flow_append_item(flow, (AM_GUI_FlowItem_t*)sp);
	
	return AM_SUCCESS;
}

/**\brief 清除流对象中的数据
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowClear(AM_GUI_Flow_t *flow)
{
	AM_GUI_FlowItem_t *item, *inext;
	AM_GUI_FlowRow_t *row, *rnext;
	AM_GUI_FlowText_t *text;
	AM_GUI_FlowImage_t *img;
	
	assert(flow);
	
	/*释放元素链表*/
	for(item=flow->item_head; item; item=inext)
	{
		inext = item->next;
		
		/*释放元素*/
		switch(item->type)
		{
			case AM_GUI_FLOW_ITEM_TEXT:
				text = (AM_GUI_FlowText_t*)item;
				if(text->text)
					free(text->text);
			break;
			case AM_GUI_FLOW_ITEM_IMAGE:
				img = (AM_GUI_FlowImage_t*)item;
				if(img->image && img->need_free)
					AM_OSD_DestroySurface(img->image);
			break;
			case AM_GUI_FLOW_ITEM_NEWLINE:
			case AM_GUI_FLOW_ITEM_SPACE:
			default:
			break;
		}
		
		free(item);
		
		flow->reset = AM_TRUE;
	}
	
	/*释放行链表*/
	for(row=flow->row_head; row; row=rnext)
	{
		rnext = row->next;
		free(row);
	}
	
	flow->item_head = NULL;
	flow->item_tail = NULL;
	flow->row_head  = NULL;
	flow->row_tail  = NULL;
	
	return AM_SUCCESS;
}

/**\brief 设定流是否使用多行模式
 * \param[in] flow 流对象
 * \param multiline 是否使用多行模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowSetMultiline(AM_GUI_Flow_t *flow, AM_Bool_t multiline)
{
	assert(flow);
	
	if(flow->multiline!=multiline)
	{
		flow->multiline = multiline;
		flow->reset = AM_TRUE;
	}
	
	return AM_SUCCESS;
}

/**\brief 设定流对齐方式
 * \param[in] flow 流对象
 * \param halign 水平对齐方式
 * \param valign 垂直对齐方式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowSetAlignment(AM_GUI_Flow_t *flow, AM_GUI_HAlignment_t halign, AM_GUI_VAlignment_t valign)
{
	assert(flow);
	
	flow->halign = halign;
	flow->valign = valign;
	
	return AM_SUCCESS;
}

/**\brief 重新设定流对象的宽度，根据新宽度重新计算行信息
 * \param[in] flow 流对象
 * \param width 新设定流对象容器的宽度
 * \param font_id 流中文字的字体
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowResize(AM_GUI_Flow_t *flow, int width, int font_id)
{
	AM_GUI_FlowRow_t *row, *rnext;
	AM_GUI_FlowItem_t *item, *begin, *end;
	AM_GUI_FlowText_t *text;
	AM_GUI_FlowImage_t *img;
	AM_GUI_FlowSpace_t *sp;
	AM_FONT_TextInfo_t info;
	AM_FONT_Attr_t attr;
	char *ptr;
	int bytes;
	int min_w, max_w, height;
	int begin_pos, end_pos;
	
	assert(flow);
	
	if(flow->multiline && (flow->width!=width))
		flow->reset = AM_TRUE;
	
	if(!flow->reset)
		return AM_SUCCESS;
	
	
	/*释放行链表*/
	for(row=flow->row_head; row; row=rnext)
	{
		rnext = row->next;
		free(row);
	}
	
	flow->row_head = NULL;
	flow->row_tail = NULL;
	
	/*计算元素信息*/
	if(!flow->multiline)
		width = 0;
	
	memset(&info, 0, sizeof(info));
	
	flow->min_w = 0;
	flow->max_w = 0;
	flow->height = 0;
	
	min_w = 0;
	max_w = 0;
	height = 0;
	
	begin = flow->item_head;
	begin_pos = 0;
	
	for(item=flow->item_head; item; item=item->next)
	{
		switch(item->type)
		{
			case AM_GUI_FLOW_ITEM_TEXT:
				text = (AM_GUI_FlowText_t*)item;
				ptr  = text->text;
				bytes= text->len;
next_line:
				if(width>0)
				{
					int left = width-max_w;
					if(left<=0)
					{
						flow_append_row(flow, begin, begin_pos, end, end_pos, max_w, height);
						begin = item;
						begin_pos = ptr-text->text;
						end   = item;
						end_pos = begin_pos;
						
						flow->min_w = AM_MAX(flow->min_w, min_w);
						flow->max_w = AM_MAX(flow->max_w, max_w);
						flow->height += height;
						min_w = 0;
						max_w = 0;
						height = 0;
					}
					else
					{
						info.max_width = left;
					}
					info.stop_code = 13;
				}
				
				if(AM_FONT_Size(font_id, ptr, bytes, &info)==AM_SUCCESS)
				{
					AM_FONT_GetAttr(font_id, &attr);
					
					min_w = AM_MAX(min_w, attr.height?attr.height:attr.width);
					max_w += info.width;
					height = AM_MAX(height, attr.height);
					
					if(info.bytes!=bytes)
					{
						if(info.bytes)
						{
							end = item;
							end_pos = ptr+info.bytes-text->text;
						}
						
						flow_append_row(flow, begin, begin_pos, end, end_pos, max_w, height);
						
						begin = item;
						begin_pos = ptr+info.bytes-text->text;
						end   = item;
						end_pos   = begin_pos;
						
						flow->min_w = AM_MAX(flow->min_w, min_w);
						flow->max_w = AM_MAX(flow->max_w, max_w);
						flow->height += height;
						min_w = 0;
						max_w = 0;
						height = 0;
						ptr  += info.bytes;
						bytes-= info.bytes;
						goto next_line;
					}
				}
				end_pos = text->len;
			break;
			case AM_GUI_FLOW_ITEM_IMAGE:
				img = (AM_GUI_FlowImage_t*)item;
				if((width>0) && max_w && ((max_w+img->width)>width))
				{
					flow_append_row(flow, begin, begin_pos, end, end_pos, max_w, height);
					begin     = item;
					begin_pos = 0;
					
					flow->min_w = AM_MAX(flow->min_w, min_w);
					flow->max_w = AM_MAX(flow->max_w, max_w);
					flow->height += height;
					min_w = 0;
					max_w = 0;
					height = 0;
				}
				min_w = AM_MAX(min_w, img->width);
				max_w += img->width;
				height = AM_MAX(height, img->height);
				end_pos = 0;
			break;
			case AM_GUI_FLOW_ITEM_NEWLINE:
				if(width>0)
				{
					flow_append_row(flow, begin, begin_pos, end, end_pos, max_w, height);
					begin     = item;
					begin_pos = 0;
					
					flow->min_w = AM_MAX(flow->min_w, min_w);
					flow->max_w = AM_MAX(flow->max_w, max_w);
					flow->height += height;
					min_w = 0;
					max_w = 0;
					height = 0;
				}
				end_pos = 0;
			break;
			case AM_GUI_FLOW_ITEM_SPACE:
				sp = (AM_GUI_FlowSpace_t*)item;
				if((width>0) && max_w && ((max_w+sp->width)>width))
				{
					flow_append_row(flow, begin, begin_pos, end, end_pos, max_w, height);
					begin     = item;
					begin_pos = 0;
					
					flow->min_w = AM_MAX(flow->min_w, min_w);
					flow->max_w = AM_MAX(flow->max_w, max_w);
					flow->height += height;
					min_w = 0;
					max_w = 0;
					height = 0;
				}
				min_w = AM_MAX(min_w, sp->width);
				max_w += sp->width;
				end_pos = 0;
			break;
		}
		
		end = item;
	}
	
	if(width)
	{
		flow_append_row(flow, begin, begin_pos, end, end_pos, max_w, height);
	}
	
	flow->min_w = AM_MAX(flow->min_w, min_w);
	flow->max_w = AM_MAX(flow->max_w, max_w);
	flow->height += height;
	flow->width = width;
	flow->reset = AM_FALSE;
	
	return AM_SUCCESS;
}

/**\brief 绘制流对象
 * \param[in] flow 流对象
 * \param[in] s 绘图表面
 * \param[in] rect 绘制流对象的矩形
 * \param font_id 流中文字的字体
 * \param pixel 流中文字的颜色
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_FlowDraw(AM_GUI_Flow_t *flow, AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, int font_id, uint32_t pixel)
{
	AM_GUI_FlowRow_t *row;
	int x = 0, y = 0;
	
	assert(flow && s && rect);
	
	switch(flow->valign)
	{
		case AM_GUI_V_ALIGN_TOP:
			y = rect->y;
		break;
		case AM_GUI_V_ALIGN_BOTTOM:
			y = rect->h-flow->height+rect->y;
		break;
		case AM_GUI_V_ALIGN_MIDDLE:
			y = (rect->h-flow->height)/2+rect->y;
		break;
	}
	
	if(flow->multiline)
	{
		for(row=flow->row_head; row; row=row->next)
		{
			switch(flow->halign)
			{
				case AM_GUI_H_ALIGN_LEFT:
					x = rect->x;
				break;
				case AM_GUI_H_ALIGN_RIGHT:
					x = rect->w-row->width+rect->x;
				break;
				case AM_GUI_H_ALIGN_CENTER:
					x = (rect->w-row->width)/2+rect->x;
				break;
			}
			flow_draw_row(flow, row->begin, row->begin_pos, row->end, row->end_pos, s, x, y, row->height, font_id, pixel);
			y += row->height;
		}
	}
	else
	{
		int end_pos = 0;
		
		if(flow->item_tail->type==AM_GUI_FLOW_ITEM_TEXT)
		{
			end_pos = ((AM_GUI_FlowText_t*)flow->item_tail)->len;
		}
		
		switch(flow->halign)
		{
			case AM_GUI_H_ALIGN_LEFT:
				x = rect->x;
			break;
			case AM_GUI_H_ALIGN_RIGHT:
				x = rect->w-flow->max_w+rect->x;
			break;
			case AM_GUI_H_ALIGN_CENTER:
				x = (rect->w-flow->max_w)/2+rect->x;
			break;
		}
		
		flow_draw_row(flow, flow->item_head, 0, flow->item_tail, end_pos, s, x, y, flow->height, font_id, pixel);
	}
	
	return AM_SUCCESS;
}

