/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Subtitle模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-07-18: create the document
 ***************************************************************************/


#ifndef _AM_SUB_H
#define _AM_SUB_H

#include "am_types.h"
#include "am_osd.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Type definitions
 ***************************************************************************/


/**\brief Subtitle模块错误代码*/
enum AM_SUB_ErrorCode
{
	AM_SUB_ERROR_BASE=AM_ERROR_BASE(AM_MOD_SUB),
	AM_SUB_ERR_INVALID_PARAM,   /**< 参数无效*/
	AM_SUB_ERR_INVALID_HANDLE,  /**< 句柄无效*/
	AM_SUB_ERR_NOT_SUPPORTED,   /**< 不支持的操作*/
	AM_SUB_ERR_CREATE_DECODE,   /**< 打开SUBTITLE解码器失败*/
	AM_SUB_ERR_OPEN_PES,        /**< 打开PES通道失败*/
	AM_SUB_ERR_SET_BUFFER,      /**< 失置PES 缓冲区失败*/
	AM_SUB_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_SUB_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_SUB_ERR_NOT_RUN,            /**< 无法创建线程*/
	AM_SUB_INIT_DISPLAY_FAILED,    /**< 初始化显示屏幕失败*/
	AM_SUB_ERR_END
};

/**\brief 显示区域信息*/
typedef struct
{
    int          left;/**< 区域左顶点横坐标*/
    int          top; /**< 区域左顶点纵坐标*/

    unsigned int          width;     /**< 区域宽度*/
    unsigned int          height;    /**< 区域高度*/
    AM_OSD_Palette_t      palette;   /**< 调色板*/


    unsigned int          background;/**< 背景色*/
    unsigned char*        buffer;    /**< 像素信息*/

    unsigned int          textfg;    /**< 文字显示色彩*/
    unsigned int          textbg;    /**< 文字背景色*/

    unsigned int          length;    /**< 文字长度*/
    unsigned short*       text;      /**< 文字内容*/
}AM_SUB_Region_t;

/**\brief 显示屏幕信息*/
typedef struct
{
    int          left;/**< 屏幕左顶点横坐标*/
    int          top; /**<  屏幕左顶点纵坐标*/
    unsigned int          width;     /**< 屏幕宽度*/
    unsigned int          height;    /**< 屏幕高度*/
    unsigned int          region_num;/**< 显示区域个数*/
    AM_SUB_Region_t*      region;    /**< 区域结构指针*/
}AM_SUB_Screen_t;

/**\brief 显示screan
 * screen为显示信息
 */
typedef void (*AM_SUB_ShowCb_t)(AM_SUB_Screen_t* screen);

/**\brief 清除screen
 * screen为清除信息
 */
typedef void (*AM_SUB_ClearCb_t)(AM_SUB_Screen_t* screen);


/**\brief 开始处理subtitle(包括解析subtitle)
 * \param dmx_dev_id  demux 设备id号
 * \param pid  packet id
 * \param composition_id 合成page 编号
 * \param ancillary_id  辅助page编号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */

AM_ErrorCode_t AM_SUB_Start(int dmx_dev_id, int pid, int composition_id, int ancillary_id);

/**\brief 停止处理subtitle
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */
AM_ErrorCode_t AM_SUB_Stop(void);

/**\brief 初始化subtitle 模块
 * \param  buffer_size  分配缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */
AM_ErrorCode_t AM_SUB_Init(int buffer_size);

/**\brief 卸载subtitle 模块,请先调用AM_SUB_Stop
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */

AM_ErrorCode_t AM_SUB_Quit(void);

/**\brief 设置屏幕显示(清除)回调函数
 * \param  ssc 显示screen 回调 
 * \param  scc 清除screen 回调
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */
AM_ErrorCode_t AM_SUB_RegisterUpdateCbs(AM_SUB_ShowCb_t ssc, AM_SUB_ClearCb_t scc);

/**\brief Subtitle 显示与隐藏控制接口
 * \param  isShow  非零为显示Subtitle,零为隐藏Subtitle
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_sub.h)
 */
AM_ErrorCode_t AM_SUB_Show(AM_Bool_t isShow);

#ifdef __cplusplus
}
#endif



#endif
