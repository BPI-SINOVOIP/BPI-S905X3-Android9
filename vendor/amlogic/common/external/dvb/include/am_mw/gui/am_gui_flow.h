/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief GUI流数据
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_FLOW_H
#define _AM_GUI_FLOW_H

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

/**\brief 流元素类型*/
typedef enum
{
	AM_GUI_FLOW_ITEM_TEXT,       /**< 文本*/
	AM_GUI_FLOW_ITEM_IMAGE,      /**< 图片*/
	AM_GUI_FLOW_ITEM_NEWLINE,    /**< 换行*/
	AM_GUI_FLOW_ITEM_SPACE       /**< 插入空白*/
} AM_GUI_FlowItemType_t;

/**\brief 流元素*/
typedef struct AM_GUI_FlowItem AM_GUI_FlowItem_t;
struct AM_GUI_FlowItem
{
	AM_GUI_FlowItemType_t  type; /**< 元素类型*/
	AM_GUI_FlowItem_t *next;     /**< 链表中的下一个元素*/
};

/**\brief 文字元素*/
typedef struct
{
	AM_GUI_FlowItem_t  item;     /**< 元素基本数据*/
	char              *text;     /**< 文字内容*/
	int                len;      /**< 字符串长度*/
} AM_GUI_FlowText_t;

/**\brief 图像流元素*/
typedef struct
{
	AM_GUI_FlowItem_t   item;     /**< 元素基本信息*/
	AM_OSD_Surface_t   *image;    /**< 图片*/
	int                 width;    /**< 显示宽度*/
	int                 height;   /**< 显示长度*/
	AM_Bool_t           need_free;/**< 是否需要释放*/
} AM_GUI_FlowImage_t;

/**\brief 空白流元素*/
typedef struct
{
	AM_GUI_FlowItem_t   item;     /**< 元素基本信息*/
	int                 width;    /**< 空白宽度*/
} AM_GUI_FlowSpace_t;

/**\brief 流内的行信息*/
typedef struct AM_GUI_FlowRow AM_GUI_FlowRow_t;
struct AM_GUI_FlowRow
{
	AM_GUI_FlowRow_t  *next;     /**< 链表中的下一个行信息*/
	int                width;    /**< 行宽度*/
	int                height;   /**< 行高度*/
	AM_GUI_FlowItem_t *begin;    /**< 行起始元素*/
	int                begin_pos;/**< 行起始元素的字符位置*/
	AM_GUI_FlowItem_t *end;      /**< 行结束元素*/
	int                end_pos;  /**< 行结束元素的字符位置*/
};

/**\brief 流*/
typedef struct
{
	AM_GUI_t          *gui;      /**< 流对象所属的GUI控制器*/
	AM_Bool_t          reset;    /**< 是否重新设定*/
	AM_Bool_t          multiline;/**< 知否是多行流*/
	AM_GUI_FlowItem_t *item_head;/**< 元素链表头*/
	AM_GUI_FlowItem_t *item_tail;/**< 元素链表尾*/
	AM_GUI_FlowRow_t  *row_head; /**< 行信息链表头*/
	AM_GUI_FlowRow_t  *row_tail; /**< 行信息链表尾*/
	int                min_w;    /**< 整个流占用的最小宽度*/
	int                max_w;    /**< 整个流占用的最大宽度*/
	int                height;   /**< 流占用的高度*/
	int                width;    /**< 容器宽度*/
	AM_GUI_HAlignment_t halign;  /**< 水平对齐方式*/
	AM_GUI_VAlignment_t valign;  /**< 垂直对齐方式*/
} AM_GUI_Flow_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 初始化一个流对象
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowInit(AM_GUI_t *gui, AM_GUI_Flow_t *flow);

/**\brief 释放一个流对象
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowRelease(AM_GUI_Flow_t *flow);

/**\brief 向流对象中添加文本
 * \param[in] flow 流对象
 * \param[in] text 文本字符串
 * \param len 字符串长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowAppendText(AM_GUI_Flow_t *flow, const char *text, int len);

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
extern AM_ErrorCode_t AM_GUI_FlowAppendImage(AM_GUI_Flow_t *flow, AM_OSD_Surface_t *image, int width, int height, AM_Bool_t need_free);

/**\brief 向流对象中添加换行标志
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowAppendNewLine(AM_GUI_Flow_t *flow);

/**\brief 向流对象中添加空白区域
 * \param[in] flow 流对象
 * \param width 空白区域宽度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowAppendSpace(AM_GUI_Flow_t *flow, int width);

/**\brief 清除流对象中的数据
 * \param[in] flow 流对象
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowClear(AM_GUI_Flow_t *flow);

/**\brief 设定流是否使用多行模式
 * \param[in] flow 流对象
 * \param multiline 是否使用多行模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowSetMultiline(AM_GUI_Flow_t *flow, AM_Bool_t multiline);

/**\brief 设定流对齐方式
 * \param[in] flow 流对象
 * \param halign 水平对齐方式
 * \param valign 垂直对齐方式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowSetAlignment(AM_GUI_Flow_t *flow, AM_GUI_HAlignment_t halign, AM_GUI_VAlignment_t valign);

/**\brief 重新设定流对象的宽度，根据新宽度重新计算行信息
 * \param[in] flow 流对象
 * \param width 新设定流对象容器的宽度
 * \param font_id 流中文字的字体
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_FlowResize(AM_GUI_Flow_t *flow, int width, int font_id);

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
extern AM_ErrorCode_t AM_GUI_FlowDraw(AM_GUI_Flow_t *flow, AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, int font_id, uint32_t pixel);

#ifdef __cplusplus
}
#endif

#endif

