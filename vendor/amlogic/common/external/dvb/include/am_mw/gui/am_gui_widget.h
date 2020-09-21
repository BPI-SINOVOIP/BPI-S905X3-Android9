/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief GUI widget基本控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-22: create the document
 ***************************************************************************/

#ifndef _AM_GUI_WIDGET_H
#define _AM_GUI_WIDGET_H

#include <am_osd.h>
#include <am_inp.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_WIDGET_FL_VISIBLE   (1)  /**< 控件可见*/
#define AM_GUI_WIDGET_FL_FOCUSED   (2)  /**< 控件得到焦点*/
#define AM_GUI_WIDGET_FL_DISABLED  (4)  /**< 控件被禁用*/
#define AM_GUI_WIDGET_FL_UPDATE    (8)  /**< 控件需要重绘*/
#define AM_GUI_WIDGET_FL_FOCUSABLE (16) /**< 控件可以获得焦点*/
#define AM_GUI_WIDGET_FL_WINDOW    (32) /**< 控件为窗口控件*/

/**\brief 检查控件是否可见*/
#define AM_GUI_WidgetIsVisible(w)  ((w)->flags&AM_GUI_WIDGET_FL_VISIBLE)

/**\brief 检查控件是否获得焦点*/
#define AM_GUI_WidgetIsFocused(w)  ((w)->flags&AM_GUI_WIDGET_FL_FOCUSED)

/**\brief 检查控件是否处于不可用状态*/
#define AM_GUI_WidgetIsDisabled(w) ((w)->flags&AM_GUI_WIDGET_FL_DISABLED)

/**\brief 检查控件是否为窗口控件*/
#define AM_GUI_WidgetIsWindow(w)  ((w)->flags&AM_GUI_WIDGET_FL_WINDOW)

/**\brief 触发控件事件*/
#define AM_GUI_WidgetSendEvent(w,e)\
	({\
	AM_ErrorCode_t ret = AM_GUI_ERR_UNKNOWN_EVENT;\
	if(AM_GUI_WIDGET(w)->cb) {\
		ret = AM_GUI_WIDGET(w)->cb(AM_GUI_WIDGET(w), (AM_GUI_WidgetEvent_t*)(e));\
	}\
	if((ret==AM_GUI_ERR_UNKNOWN_EVENT) && AM_GUI_WIDGET(w)->user_cb) {\
		ret = AM_GUI_WIDGET(w)->user_cb(AM_GUI_WIDGET(w), (AM_GUI_WidgetEvent_t*)(e));\
	}\
	ret;\
	})

#define AM_GUI_WIDGET(w) ((AM_GUI_Widget_t*)(w))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief GUI管理器*/
typedef struct AM_GUI AM_GUI_t;

/**\brief 控件类型*/
typedef enum
{
	AM_GUI_WIDGET_UNKNOWN, /**< 未知控件*/
	AM_GUI_WIDGET_DESKTOP, /**< 桌面控件*/
	AM_GUI_WIDGET_BOX,     /**< 布局框控件*/
	AM_GUI_WIDGET_BOX_CELL,/**< 布局框单元格*/
	AM_GUI_WIDGET_TABLE,   /**< 表格控件*/
	AM_GUI_WIDGET_TABLE_ROW,  /**< 表格行控件*/
	AM_GUI_WIDGET_TABLE_CELL, /**< 表格单元格控件*/
	AM_GUI_WIDGET_WINDOW,  /**< 窗口控件*/
	AM_GUI_WIDGET_LABEL,   /**< 标签控件*/
	AM_GUI_WIDGET_BUTTON,  /**< 按钮控件*/
	AM_GUI_WIDGET_INPUT,   /**< 输入控件*/
	AM_GUI_WIDGET_PROGRESS,/**< 进度条控件*/
	AM_GUI_WIDGET_SCROLL,  /**< 滚动条控件*/
	AM_GUI_WIDGET_LIST_ROW,/**< 列表行控件*/
	AM_GUI_WIDGET_LIST,    /**< 列表控件*/
	AM_GUI_WIDGET_SELECT   /**< 选择框控件*/
} AM_GUI_WidgetType_t;

/**\brief 水平对齐类型*/
typedef enum
{
	AM_GUI_H_ALIGN_LEFT,   /**< 左对齐*/
	AM_GUI_H_ALIGN_RIGHT,  /**< 右对齐*/
	AM_GUI_H_ALIGN_CENTER  /**< 居中对齐*/
} AM_GUI_HAlignment_t;

/**\brief 垂直对齐类型*/
typedef enum
{
	AM_GUI_V_ALIGN_TOP,    /**< 上对齐*/
	AM_GUI_V_ALIGN_BOTTOM, /**< 下对齐*/
	AM_GUI_V_ALIGN_MIDDLE  /**< 居中对齐*/
} AM_GUI_VAlignment_t;

