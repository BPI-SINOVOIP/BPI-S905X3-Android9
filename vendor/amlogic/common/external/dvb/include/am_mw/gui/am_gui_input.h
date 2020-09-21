/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 输入框控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_INPUT_H
#define _AM_GUI_INPUT_H

#include "am_gui_widget.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_INPUT(i)  ((AM_GUI_Input_t*)(i))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 输入数据类型*/
typedef enum
{
	AM_GUI_INPUT_NUMBER,  /**< 数字输入*/
	AM_GUI_INPUY_TEXT     /**< 文本输入*/
} AM_GUI_InputType_t;

/**\brief 编辑框输入模式*/
typedef enum
{
	AM_GUI_INPUT_INSERT,  /**< 在光标处插入字符*/
	AM_GUI_INPUT_REPLACE  /**< 替换光标处的字符*/
} AM_GUI_InputMode_t;

/**\brief 输入框控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;     /**< 基本控件数据*/
	AM_GUI_InputType_t type;       /**< 输入数据类型*/
	AM_GUI_InputMode_t mode;       /**< 输入框编辑模式*/
	int                cursor_pos; /**< 光标当前位置*/
	int                num_len;    /**< 数字输入的位数*/
	int                num_min;    /**< 数字输入的最小值*/
	int                num_max;    /**< 数字输入的最大值*/
	int                text_len;   /**< 文本输入的长度*/
	int                text_size;  /**< 字符缓冲区大小*/
	char              *text_buf;   /**< 字符缓冲区*/
} AM_GUI_Input_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新输入框控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] inp 返回新的输入框控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputCreate(AM_GUI_t *gui, AM_GUI_Input_t **inp);

/**\brief 初始化一个输入框控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] inp 输入框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputInit(AM_GUI_t *gui, AM_GUI_Input_t *inp);

/**\brief 释放一个输入框控件内部相关资源
 * \param[in] inp 输入框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputRelease(AM_GUI_Input_t *inp);

/**\brief 设定一个输入框为数字输入框
 * \param[in] inp 输入框控件
 * \param len 数字位数
 * \param min 数字最大值
 * \param max 数字最小值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputSetNumberMode(AM_GUI_Input_t *inp, int len, int min, int max);

/**\brief 设定一个输入框为文本输入框
 * \param[in] inp 输入框控件
 * \param len 文字长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputSetTextMode(AM_GUI_Input_t *inp, int len);

/**\brief 设定一个输入框的标记模式
 * \param[in] inp 输入框控件
 * \param mode 编辑模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputSetMode(AM_GUI_Input_t *inp, AM_GUI_InputMode_t mode);

/**\brief 设定一个文本输入框内的文字
 * \param[in] inp 输入框控件
 * \param[in] text 文本字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputSetText(AM_GUI_Input_t *inp, const char *text);

/**\brief 设定一个数字输入框内数字
 * \param[in] inp 输入框控件
 * \param num 数字
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputSetNumber(AM_GUI_Input_t *inp, int num);

/**\brief 取得一个文本输入框内的文字
 * \param[in] inp 输入框控件
 * \param[out] ptext 返回文本字符串指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputGetText(AM_GUI_Input_t *inp, const char **ptext);

/**\brief 取得一个数字输入框内的数字
 * \param[in] inp 输入框控件
 * \param[out] num 返回当前数字
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_InputGetNumber(AM_GUI_Input_t *inp, int *num);

#ifdef __cplusplus
}
#endif

#endif

