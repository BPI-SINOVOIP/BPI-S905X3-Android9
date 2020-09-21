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

#ifndef _AM_GUI_THEME_H
#define _AM_GUI_THEME_H

#include "am_gui_widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 主题属性类型*/
typedef enum
{
	AM_GUI_THEME_ATTR_BG_COLOR,   /**< 背景颜色*/
	AM_GUI_THEME_ATTR_FG_COLOR,   /**< 前景颜色*/
	AM_GUI_THEME_ATTR_TEXT_COLOR, /**< 文字颜色*/
	AM_GUI_THEME_ATTR_FONT,       /**< 字体*/
	AM_GUI_THEME_ATTR_BORDER,     /**< 边框尺寸*/
	AM_GUI_THEME_ATTR_H_SEP,      /**< 水平间隔*/
	AM_GUI_THEME_ATTR_V_SEP,      /**< 垂直间隔*/
	AM_GUI_THEME_ATTR_WIDTH,      /**< 宽度*/
	AM_GUI_THEME_ATTR_HEIGHT      /**< 高度*/
} AM_GUI_ThemeAttrType;

/**\brief 主题属性*/
typedef union
{
	uint32_t        pixel;        /**< 像素值属性*/
	int             font_id;      /**< 字体属性*/
	AM_OSD_Rect_t   rect;         /**< 位置矩形*/
	AM_GUI_Border_t border;       /**< 边框尺寸*/
	int             size;         /**< 尺寸(像素数)*/
} AM_GUI_ThemeAttr_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 初始化GUI主题相关数据
 * \param[in] gui GUI控制器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ThemeInit(AM_GUI_t *gui);

/**\brief 释放GUI主题相关数据
 * \param[in] gui GUI控制器
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ThemeQuit(AM_GUI_t *gui);

/**\brief 取得控件对应的主题属性值
 * \param[in] widget 控件指针
 * \param type 属性类型
 * \param[out] attr 返回属性值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ThemeGetAttr(AM_GUI_Widget_t *widget, AM_GUI_ThemeAttrType type, AM_GUI_ThemeAttr_t *attr);

/**\brief 根据主题设定控件内部布局
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ThemeLayOut(AM_GUI_Widget_t *widget);

/**\brief 根据主题绘制控件,此函数在绘制子控件前调用
 * \param[in] widget 控件指针
 * \param[in] s 绘图表面
 * \param org_x 控件左上角X坐标
 * \param org_y 控件左上角Y坐标
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ThemeDrawBegin(AM_GUI_Widget_t *widget, AM_OSD_Surface_t *s, int org_x, int org_y);

/**\brief 根据主题绘制控件，此函数在绘制子控件后调用
 * \param[in] widget 控件指针
 * \param[in] s 绘图表面
 * \param org_x 控件左上角X坐标
 * \param org_y 控件左上角Y坐标
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ThemeDrawEnd(AM_GUI_Widget_t *widget, AM_OSD_Surface_t *s, int org_x, int org_y);

#ifdef __cplusplus
}
#endif

#endif

