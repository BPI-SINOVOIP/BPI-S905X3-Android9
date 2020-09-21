/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Teletext模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-18: create the document
 ***************************************************************************/


#ifndef _AM_TT_H
#define _AM_TT_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/



/**\brief TeleText模块错误代码*/
enum AM_TT_ErrorCode
{
	AM_TT_ERROR_BASE=AM_ERROR_BASE(AM_MOD_TT),
	AM_TT_ERR_INVALID_PARAM,       /**< 参数无效*/
	AM_TT_ERR_INVALID_HANDLE,      /**< 句柄无效*/
	AM_TT_ERR_NOT_SUPPORTED,       /**< 不支持的操作*/
	AM_TT_ERR_CREATE_DECODE,       /**< 打开TELETEXT解码器失败*/
	AM_TT_ERR_OPEN_PES,            /**< 打开PES通道失败*/
	AM_TT_ERR_SET_BUFFER,          /**< 失置PES 缓冲区失败*/
	AM_TT_ERR_NO_MEM,              /**< 空闲内存不足*/
	AM_TT_ERR_CANNOT_CREATE_THREAD,/**< 无法创建线程*/
	AM_TT_ERR_NOT_RUN,             /**< 没有运行teletext*/
	AM_TT_ERR_NOT_START,           /**< 没有开始*/
	AM_TT_ERR_PAGE_NOT_EXSIT,      /**< 请求页不存在*/
	AM_TT_INIT_DISPLAY_FAILED,     /**< 初始化显示屏幕失败*/
	AM_TT_INIT_FONT_FAILED,        /**< 初始化字体失败*/
	AM_TT_ERR_END
};

typedef enum AM_TT_EventCode
{
	AM_TT_EVT_TIMEOUT=0,
	AM_TT_EVT_PAGE_UPDATE,
	AM_TT_EVT_PAGE_ERROR,
} AM_TT_EventCode_t;

typedef enum
{
    AM_TT_COLOR_TEXT_FG,
    AM_TT_COLOR_TEXT_BG,
    AM_TT_COLOR_SPACE,
    AM_TT_COLOR_MOSAIC
} AM_TT_ColorType_t;


/**\brief 填充矩形
 * left,top,width,height为矩形属性
 * color 为填充颜色
 */
typedef void (*AM_TT_FillRectCb_t)(int left, int top, unsigned int width, unsigned int height, unsigned int pixel);

/**\brief 事件回调
 * evt为事件编码
 */
typedef void (*AM_TT_EventCb_t)(AM_TT_EventCode_t evt);

/**\brief 绘制字符
 * x,y,width,height字体所在区域
 * text,len 字符串及长度
 * color 为填充颜色
 * w_scale,h_scale 绘制比例
 */
typedef void (*AM_TT_DrawTextCb_t)(int x, int y, unsigned int width, unsigned int height, unsigned short *text, int len, unsigned int pixel, unsigned int w_scale, unsigned int h_scale);

/**\brief 颜色转换
 * index颜色索引值(0:黑色;1:蓝色;2:绿色;3:青色;4:红色;5:紫色;6:黄色;15:白色;)
 */
typedef unsigned int (*AM_TT_ConvertColorCb_t)(unsigned int index, unsigned int type);
/**\brief 获取字体高度
 * (无参数)
 */
typedef unsigned int (*AM_TT_GetFontHeightCb_t)(void);
/**\brief 获取字体宽度
 * (无参数)
 */
typedef unsigned int (*AM_TT_GetFontMaxWidthCb_t)(void);

/**\brief 开始绘图
 * (无参数)
 */
typedef void (*AM_TT_DrawBeginCb_t)(void);

/**\brief 更新屏幕
 * (无参数)
 */
typedef void (*AM_TT_DisplayUpdateCb_t)(void);

/**\brief 清空屏幕
 * (无参数)
 */
typedef void (*AM_TT_ClearDisplayCb_t)(void);


/**\brief 绘图回调函数集*/
typedef struct {
	AM_TT_FillRectCb_t        fill_rect;     /**< 填充矩形*/
	AM_TT_DrawTextCb_t        draw_text;     /**< 绘制字符*/
	AM_TT_ConvertColorCb_t    conv_color;    /**< 调色板颜色转换*/
	AM_TT_GetFontHeightCb_t   font_height;   /**< 取字体高度*/
	AM_TT_GetFontMaxWidthCb_t font_width;    /**< 取字体最大宽度*/
	AM_TT_DrawBeginCb_t       draw_begin;    /**< 开始绘图*/
	AM_TT_DisplayUpdateCb_t   disp_update;   /**< 更新显示*/
	AM_TT_ClearDisplayCb_t    clear_disp;    /**< 清空屏幕*/
}AM_TT_DrawOps_t;

/**\brief 初始化teletext 模块
 * \param  buffer_size  分配缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_Init(int buffer_size);

/**\brief 卸载teletext 模块 请先调用am_mw_teletext_stop
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */

AM_ErrorCode_t AM_TT_Quit();

/**\brief 开始处理teletext(包括数据接收与解析)
 * \param dmx_id  demux id号
 * \param pid  packet id
 * \param mag magazine id
 * \param page page number
 * \param sub_page sub page number
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_Start(int dmx_id, int pid, int mag, int page, int sub_page);

/**\brief 停止处理teletext
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_Stop();

/**\brief 获取当前页编码
 * \return
 *   - 当前页码与字页码(低十六位为页编码,高十六位为子页编码)
 */
unsigned int AM_TT_GetCurrentPageCode();

/**\brief 改变当前页编码
 * \param wPageCode  页编码
 * \param wSubPageCode  字页编码
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_SetCurrentPageCode(unsigned short wPageCode, unsigned short wSubPageCode);

/**\brief 子页下翻
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_NextSubPage();

/**\brief 子页上翻
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_PreviousSubPage();

/**\brief 页下翻
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_NextPage();

/**\brief 页上翻
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_PreviousPage();

/**\brief 进入链接页
 * \param color  颜色索引(取值0~5)
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_PerformColorLink(unsigned char color);

/**\brief 注册图形相关接口函数
 * \param[in] ops 绘图操作函数结构指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_RegisterDrawOps (AM_TT_DrawOps_t *ops);

/**\brief 注册事件回调函数
 * \param  cb 函数指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_tt.h)
 */
AM_ErrorCode_t AM_TT_RegisterEventCb(AM_TT_EventCb_t cb);

#ifdef __cplusplus
}
#endif


#endif
