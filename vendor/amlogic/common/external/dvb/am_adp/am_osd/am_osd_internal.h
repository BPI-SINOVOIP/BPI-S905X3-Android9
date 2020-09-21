/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief OSD模块内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-18: create the document
 ***************************************************************************/

#ifndef _AM_OSD_INTERNAL_H
#define _AM_OSD_INTERNAL_H

#include <am_osd.h>
#include <endian.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#if __BYTE_ORDER == __BIG_ENDIAN
# define AM_PUT_3BPP_PIXEL(ptr,pix)\
	AM_MACRO_BEGIN\
		(ptr)[0] = (pix)>>16;\
		(ptr)[1] = (pix)>>8;\
		(ptr)[2] = (pix);\
	AM_MACRO_END
# define AM_GET_3BPP_PIXEL(ptr)\
	(((ptr)[0]<<16)|((ptr)[1]<<8)|(ptr)[2])
#else
# define AM_PUT_3BPP_PIXEL(ptr,pix)\
	AM_MACRO_BEGIN\
		(ptr)[2] = (pix)>>16;\
		(ptr)[1] = (pix)>>8;\
		(ptr)[0] = (pix);\
	AM_MACRO_END
# define AM_GET_3BPP_PIXEL(ptr)\
	(((ptr)[2]<<16)|((ptr)[1]<<8)|(ptr)[0])
#endif

