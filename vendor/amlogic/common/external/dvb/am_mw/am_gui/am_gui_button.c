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
 * \brief 按钮控件
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


/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 分配一个新按钮控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] btn 返回新的按钮控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ButtonCreate(AM_GUI_t *gui, AM_GUI_Button_t **btn)
{
	AM_GUI_Button_t *b;
	AM_ErrorCode_t ret;
	
	assert(gui && btn);
	
	b = (AM_GUI_Button_t*)malloc(sizeof(AM_GUI_Button_t));
	if(!b)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_ButtonInit(gui, b);
	if(ret!=AM_SUCCESS)
	{
		free(b);
		return ret;
	}
	
	*btn = b;
	return AM_SUCCESS;
}

/**\brief 初始化一个按钮控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] btn 按钮控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ButtonInit(AM_GUI_t *gui, AM_GUI_Button_t *btn)
{
	AM_ErrorCode_t ret;
	
	assert(gui && btn);
	
	ret = AM_GUI_LabelInit(gui, AM_GUI_LABEL(btn));
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(btn)->type = AM_GUI_WIDGET_BUTTON;
	AM_GUI_WIDGET(btn)->cb   = AM_GUI_ButtonEventCb;
	AM_GUI_WIDGET(btn)->flags |= AM_GUI_WIDGET_FL_FOCUSABLE;
	
	return AM_SUCCESS;
}

/**\brief 释放一个按钮控件内部相关资源
 * \param[in] btn 按钮控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ButtonRelease(AM_GUI_Button_t *btn)
{
	assert(btn);
	
	AM_GUI_LabelRelease(AM_GUI_LABEL(btn));
	
	return AM_SUCCESS;
}

/**\brief 按钮事件回调
 * \param[in] widget 按钮控件指针
 * \param[in] evt 要处理的控件事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ButtonEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Button_t *btn = AM_GUI_BUTTON(widget);
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_ENTER:
		case AM_GUI_WIDGET_EVT_LEAVE:
			AM_GUI_WidgetUpdate(widget, NULL);
		break;
		case AM_GUI_WIDGET_EVT_KEY_DOWN:
			if(evt->input.input.code==AM_INP_KEY_OK)
			{
				AM_GUI_WidgetEvent_t pevt;
				
				pevt.type = AM_GUI_WIDGET_EVT_PRESSED;
				AM_GUI_WidgetSendEvent(widget, &pevt);
			}
			else
				return AM_GUI_ERR_UNKNOWN_EVENT;
		break;
		case AM_GUI_WIDGET_EVT_DRAW_BEGIN:
			AM_GUI_ThemeDrawBegin(widget, evt->draw.surface, evt->draw.org_x, evt->draw.org_y);
			AM_GUI_LabelEventCb(widget, evt);
		break;
		case AM_GUI_WIDGET_EVT_DESTROY:
			AM_GUI_ButtonRelease(btn);
		break;
		default:
			ret = AM_GUI_LabelEventCb(widget, evt);
	}
	
	return ret;
}

/**\brief 设定按钮控件显示的标题
 * \param[in] btn 按钮控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ButtonSetText(AM_GUI_Button_t *btn, const char *text)
{
	return AM_GUI_LabelSetText(AM_GUI_LABEL(btn), text);
}

/**\brief 分配一个带文本标题的按钮控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] text 按钮的标题
 * \param[out] btn 返回新的按钮控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_TextButtonCreate(AM_GUI_t *gui, const char *text, AM_GUI_Button_t **btn)
{
	AM_GUI_Button_t *b;
	AM_ErrorCode_t ret;
	
	ret = AM_GUI_ButtonCreate(gui, &b);
	if(ret!=AM_SUCCESS)
		return ret;
	
	ret = AM_GUI_ButtonSetText(b, text);
	if(ret!=AM_SUCCESS)
	{
		AM_GUI_WidgetDestroy(AM_GUI_WIDGET(b));
		return ret;
	}
	
	*btn = b;
	return AM_SUCCESS;
}

