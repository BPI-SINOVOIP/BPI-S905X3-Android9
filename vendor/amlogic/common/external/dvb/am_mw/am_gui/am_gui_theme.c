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
 * \brief GUI主题
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-10: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_mem.h>
#include <am_font.h>
#include <string.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DEF_BORDER    2
#define DEF_FONT_SIZE 24
#define DEF_SEP       1
#define DEF_BAR_W     2
#define DEF_BAR_H     10

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 缺省主题*/
typedef struct
{
	int      def_font;           /**< 缺省字体*/
	uint32_t text_pixel;         /**< 文字颜色*/
	uint32_t focus_text_pixel;   /**< 焦点状态下的文字颜色*/
	uint32_t fg_pixel;           /**< 前景颜色*/
	uint32_t bg_pixel;           /**< 背景颜色*/
	uint32_t focus_fg_pixel;     /**< 焦点状态下的前景颜色*/
	uint32_t focus_bg_pixel;     /**< 焦点状态下的背景颜色*/
} AM_GUI_DefaultTheme_t;

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief 初始化缺省主题*/
static AM_ErrorCode_t default_init(AM_GUI_t *gui)
{
	AM_GUI_DefaultTheme_t *def;
	AM_FONT_Attr_t fattr;
	AM_OSD_Color_t c;
	AM_ErrorCode_t ret;
	
	def = (AM_GUI_DefaultTheme_t*)malloc(sizeof(AM_GUI_DefaultTheme_t));
	if(!def)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	fattr.name   = NULL;
	fattr.flags  = 0;
	fattr.width  = DEF_FONT_SIZE;
	fattr.height = 0;
	fattr.encoding = AM_FONT_ENC_UTF8;
	fattr.charset  = AM_FONT_CHARSET_UNICODE;
	
	ret = AM_FONT_Create(&fattr, &def->def_font);
	if(ret!=AM_SUCCESS)
	{
		free(def);
		return ret;
	}
	
	c.r = 255;
	c.g = 255;
	c.b = 255;
	c.a = 255;
	AM_OSD_MapColor(gui->surface->format, &c, &def->text_pixel);
	
	c.r = 255;
	c.g = 255;
	c.b = 0;
	c.a = 255;
	AM_OSD_MapColor(gui->surface->format, &c, &def->focus_text_pixel);
	
	c.r = 255;
	c.g = 255;
	c.b = 255;
	c.a = 255;
	AM_OSD_MapColor(gui->surface->format, &c, &def->fg_pixel);
	
	c.r = 0;
	c.g = 0;
	c.b = 0;
	c.a = 255;
	AM_OSD_MapColor(gui->surface->format, &c, &def->bg_pixel);
	
	c.r = 255;
	c.g = 255;
	c.b = 0;
	c.a = 255;
	AM_OSD_MapColor(gui->surface->format, &c, &def->focus_fg_pixel);
	
	c.r = 255;
	c.g = 255;
	c.b = 255;
	c.a = 255;
	AM_OSD_MapColor(gui->surface->format, &c, &def->focus_bg_pixel);
	
	gui->theme_data = def;
	return AM_SUCCESS;
}

/**\brief 释放缺省主题*/
static AM_ErrorCode_t default_quit(AM_GUI_t *gui)
{
	AM_GUI_DefaultTheme_t *def = (AM_GUI_DefaultTheme_t*)gui->theme_data;
	
	AM_FONT_Destroy(def->def_font);
	
	free(def);
	return AM_SUCCESS;
}