/**\brief 方向*/
typedef enum
{
	AM_GUI_DIR_HORIZONTAL, /**< 水平方向*/
	AM_GUI_DIR_VERTICAL    /**< 垂直方向*/
} AM_GUI_Direction_t;

/**\brief 控件边框尺寸*/
typedef struct
{
	int   left;            /**< 左边界宽度*/
	int   top;             /**< 上边界宽度*/
	int   right;           /**< 右边界宽度*/
	int   bottom;          /**< 下边界宽度*/
} AM_GUI_Border_t;

/**\brief 控件位置和长宽信息*/
typedef struct
{
	int   mask;            /**< 已经设定的值的标志*/
	int   left;            /**< 距父控件左边界距离*/
	int   right;           /**< 距父控件右边界距离*/
	int   top;             /**< 距父控件上边界距离*/
	int   bottom;          /**< 距父控件上边界距离*/
	int   width;           /**< 宽度*/
	int   height;          /**< 高度*/
} AM_GUI_PosSize_t;

#define AM_GUI_POS_SIZE_FL_LEFT    (1<<1)
#define AM_GUI_POS_SIZE_FL_RIGHT   (1<<2)
#define AM_GUI_POS_SIZE_FL_TOP     (1<<3)
#define AM_GUI_POS_SIZE_FL_BOTTOM  (1<<4)
#define AM_GUI_POS_SIZE_FL_WIDTH   (1<<5)
#define AM_GUI_POS_SIZE_FL_HEIGHT  (1<<6)
#define AM_GUI_POS_SIZE_FL_LEFT_PERCENT    (1<<7)
#define AM_GUI_POS_SIZE_FL_RIGHT_PERCENT   (1<<8)
#define AM_GUI_POS_SIZE_FL_TOP_PERCENT     (1<<9)
#define AM_GUI_POS_SIZE_FL_BOTTOM_PERCENT  (1<<10)
#define AM_GUI_POS_SIZE_FL_WIDTH_PERCENT   (1<<11)
#define AM_GUI_POS_SIZE_FL_HEIGHT_PERCENT  (1<<12)
#define AM_GUI_POS_SIZE_FL_CENTER          (1<<13)
#define AM_GUI_POS_SIZE_FL_MIDDLE          (1<<14)
#define AM_GUI_POS_SIZE_FL_V_EXPAND        (1<<15)
#define AM_GUI_POS_SIZE_FL_H_EXPAND        (1<<16)

