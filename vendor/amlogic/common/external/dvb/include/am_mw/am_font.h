/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 字体管理模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-20: create the document
 ***************************************************************************/

#ifndef _AM_FONT_H
#define _AM_FONT_H

#include <am_types.h>
#include <am_osd.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief 字体管理模块错误代码*/
enum AM_FONT_ErrorCode
{
	AM_FONT_ERROR_BASE=AM_ERROR_BASE(AM_MOD_FONT),
	AM_FONT_ERR_CANNOT_OPEN_FILE,   /**< 不能打开文件*/
	AM_FONT_ERR_NO_MEM,             /**< 内存不足*/
	AM_FONT_ERR_NOT_SUPPORTED,      /**< 不支持*/
	AM_FONT_ERR_ILLEGAL_CODE,       /**< 无效的字符*/
	AM_FONT_ERR_END
};


/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief 字体中全部字符宽度一致*/
#define AM_FONT_FL_FIXED_WIDTH    (1)

/**\brief 字体中全部字符高度一致*/
#define AM_FONT_FL_FIXED_HEIGHT   (2)

/**\brief 字体为定点字体，不能缩放*/
#define AM_FONT_FL_FIXED_SIZE     (4)

/**\brief 字体为斜体*/
#define AM_FONT_FL_ITALIC         (8)

/**\brief 字体为黑体*/
#define AM_FONT_FL_BOLD           (16)

/**\brief 字体带下划线*/
#define AM_FONT_FL_BASELINE       (32)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 字体文件使用的编码*/
typedef enum
{
	AM_FONT_ENC_GBK,              /**< GBK编码*/
	AM_FONT_ENC_UTF8,             /**< UTF8编码*/
	AM_FONT_ENC_UTF16             /**< UTF16编码*/
} AM_FONT_Encoding_t;

/**\brief 字体文件包含的字符集*/
typedef enum
{
	AM_FONT_CHARSET_ASCII=1,      /**< ASCII字符集*/
	AM_FONT_CHARSET_GBK=2,        /**< GBK汉字字符集*/
	AM_FONT_CHARSET_UNICODE=AM_FONT_CHARSET_ASCII|AM_FONT_CHARSET_GBK /**< UNICODE字符集*/
} AM_FONT_Charset_t;

/**\brief 字体属性*/
typedef struct
{
	char              *name;      /**< 字体名称*/
	int                flags;     /**< 字体属性标志，AM_FONT_FL_XXXX*/
	int                width;     /**< 字体宽度*/
	int                height;    /**< 字体高度*/
	AM_FONT_Encoding_t encoding;  /**< 字体文件使用的编码*/
	AM_FONT_Charset_t  charset;   /**< 字体文件包含的字符集*/
} AM_FONT_Attr_t;

/**\brief 字符串的长宽信息*/
typedef struct
{
	int                width;     /**< 字符串的宽度*/
	int                height;    /**< 字符串的高度*/
	int                ascent;    /**< 基准线上半部分高度*/
	int                descent;   /**< 基准线下半部分高度*/
	int                stop_code; /**< 当遇到此字符时，退出AM_FONT_Size*/
	int                max_width; /**< 字符串占用的最大宽度，如果达到这个宽度，AM_FONT_Size直接退出,<=0表示没有限制*/
	int                bytes;     /**< 达到最大宽度时已经分析的字节数*/
} AM_FONT_TextInfo_t;

/**\brief 字符串绘制位置信息*/
typedef struct
{
	AM_OSD_Rect_t      rect;      /**< 绘制字符串的矩形*/
	int                base;      /**< 字符串基线位置(距离底边距离)*/
} AM_FONT_TextPos_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 初始化字体管理模块
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_Init(void);

/**\brief 加载一个字库文件
 * \param[in] path 字体文件名
 * \param[in] drv_name 字体驱动名
 * \param[in] attr 字体文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_LoadFile(const char *path, const char *drv_name, const AM_FONT_Attr_t *attr);

/**\brief 设定一个字体为缺省字体
 * \param font_id 缺省字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_SetDefault(int font_id);

/**\brief 取得当前的缺省字体
 * \param[out] font_id 缺省字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_GetDefault(int *font_id);

/**\brief 创建一个字体
 * \param[in] attr 要创建的字体属性
 * \param[out] font_id 返回创建的字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_Create(const AM_FONT_Attr_t *attr, int *font_id);

/**\brief 取得字体属性
 * \param font_id 字体句柄
 * \param[in] attr 要创建的字体属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_GetAttr(int font_id, AM_FONT_Attr_t *attr);

/**\brief 计算绘制指定字体字符串的高度和宽度
 * \param font_id 字体句柄
 * \param[in] text 待计算的字符串指针
 * \param len 字符串长度
 * \param[out] info 返回字符串信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_Size(int font_id, const char *text, int len, AM_FONT_TextInfo_t *info);

/**\brief 用指定字体绘制字符串
 * \param font_id 字体句柄
 * \param[in] surface 绘图表面
 * \param[in] text 需要绘制的字符串
 * \param len 字符串长度
 * \param[in] pos 绘制字符串的位置
 * \param pixel 绘制文字使用的像素值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_Draw(int font_id, AM_OSD_Surface_t *surface, const char *text, int len, const AM_FONT_TextPos_t *pos, uint32_t pixel);

/**\brief 取指定字体的行间距
 * \param font_id 字体句柄
 * \param[out] lineskip 返回行间距值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_GetLineSkip(int font_id, int *lineskip);

/**\brief 销毁一个字体
 * \param font_id 要销毁的字体句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_Destroy(int font_id);

/**\brief 终止字体管理模块
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_font.h)
 */
extern AM_ErrorCode_t AM_FONT_Quit(void);

#ifdef __cplusplus
}
#endif

#endif