#define AM_RGB_PIXEL_TO_COLOR_COMPONENT(format,pix,c)\
	((((pix)&(format)->c##_mask)>>(format)->c##_offset)<<(format)->c##_shift)

#define AM_RGB_PIXEL_TO_COLOR(format,pix,col)\
	AM_MACRO_BEGIN\
		(col)->r = AM_RGB_PIXEL_TO_COLOR_COMPONENT(format,pix,r);\
		(col)->g = AM_RGB_PIXEL_TO_COLOR_COMPONENT(format,pix,g);\
		(col)->b = AM_RGB_PIXEL_TO_COLOR_COMPONENT(format,pix,b);\
		(col)->a = (format)->a_mask?AM_RGB_PIXEL_TO_COLOR_COMPONENT(format,pix,a):0xFF;\
	AM_MACRO_END

#define AM_COLOR_COMPONENT_TO_RGB_PIXEL(format,col,com)\
	((((col)->com>>(format)->com##_shift)<<(format)->com##_offset)&(format)->com##_mask)

#define AM_COLOR_TO_RGB_PIXEL(format,col)\
	(AM_COLOR_COMPONENT_TO_RGB_PIXEL(format,col,r)|\
	AM_COLOR_COMPONENT_TO_RGB_PIXEL(format,col,g)|\
	AM_COLOR_COMPONENT_TO_RGB_PIXEL(format,col,b)|\
	AM_COLOR_COMPONENT_TO_RGB_PIXEL(format,col,a))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 绘图表面操作*/
typedef struct
{
	AM_ErrorCode_t (*init) (void);
	AM_ErrorCode_t (*set_clip) (AM_OSD_Surface_t *s, AM_OSD_Rect_t *clip);
	AM_ErrorCode_t (*create_surface) (AM_OSD_Surface_t *s);
	AM_ErrorCode_t (*destroy_surface) (AM_OSD_Surface_t *s);
	AM_ErrorCode_t (*pixel) (AM_OSD_Surface_t *s, int x, int y, uint32_t pix);
	AM_ErrorCode_t (*hline) (AM_OSD_Surface_t *s, int x, int y, int w, uint32_t pix);
	AM_ErrorCode_t (*vline) (AM_OSD_Surface_t *s, int x, int y, int h, uint32_t pix);
	AM_ErrorCode_t (*filled_rect) (AM_OSD_Surface_t *s, AM_OSD_Rect_t *rect, uint32_t pix);
	AM_ErrorCode_t (*blit) (AM_OSD_Surface_t *src, AM_OSD_Rect_t *sr, AM_OSD_Surface_t *dst, AM_OSD_Rect_t *dr, const AM_OSD_BlitPara_t *para);
	AM_ErrorCode_t (*deinit) (void);
} AM_OSD_SurfaceOp_t;

/**\brief OSD设备*/
typedef struct AM_OSD_Device AM_OSD_Device_t;

/**\brief OSD设备驱动*/
typedef struct
{
	AM_ErrorCode_t (*open) (AM_OSD_Device_t *dev, const AM_OSD_OpenPara_t *para, AM_OSD_Surface_t **s, AM_OSD_Surface_t **d);
	AM_ErrorCode_t (*enable) (AM_OSD_Device_t *dev, AM_Bool_t enable);
	AM_ErrorCode_t (*update) (AM_OSD_Device_t *dev, AM_OSD_Rect_t *rect);
	AM_ErrorCode_t (*set_palette) (AM_OSD_Device_t *dev, AM_OSD_Color_t *col, int start, int count);
	AM_ErrorCode_t (*set_alpha) (AM_OSD_Device_t *dev, uint8_t alpha);
	AM_ErrorCode_t (*close) (AM_OSD_Device_t *dev);
} AM_OSD_Driver_t;

/**\brief OSD设备*/
struct AM_OSD_Device
{
	const AM_OSD_Driver_t *drv;             /**< 设备驱动*/
	int               vout_dev_no;          /**< OSD对应的视频输出设备ID*/
	int               vout_w;               /**< 视频输出宽度*/
	int               vout_h;               /**< 视频输出高度*/
	int               dev_no;               /**< 设备ID*/
	AM_OSD_OpenPara_t para;                 /**< 设备开启参数*/
	void             *drv_data;             /**< 驱动私有数据*/
	AM_Bool_t         enable_double_buffer; /**< 是否支持双缓冲*/
	AM_Bool_t         enable;               /**< 是否显示OSD*/
	AM_Bool_t         openned;              /**< 设备已经打开*/
	AM_OSD_Surface_t *surface;              /**< OSD对应绘图表面*/
	AM_OSD_Surface_t *display;              /**< OSD的显示绘图表面*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 检查一个点是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param x 检测点的X座标
 * \param y 检测点的Y座标
 * \return
 *   - AM_TRUE 点在剪切矩形内
 *   - AM_FALSE 点在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipPoint(AM_OSD_Rect_t *c, int x, int y);

/**\brief 检查一条水平线是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param x 水平线左顶点X座标
 * \param y 水平线Y座标
 * \param w 水平线宽度
 * \param[out] ox 水平线在剪切矩形内的左顶点X坐标
 * \param[out] ow 水平线在剪切矩形内的宽度
 * \return
 *   - AM_TRUE 水平线在剪切矩形内
 *   - AM_FALSE 水平线在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipHLine(AM_OSD_Rect_t *c, int x, int y, int w, int *ox, int *ow);

/**\brief 检查一条垂直线是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param x 垂直线上顶点X座标
 * \param y 垂直线Y座标
 * \param h 垂直线高度
 * \param[out] oy 垂直线在剪切矩形内的上顶点Y坐标
 * \param[out] oh 垂直线在剪切矩形内的高度
 * \return
 *   - AM_TRUE 垂直线在剪切矩形内
 *   - AM_FALSE 垂直线在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipVLine(AM_OSD_Rect_t *c, int x, int y, int h, int *oy, int *oh);

/**\brief 检查一个矩形是否在剪切矩形内
 * \param[in] c 剪切矩形
 * \param r 检测矩形
 * \param[out] or 矩形在剪切区内的部分
 * \return
 *   - AM_TRUE 矩形在剪切矩形内
 *   - AM_FALSE 矩形在剪切矩形外
 */
AM_Bool_t AM_OSD_ClipRect(AM_OSD_Rect_t *c, AM_OSD_Rect_t *r, AM_OSD_Rect_t *or);

/**\brief 计算两个矩形的合集矩形
 * \param[in] r1 矩形1
 * \param[in] r2 矩形2
 * \param[out] or 合集矩形
 */
void AM_OSD_CollectRect(AM_OSD_Rect_t *r1, AM_OSD_Rect_t *r2, AM_OSD_Rect_t *or);

#ifdef __cplusplus
}
#endif

#endif