/**\brief 初始化AM_GUI_PosSize_t数据结构*/
#define AM_GUI_POS_SIZE_INIT(ps)      ((ps)->mask=AM_GUI_POS_SIZE_FL_H_EXPAND|AM_GUI_POS_SIZE_FL_V_EXPAND)
/**\brief 设定距离父控件左边界的像素数*/
#define AM_GUI_POS_SIZE_LEFT(ps,v)    ((ps)->mask|=AM_GUI_POS_SIZE_FL_LEFT,(ps)->left=(v))
/**\brief 设定距离父控件右边界的像素数*/
#define AM_GUI_POS_SIZE_RIGHT(ps,v)   ((ps)->mask|=AM_GUI_POS_SIZE_FL_RIGHT,(ps)->right=(v))
/**\brief 设定距离父控件上边界的像素数*/
#define AM_GUI_POS_SIZE_TOP(ps,v)     ((ps)->mask|=AM_GUI_POS_SIZE_FL_TOP,(ps)->top=(v))
/**\brief 设定距离父控件下边界的像素数*/
#define AM_GUI_POS_SIZE_BOTTOM(ps,v)  ((ps)->mask|=AM_GUI_POS_SIZE_FL_BOTTOM,(ps)->bottom=(v))
/**\brief 设定控件宽度像素数*/
#define AM_GUI_POS_SIZE_WIDTH(ps,v)   ((ps)->mask|=AM_GUI_POS_SIZE_FL_WIDTH,(ps)->width=(v))
/**\brief 设定控件高度像素数*/
#define AM_GUI_POS_SIZE_HEIGHT(ps,v)  ((ps)->mask|=AM_GUI_POS_SIZE_FL_HEIGHT,(ps)->height=(v))
/**\brief 设定距离父控件左边界的百分比*/
#define AM_GUI_POS_SIZE_LEFT_PERCENT(ps,v)    ((ps)->mask|=AM_GUI_POS_SIZE_FL_LEFT|AM_GUI_POS_SIZE_FL_LEFT_PERCENT,(ps)->left=(v))
/**\brief 设定距离父控件右边界的百分比*/
#define AM_GUI_POS_SIZE_RIGHT_PERCENT(ps,v)   ((ps)->mask|=AM_GUI_POS_SIZE_FL_RIGHT|AM_GUI_POS_SIZE_FL_RIGHT_PERCENT,(ps)->right=(v))
/**\brief 设定距离父控件上边界的百分比*/
#define AM_GUI_POS_SIZE_TOP_PERCENT(ps,v)     ((ps)->mask|=AM_GUI_POS_SIZE_FL_TOP|AM_GUI_POS_SIZE_FL_TOP_PERCENT,(ps)->top=(v))
/**\brief 设定距离父控件下边界的百分比*/
#define AM_GUI_POS_SIZE_BOTTOM_PERCENT(ps,v)  ((ps)->mask|=AM_GUI_POS_SIZE_FL_BOTTOM|AM_GUI_POS_SIZE_FL_BOTTOM_PERCENT,(ps)->bottom=(v))
/**\brief 设定控件宽度百分比*/
#define AM_GUI_POS_SIZE_WIDTH_PERCENT(ps,v)   ((ps)->mask|=AM_GUI_POS_SIZE_FL_WIDTH|AM_GUI_POS_SIZE_FL_WIDTH_PERCENT,(ps)->width=(v))
/**\brief 设定控件高度百分比*/
#define AM_GUI_POS_SIZE_HEIGHT_PERCENT(ps,v)  ((ps)->mask|=AM_GUI_POS_SIZE_FL_HEIGHT|AM_GUI_POS_SIZE_FL_HEIGHT_PERCENT,(ps)->height=(v))
/**\brief 设定控件水平居中显示*/
#define AM_GUI_POS_SIZE_CENTER(ps,off)        ((ps)->mask|=AM_GUI_POS_SIZE_FL_CENTER|AM_GUI_POS_SIZE_FL_LEFT,(ps)->left=(off))
/**\brief 设定控件垂直居中显示*/
#define AM_GUI_POS_SIZE_MIDDLE(ps,off)        ((ps)->mask|=AM_GUI_POS_SIZE_FL_MIDDLE|AM_GUI_POS_SIZE_FL_TOP,(ps)->top=(off))
/**\brief 设定控件水平方向扩展*/
#define AM_GUI_POS_SIZE_H_EXPAND(ps)          ((ps)->mask|=AM_GUI_POS_SIZE_FL_H_EXPAND)
/**\brief 设定控件垂直方向扩展*/
#define AM_GUI_POS_SIZE_V_EXPAND(ps)          ((ps)->mask|=AM_GUI_POS_SIZE_FL_V_EXPAND)
/**\brief 设定控件水平方向紧缩*/
#define AM_GUI_POS_SIZE_H_SHRINK(ps)          ((ps)->mask&=~AM_GUI_POS_SIZE_FL_H_EXPAND)
/**\brief 设定控件垂直方向紧缩*/
#define AM_GUI_POS_SIZE_V_SHRINK(ps)          ((ps)->mask&=~AM_GUI_POS_SIZE_FL_V_EXPAND)

/**\brief Widget*/
typedef struct AM_GUI_Widget AM_GUI_Widget_t;

/**\brief 控件事件类型*/
typedef enum
{
	AM_GUI_WIDGET_EVT_NONE,     /**< 无事件*/
	AM_GUI_WIDGET_EVT_SHOW,     /**< 控件显示*/
	AM_GUI_WIDGET_EVT_HIDE,     /**< 控件隐藏*/
	AM_GUI_WIDGET_EVT_ENABLE,   /**< 控件使能*/
	AM_GUI_WIDGET_EVT_DISABLE,  /**< 控件禁用*/
	AM_GUI_WIDGET_EVT_DESTROY,  /**< 控件释放*/
	AM_GUI_WIDGET_EVT_ENTER,    /**< 光标进入控件*/
	AM_GUI_WIDGET_EVT_LEAVE,    /**< 光标离开控件*/
	AM_GUI_WIDGET_EVT_KEY_DOWN, /**< 按键*/
	AM_GUI_WIDGET_EVT_KEY_UP,   /**< 键抬起*/
	AM_GUI_WIDGET_EVT_PRESSED,  /**< 按钮被按下*/
	AM_GUI_WIDGET_EVT_DRAW_BEGIN,  /**< 控件重画,此事件在子控件绘制前被调用*/
	AM_GUI_WIDGET_EVT_DRAW_END,    /**< 控件重画,此事件在子控件绘制后被调用*/
	AM_GUI_WIDGET_EVT_LAY_OUT_W,   /**< 设定子控件宽度*/
	AM_GUI_WIDGET_EVT_LAY_OUT_H,   /**< 设定子控件高度*/
	AM_GUI_WIDGET_EVT_GET_W,       /**< 取得控件宽度信息*/
	AM_GUI_WIDGET_EVT_GET_H        /**< 取得控件高度信息*/
} AM_GUI_WidgetEventType_t;

