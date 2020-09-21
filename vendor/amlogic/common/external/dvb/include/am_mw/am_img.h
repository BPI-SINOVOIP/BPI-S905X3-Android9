/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 图片加载模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-10-18: create the document
 ***************************************************************************/

#ifndef _AM_IMG_H
#define _AM_IMG_H

#include <am_types.h>
#include <am_osd.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief 图片加载模块错误代码*/
enum AM_IMG_ErrorCode
{
	AM_IMG_ERROR_BASE=AM_ERROR_BASE(AM_MOD_IMG),
	AM_IMG_ERR_FORMAT_MISMATCH,    /**< 文件格式不匹配*/
	AM_IMG_ERR_CANNOT_OPEN_FILE,   /**< 不能打开文件*/
	AM_IMG_ERR_NO_MEM,             /**< 内存不足*/
	AM_IMG_ERR_NOT_SUPPORTED,      /**< 不支持此操作*/
	AM_IMG_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 图像解码参数*/
typedef struct
{
	AM_Bool_t    enable_hw;       /**< 用硬件进行解码*/
	int          hw_dev_no;       /**< 硬件解码设备ID*/
	int          surface_flags;   /**< 创建image surface的标志，可以为0，AM_OSD_SURFACE_FL_HW或者AM_OSD_SURFACE_FL_OPENGL*/
} AM_IMG_DecodePara_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

#ifdef IMG_BMP
/**\brief 加载BMP图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
extern AM_ErrorCode_t AM_IMG_LoadBMP(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img);
#endif

#ifdef IMG_PNG
/**\brief 加载PNG图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
extern AM_ErrorCode_t AM_IMG_LoadPNG(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img);
#endif

#ifdef IMG_GIF
/**\brief 加载GIF图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_gif.h)
 */
extern AM_ErrorCode_t AM_IMG_LoadGIF(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img);
#endif

/**\brief 加载JPEG图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
extern AM_ErrorCode_t AM_IMG_LoadJPEG(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img);

#ifdef IMG_TIFF
/**\brief 加载TIFF图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
extern AM_ErrorCode_t AM_IMG_LoadTIFF(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img);
#endif

/**\brief 加载图片
 * \param[in] name 图片文件名
 * \param[in] para 解码参数
 * \param[out] img 返回加载的绘图表面
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_img.h)
 */
extern AM_ErrorCode_t AM_IMG_Load(const char *name, const AM_IMG_DecodePara_t *para, AM_OSD_Surface_t **img);


#ifdef __cplusplus
}
#endif

#endif