/**\brief 缺省主题*/
static AM_ErrorCode_t default_get_attr(AM_GUI_Widget_t *widget, AM_GUI_ThemeAttrType type, AM_GUI_ThemeAttr_t *attr)
{
	AM_GUI_DefaultTheme_t *def = (AM_GUI_DefaultTheme_t*)widget->gui->theme_data;
	AM_GUI_Window_t *win;
	
	switch(type)
	{
		case AM_GUI_THEME_ATTR_FONT:
			attr->font_id = def->def_font;
		break;
		case AM_GUI_THEME_ATTR_TEXT_COLOR:
			if(AM_GUI_WidgetIsFocused(widget))
				attr->pixel = def->focus_text_pixel;
			else
				attr->pixel = def->text_pixel;
		break;
		case AM_GUI_THEME_ATTR_H_SEP:
			attr->size = DEF_SEP;
		break;
		case AM_GUI_THEME_ATTR_V_SEP:
			attr->size = DEF_SEP;
		break;
		case AM_GUI_THEME_ATTR_BORDER:
			switch(widget->type)
			{
				case AM_GUI_WIDGET_BUTTON:
					attr->border.left   = DEF_BORDER;
					attr->border.top    = DEF_BORDER;
					attr->border.right  = DEF_BORDER;
					attr->border.bottom = DEF_BORDER;
				break;
				case AM_GUI_WIDGET_WINDOW:
					attr->border.left   = DEF_BORDER;
					attr->border.top    = DEF_BORDER;
					attr->border.right  = DEF_BORDER;
					attr->border.bottom = DEF_BORDER;
					win = (AM_GUI_Window_t*)widget;
					if(win->caption)
					{
						attr->border.top += AM_GUI_WIDGET(win->caption)->prefer_h;
						attr->border.top += DEF_BORDER;
					}
				break;
				default:
					attr->border.left   = 0;
					attr->border.top    = 0;
					attr->border.right  = 0;
					attr->border.bottom = 0;
				break;
			}
		break;
		case AM_GUI_THEME_ATTR_WIDTH:
			switch(widget->type)
			{
				case AM_GUI_WIDGET_PROGRESS:
				case AM_GUI_WIDGET_SCROLL:
					attr->size = AM_GUI_PROGRESS(widget)->dir==AM_GUI_DIR_VERTICAL?DEF_BAR_W:DEF_BAR_H;
				break;
				default:
				break;
			}
		break;
		case AM_GUI_THEME_ATTR_HEIGHT:
			switch(widget->type)
			{
				case AM_GUI_WIDGET_PROGRESS:
				case AM_GUI_WIDGET_SCROLL:
					attr->size = AM_GUI_PROGRESS(widget)->dir==AM_GUI_DIR_VERTICAL?DEF_BAR_H:DEF_BAR_W;
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
	
	return AM_SUCCESS;
}

/**\brief 缺省布局*/
static AM_ErrorCode_t default_lay_out(AM_GUI_Widget_t *widget)
{
	if(AM_GUI_WidgetIsWindow(widget))
	{
		AM_GUI_Window_t *win = AM_GUI_WINDOW(widget);
		
		if(win->caption)
		{
			AM_GUI_Widget_t *cw = AM_GUI_WIDGET(win->caption);
			
			cw->rect.x = DEF_BORDER;
			cw->rect.y = DEF_BORDER;
			cw->rect.w = widget->rect.w-DEF_BORDER*2;
			cw->rect.h = cw->prefer_h;
		}
	}
	
	return AM_SUCCESS;
}

/**\brief 缺省主题绘制*/
static AM_ErrorCode_t default_draw_begin(AM_GUI_Widget_t *widget, AM_OSD_Surface_t *s, int org_x, int org_y)
{
	AM_GUI_DefaultTheme_t *def = (AM_GUI_DefaultTheme_t*)widget->gui->theme_data;
	AM_GUI_Window_t *win;
	AM_GUI_Progress_t *prog;
	AM_GUI_Scroll_t *scroll;
	AM_OSD_Rect_t r;
	uint32_t pixel;
	
	switch(widget->type)
	{
		case AM_GUI_WIDGET_BUTTON:
			r.x = org_x;
			r.y = org_y;
			r.w = widget->rect.w;
			r.h = widget->rect.h;
			if(AM_GUI_WidgetIsFocused(widget))
				pixel = def->focus_fg_pixel;
			else
				pixel = def->fg_pixel;
			AM_OSD_DrawRect(s, &r, pixel);
		break;
		case AM_GUI_WIDGET_PROGRESS:
			prog = AM_GUI_PROGRESS(widget);
			pixel = def->fg_pixel;
			if(prog->dir==AM_GUI_DIR_VERTICAL)
			{
				int v = prog->value*widget->rect.h/(prog->max-prog->min);
				
				r.x = org_x;
				r.y = org_y+widget->rect.h-v;
				r.w = DEF_BAR_W;
				r.h = v;
			}
			else
			{
				int v = prog->value*widget->rect.w/(prog->max-prog->min);
				
				r.x = org_x;
				r.y = org_y;
				r.w = v;
				r.h = DEF_BAR_W;
			}
			AM_OSD_DrawFilledRect(s, &r, pixel);
		break;
		case AM_GUI_WIDGET_SCROLL:
			scroll = AM_GUI_SCROLL(widget);
			pixel = def->fg_pixel;
			if(scroll->dir==AM_GUI_DIR_VERTICAL)
			{
				int p = scroll->page*widget->rect.h/(scroll->max-scroll->min);
				int v;
				
				if(scroll->max==scroll->min)
					v = 0;
				else
					v = (scroll->value-scroll->min)*widget->rect.h/(scroll->max-scroll->min);
				
				r.x = org_x;
				r.y = org_y+v;
				r.w = DEF_BAR_W;
				r.h = p;
			}
			else
			{
				int p = scroll->page*widget->rect.w/(scroll->max-scroll->min);
				int v;
				
				if(scroll->max==scroll->min)
					v = 0;
				else
					v = (scroll->value-scroll->min)*widget->rect.w/(scroll->max-scroll->min);
				
				r.x = org_x+v;
				r.y = org_y;
				r.w = p;
				r.h = DEF_BAR_W;
			}
			AM_OSD_DrawFilledRect(s, &r, pixel);
		break;
		case AM_GUI_WIDGET_WINDOW:
			r.x = org_x;
			r.y = org_y;
			r.w = widget->rect.w;
			r.h = widget->rect.h;
			pixel = def->bg_pixel;
			AM_OSD_DrawFilledRect(s, &r, pixel);
			pixel = def->fg_pixel;
			AM_OSD_DrawRect(s, &r, pixel);
			win = AM_GUI_WINDOW(widget);
			if(win->caption)
			{
				AM_OSD_DrawHLine(s, org_x,
						org_y+DEF_BORDER+AM_GUI_WIDGET(win->caption)->prefer_h,
						widget->rect.w, pixel);
			}
		break;
		default:
		break;
	}
	
	return AM_SUCCESS;
}

/**\brief 缺省主题绘制*/
static AM_ErrorCode_t default_draw_end(AM_GUI_Widget_t *widget, AM_OSD_Surface_t *s, int org_x, int org_y)
{
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 初始化GUI主题相关数据
 * \param[in] gui GUI控制器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ThemeInit(AM_GUI_t *gui)
{
	return default_init(gui);
}

/**\brief 释放GUI主题相关数据
 * \param[in] gui GUI控制器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ThemeQuit(AM_GUI_t *gui)
{
	return default_quit(gui);
}

/**\brief 取得控件对应的主题属性值
 * \param[in] widget 控件指针
 * \param type 属性类型
 * \param[out] attr 返回属性值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ThemeGetAttr(AM_GUI_Widget_t *widget, AM_GUI_ThemeAttrType type, AM_GUI_ThemeAttr_t *attr)
{
	return default_get_attr(widget, type, attr);
}

/**\brief 根据主题设定控件内部布局
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ThemeLayOut(AM_GUI_Widget_t *widget)
{
	return default_lay_out(widget);
}

/**\brief 根据主题绘制控件,此函数在绘制子控件前调用
 * \param[in] widget 控件指针
 * \param[in] s 绘图表面
 * \param org_x 控件左上角X坐标
 * \param org_y 控件左上角Y坐标
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ThemeDrawBegin(AM_GUI_Widget_t *widget, AM_OSD_Surface_t *s, int org_x, int org_y)
{
	return default_draw_begin(widget, s, org_x, org_y);
}

/**\brief 根据主题绘制控件，此函数在绘制子控件后调用
 * \param[in] widget 控件指针
 * \param[in] s 绘图表面
 * \param org_x 控件左上角X坐标
 * \param org_y 控件左上角Y坐标
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_ThemeDrawEnd(AM_GUI_Widget_t *widget, AM_OSD_Surface_t *s, int org_x, int org_y)
{
	return default_draw_end(widget, s, org_x, org_y);
}

