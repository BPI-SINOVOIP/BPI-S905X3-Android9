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
 * \brief 进度条控件
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

/**\brief 进度条事件回调
 * \param[in] widget 进度条控件指针
 * \param[in] evt 要处理的控件事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ProgressEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	AM_GUI_Progress_t *prog = AM_GUI_PROGRESS(widget);
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_GUI_ThemeAttr_t attr;
	
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_GET_W:
			AM_GUI_WidgetEventCb(widget, evt);
			if(prog->dir==AM_GUI_DIR_HORIZONTAL)
			{
				AM_GUI_ThemeGetAttr(widget, AM_GUI_THEME_ATTR_WIDTH, &attr);
				widget->min_w = attr.size;
				widget->max_w = attr.size;
				widget->prefer_w = attr.size;
			}
		break;
		case AM_GUI_WIDGET_EVT_GET_H:
			AM_GUI_WidgetEventCb(widget, evt);
			if(prog->dir==AM_GUI_DIR_VERTICAL)
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

/**\brief 分配一个新进度条控件并初始化
 * \param[in] gui 进度条控件所属的GUI控制器
 * \param dir 进度条显示方向
 * \param[out] prog 返回新的进度条控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ProgressCreate(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Progress_t **prog)
{
	AM_GUI_Progress_t *p;
	AM_ErrorCode_t ret;
	
	assert(gui && prog);
	
	p = (AM_GUI_Progress_t*)malloc(sizeof(AM_GUI_Progress_t));
	if(!p)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_ProgressInit(gui, dir, p);
	if(ret!=AM_SUCCESS)
	{
		free(p);
		return ret;
	}
	
	*prog = p;
	return AM_SUCCESS;
}

/**\brief 初始化一个进度条控件
 * \param[in] gui 控件所属的GUI控制器
 * \param dir 进度条显示方向
 * \param[in] prog 进度条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ProgressInit(AM_GUI_t *gui, AM_GUI_Direction_t dir, AM_GUI_Progress_t *prog)
{
	AM_ErrorCode_t ret;
	AM_GUI_PosSize_t ps;
	AM_GUI_ThemeAttr_t attr;
	
	assert(gui && prog);
	
	ret = AM_GUI_WidgetInit(gui, AM_GUI_WIDGET(prog));
	if(ret!=AM_SUCCESS)
		return ret;
	
	AM_GUI_WIDGET(prog)->type = AM_GUI_WIDGET_PROGRESS;
	AM_GUI_WIDGET(prog)->cb   = AM_GUI_ProgressEventCb;
	prog->dir = dir;
	prog->min = 0;
	prog->max = 100;
	prog->value = 0;
	
	AM_GUI_POS_SIZE_INIT(&ps);
	
	if(dir==AM_GUI_DIR_VERTICAL)
	{
		AM_GUI_ThemeGetAttr(AM_GUI_WIDGET(prog), AM_GUI_THEME_ATTR_WIDTH, &attr);
		AM_GUI_POS_SIZE_WIDTH(&ps, attr.size);
		AM_GUI_POS_SIZE_CENTER(&ps, 0);
		AM_GUI_POS_SIZE_V_EXPAND(&ps);
	}
	else
	{
		AM_GUI_ThemeGetAttr(AM_GUI_WIDGET(prog), AM_GUI_THEME_ATTR_HEIGHT, &attr);
		AM_GUI_POS_SIZE_HEIGHT(&ps, attr.size);
		AM_GUI_POS_SIZE_MIDDLE(&ps, 0);
		AM_GUI_POS_SIZE_H_EXPAND(&ps);
	}
	
	AM_GUI_WidgetSetPosSize(AM_GUI_WIDGET(prog), &ps);
	
	return AM_SUCCESS;
}

/**\brief 释放一个进度条控件内部相关资源
 * \param[in] prog 进度条控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ProgressRelease(AM_GUI_Progress_t *prog)
{
	assert(prog);
	
	AM_GUI_WidgetRelease(AM_GUI_WIDGET(prog));
	
	return AM_SUCCESS;
}

/**\brief 设定进度条的取值范围
 * \param[in] prog 要释放的进度条控件
 * \param min 进度条最小值
 * \param max 进度条最大值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ProgressSetRange(AM_GUI_Progress_t *prog, int min, int max)
{
	assert(prog && (max>=min));
	
	if((min!=prog->min) || (max!=prog->max))
	{
		prog->min = min;
		prog->max = max;
		
		AM_GUI_WidgetUpdate(AM_GUI_WIDGET(prog), NULL);
	}
	
	return AM_SUCCESS;
}

/**\brief 设定进度条的当前值
 * \param[in] prog 要释放的进度条控件
 * \param value 进度条值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ProgressSetValue(AM_GUI_Progress_t *prog, int value)
{
	assert(prog);
	
	value = AM_MAX(prog->min, value);
	value = AM_MIN(prog->max, value);
	
	if(value!=prog->value)
	{
		prog->value = value;
		
		AM_GUI_WidgetUpdate(AM_GUI_WIDGET(prog), NULL);
	}
	
	return AM_SUCCESS;
}

