/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 字体管理模块内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-20: create the document
 ***************************************************************************/

#ifndef _AM_FONT_INTERNAL_H
#define _AM_FONT_INTERNAL_H

#include <am_font.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define FONT_FILE_REF_CNT          (2)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 字形数据位图类型*/
typedef enum
{
	AM_FONT_BITMAP_1BPP, /**< 每个点占一位*/
	AM_FONT_BITMAP_8BPP  /**< 每个点占一字节*/
} AM_FONT_BitmapType_t;

/**\brief 字形数据*/
typedef struct
{
	AM_FONT_BitmapType_t type; /**< 位图类型*/
	int   advance; /**< 字符占用宽度*/
	int   left;    /**< 字形距离起始坐标的距离*/
	int   width;   /**< 字符宽度*/
	int   height;  /**< 字符高度*/
	int   ascent;  /**< 基准线上部分高度*/
	int   descent; /**< 基准线下部分高度*/
	int   pitch;   /**< 每行字节数*/
	char *buffer;  /**< 字形数据缓冲区*/
} AM_FONT_Glyph_t;

typedef struct AM_FONT_File AM_FONT_File_t;
typedef struct AM_FONT AM_FONT_t;

/**\brief 字体驱动*/
typedef struct
{
	char *name;
	AM_ErrorCode_t (*init) (void);
	AM_ErrorCode_t (*open_file) (const char *path, AM_FONT_File_t *file);
	AM_ErrorCode_t (*match_size) (AM_FONT_File_t *file, int *width, int *height);
	AM_ErrorCode_t (*open_font) (AM_FONT_t *font, AM_FONT_File_t *file);
	AM_ErrorCode_t (*char_size) (AM_FONT_t *font, AM_FONT_File_t *file, const char *ch, int size, AM_FONT_Glyph_t *glyph);
	AM_ErrorCode_t (*char_glyph) (AM_FONT_t *font, AM_FONT_File_t *file, const char *ch, int size, AM_FONT_Glyph_t *glyph);
	AM_ErrorCode_t (*close_font) (AM_FONT_t *font, AM_FONT_File_t *file);
	AM_ErrorCode_t (*close_file) (AM_FONT_File_t *file);
	AM_ErrorCode_t (*quit) (void);
} AM_FONT_Driver_t;

/**\brief 字库文件*/
struct AM_FONT_File
{
	const AM_FONT_Driver_t *drv;/**< 字体驱动*/
	void             *drv_data; /**< 驱动私有数据*/
	AM_FONT_Attr_t    attr;     /**< 字库文件属性*/
	AM_FONT_File_t   *next;     /**< 指向链表中的下一个字体文件*/
};

/**\brief 字体文件引用*/
typedef struct
{
	AM_FONT_File_t   *file;     /**< 指向对应的字体文件*/
	void             *file_data;/**< 字体文件私有数据*/
} AM_FONT_FileRef_t;

/**\brief 字体*/
struct AM_FONT
{
	AM_FONT_File_t   *refs[FONT_FILE_REF_CNT]; /**< 该字体对应的字体文件引用*/
	AM_FONT_t        *prev;     /**< 指向链表中前一个字体*/
	AM_FONT_t        *next;     /**< 指向链表中下一个字体*/
	AM_FONT_Attr_t    attr;     /**< 该字体的属性*/
	void             *drv_data; /**< 驱动私有数据*/
	int               lineskip; /**< 行间距*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

