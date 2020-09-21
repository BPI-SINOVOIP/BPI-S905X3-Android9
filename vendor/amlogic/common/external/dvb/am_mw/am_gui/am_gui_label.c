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
 * \brief 标签控件
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
 * API functions
 ***************************************************************************/

/**\brief 标签控件事件回调
 * \param[in] widget 标签控件
 * \param[in] evt 事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_LabelEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_ThemeAttr_t attr;
	AM_GUI_Label_t *lab = AM_GUI_LABEL(widget);
	AM_OSD_Rect_t r;
	int font_id;
	uint32_t pixel;
	
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_GET_W:
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_FONT, &attr);
			font_id = attr.font_id;
			
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_BORDER, &attr);
			
			AM_GUI_FlowResize(&lab->flow, 0, font_id);
			widget->min_w = lab->flow.min_w+attr.border.left+attr.border.right;
			widget->max_w = lab->flow.max_w+attr.border.left+attr.border.right;
			widget->prefer_w = lab->flow.max_w+attr.border.left+attr.border.right;
		break;
		case AM_GUI_WIDGET_EVT_GET_H:
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_BORDER, &attr);
			
			widget->max_h = lab->flow.height+attr.border.top+attr.border.bottom;
			widget->prefer_h = lab->flow.height+attr.border.top+attr.border.bottom;
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_W:
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_FONT, &attr);
			font_id = attr.font_id;
			
			AM_GUI_FlowResize(&lab->flow, widget->rect.w, font_id);
		break;
		case AM_GUI_WIDGET_EVT_LAY_OUT_H:
		break;
		case AM_GUI_WIDGET_EVT_DRAW_BEGIN:
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_FONT, &attr);
			font_id = attr.font_id;
			
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_TEXT_COLOR, &attr);
			pixel = attr.pixel;
			
			AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_BORDER, &attr);
			
			r.x = evt->draw.org_x;
			r.y = evt->draw.org_y;
			r.w = widget->rect.w;
			r.h = widget->rect.h;
			
			r.x += attr.border.left;
			r.y += attr.border.top;
			r.w -= attr.border.left+attr.border.right;
			r.h -= attr.border.top+attr.border.bottom;
			
			AM_GUI_FlowDraw(&lab->flow, evt->draw.surface, &r, font_id, pixel);
		break;
		case AM_GUI_WIDGET_EVT_DESTROY:
			AM_GUI_LabelRelease(lab);
		break;
		default:
		return AM_GUI_WidgetEventCb(widget, evt);
	}
	
	return AM_SUCCESS;
}

/**\brief 分配一个新标签控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] lab 返回新的标签控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_LabelCreate(AM_GUI_t *gui, AM_GUI_Label_t **lab)
{
	AM_GUI_Label_t *l;
	AM_ErrorCode_t ret;
	
	assert(gui && lab);
	
	l = (AM_GUI_Label_t*)malloc(sizeof(AM_GUI_Label_t));
	if(!l)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_LabelInit(gui, l);
	if(ret!=AM_SUCCESS)
	{
		free(l);
		return ret;
	}
	
	*lab = l;
	return AM_SUCCESS;
}

/**\brief 初始化一个标签控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] lab 标签控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_LabelInit(AM_GUI_t *gui, AM_GUI_Label_t *lab)
{
	AM_ErrorCode_t ret;
	
	assert(gui && lab);
	
	ret = AM_GUI_WidgetInit(gui, AM_GUI_WIDGET(lab));
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(lab)->type = AM_GUI_WIDGET_LABEL;
	AM_GUI_WIDGET(lab)->cb   = AM_GUI_LabelEventCb;
	
	AM_GUI_FlowInit(AM_GUI_WIDGET(lab)->gui, &lab->flow);
	
	return AM_SUCCESS;
}

/**\brief 释放一个标签控件内部相关资源
 * \param[in] lab 标签控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_LabelRelease(AM_GUI_Label_t *lab)
{
	assert(lab);
	
	AM_GUI_FlowRelease(&lab->flow);
	AM_GUI_WidgetRelease(AM_GUI_WIDGET(lab));
	
	return AM_SUCCESS;
}

/**\brief 设定标签控件的文本
 * \param[in] lab 标签控件
 * \param[in] text 文本字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_LabelSetText(AM_GUI_Label_t *lab, const char *text)
{
	AM_ErrorCode_t ret;
	
	assert(lab);
	
	AM_GUI_FlowClear(&lab->flow);
	
	if(text)
	{
		ret = AM_GUI_FlowAppendText(&lab->flow, text, strlen(text));
	}
	
	return ret;
}

/**\brief 设定标签控件的图片
 * \param[in] lab 标签控件
 * \param[in] image 图片数据
 * \param need_free 控件释放时是否释放图片
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_LabelSetImage(AM_GUI_Label_t *lab, AM_OSD_Surface_t *image, AM_Bool_t need_free)
{
	AM_ErrorCode_t ret;
	
	assert(lab);
	
	AM_GUI_FlowClear(&lab->flow);
	
	if(image)
	{
		ret = AM_GUI_FlowAppendImage(&lab->flow, image, 0, 0, need_free);
	}
	
	return ret;
}

/**\brief 设定标签的多行显示模式
 * \param[in] lab 标签控件
 * \param multiline 是否使用多行显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_LabelSetMultiline(AM_GUI_Label_t *lab, AM_Bool_t multiline)
{
	assert(lab);
	
	return AM_GUI_FlowSetMultiline(&lab->flow, multiline);
}

/**\brief 分配一个文字标签控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] text 文字数据
 * \param[out] lab 返回新的标签控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TextLabelCreate(AM_GUI_t *gui, const char *text, AM_GUI_Label_t **lab)
{
	AM_GUI_Label_t *l;
	AM_ErrorCode_t ret;
	
	ret = AM_GUI_LabelCreate(gui, &l);
	if(ret!=AM_SUCCESS)
		return ret;
	
	ret = AM_GUI_LabelSetText(l, text);
	if(ret!=AM_SUCCESS)
	{
		AM_GUI_WidgetDestroy(AM_GUI_WIDGET(l));
		return ret;
	}
	
	*lab = l;
	return AM_SUCCESS;
}

/**\brief 分配一个图片标签控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] image 图片
 * \param[out] lab 返回新的标签控件
 * \param need_free 释放控件时是否释放图片
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ImageLabelCreate(AM_GUI_t *gui, AM_OSD_Surface_t *image, AM_GUI_Label_t **lab, AM_Bool_t need_free)
{
	AM_GUI_Label_t *l;
	AM_ErrorCode_t ret;
	
	ret = AM_GUI_LabelCreate(gui, &l);
	if(ret!=AM_SUCCESS)
		return ret;
	
	ret = AM_GUI_LabelSetImage(l, image, need_free);
	if(ret!=AM_SUCCESS)
	{
		AM_GUI_WidgetDestroy(AM_GUI_WIDGET(l));
		return ret;
	}
	
	*lab = l;
	return AM_SUCCESS;
}