/**\brief 输入事件*/
typedef struct
{
	AM_GUI_WidgetEventType_t  type;  /**< 事件类型*/
	struct input_event        input; /**< 输入事件*/
} AM_GUI_WidgetInputEvent_t;

/**\brief 控件重绘事件*/
typedef struct
{
	AM_GUI_WidgetEventType_t  type;  /**< 事件类型*/
	AM_OSD_Surface_t         *surface; /**< 绘图表面*/
	int                       org_x; /**< 原点X坐标*/
	int                       org_y; /**< 原点Y坐标*/
} AM_GUI_WidgetDrawEvent_t;

/**\brief 控件事件*/
typedef union
{
	AM_GUI_WidgetEventType_t   type;   /**< 事件类型*/
	AM_GUI_WidgetInputEvent_t  input;  /**< 输入事件*/
	AM_GUI_WidgetDrawEvent_t   draw;   /**< 重绘事件*/
} AM_GUI_WidgetEvent_t;

/**\brief 控件事件回调*/
typedef AM_ErrorCode_t (*AM_GUI_WidgetEventCallback_t) (AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt);

/**\brief Widget*/
struct AM_GUI_Widget
{
	AM_GUI_WidgetType_t type;        /**< 控件类型*/
	int                 theme_id;    /**< 控件主题ID*/
	AM_GUI_t           *gui;         /**< 控件所属的GUI管理器*/
	AM_GUI_Widget_t    *parent;      /**< 父控件*/
	AM_GUI_Widget_t    *prev;        /**< 同一级上层的控件*/
	AM_GUI_Widget_t    *next;        /**< 同一级下层的控件*/
	AM_GUI_Widget_t    *child_head;  /**< 顶层子控件*/
	AM_GUI_Widget_t    *child_tail;  /**< 底层子控件*/
	AM_OSD_Rect_t       rect;        /**< 控件在绘图表面上的位置*/
	int                 flags;       /**< 控件当前状态*/
	AM_GUI_PosSize_t    ps;          /**< 位置尺寸信息*/
	int                 min_w;       /**< 控件占用的最小宽度*/
	int                 max_w;       /**< 控件占用的最大宽度*/
	int                 prefer_w;    /**< 控件占用的适宜宽度*/
	int                 max_h;       /**< 控件占用最大高度*/
	int                 prefer_h;    /**< 控件占用的适宜高度*/
	AM_GUI_WidgetEventCallback_t cb;      /**< 控件事件回调函数*/
	AM_GUI_WidgetEventCallback_t user_cb; /**< 用户注册控件事件回调函数*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] widget 返回新的控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetCreate(AM_GUI_t *gui, AM_GUI_Widget_t **widget);

/**\brief 初始化一个控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetInit(AM_GUI_t *gui, AM_GUI_Widget_t *widget);

/**\brief 释放一个控件内部相关资源
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetRelease(AM_GUI_Widget_t *widget);

/**\brief 释放一个控件内部资源并释放内存
 * \param[in] widget 要释放的控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetDestroy(AM_GUI_Widget_t *widget);

/**\brief 控件缺省事件回调
 * \param[in] widget 控件
 * \param[in] evt 要处理的事件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetEventCb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt);

/**\brief 将一个控件加入父控件中
 * \param[in] parent 父控件
 * \param[in] child 子控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetAppendChild(AM_GUI_Widget_t *parent, AM_GUI_Widget_t *child);

/**\brief 显示一个控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetShow(AM_GUI_Widget_t *widget);

/**\brief 隐藏一个控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetHide(AM_GUI_Widget_t *widget);

/**\brief 使能控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetEnable(AM_GUI_Widget_t *widget);

/**\brief 禁用控件
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetDisable(AM_GUI_Widget_t *widget);

/**\brief 设定控件的位置和长宽信息
 * \param[in] widget 控件指针
 * \param[in] ps 位置和长宽信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetSetPosSize(AM_GUI_Widget_t *widget, const AM_GUI_PosSize_t *ps);

/**\brief 重新设定控件布局
 * \param[in] widget 控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetLayOut(AM_GUI_Widget_t *widget);

/**\brief 设定用户事件回调
 * \param[in] widget 控件指针
 * \param cb 回调函数指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetSetUserCallback(AM_GUI_Widget_t *widget, AM_GUI_WidgetEventCallback_t cb);

/**\brief 重新绘制控件
 * \param[in] widget 控件指针
 * \param[in] rect 需要重绘的区域
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_WidgetUpdate(AM_GUI_Widget_t *widget, AM_OSD_Rect_t *rect);

#ifdef __cplusplus
}
#endif

#endif

