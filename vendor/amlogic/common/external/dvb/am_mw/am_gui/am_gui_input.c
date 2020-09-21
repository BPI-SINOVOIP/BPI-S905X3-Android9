#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
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
 * \date 2010-12-01: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_mem.h>
#include <assert.h>
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/


/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 分配一个新输入框控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] inp 返回新的输入框控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputCreate(AM_GUI_t *gui, AM_GUI_Input_t **inp)
{
	AM_GUI_Input_t *i;
	AM_ErrorCode_t ret;
	
	assert(gui && inp);
	
	i = (AM_GUI_Input_t*)malloc(sizeof(AM_GUI_Input_t));
	if(!i)
	{
		AM_DEBUG(1, "not enough memory");
		return AM_GUI_ERR_NO_MEM;
	}
	
	ret = AM_GUI_InputInit(gui, i);
	if(ret!=AM_SUCCESS)
	{
		free(i);
		return ret;
	}
	
	*inp = i;
	return AM_SUCCESS;
}

/**\brief 初始化一个输入框控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] inp 输入框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputInit(AM_GUI_t *gui, AM_GUI_Input_t *inp)
{
}

/**\brief 释放一个输入框控件内部相关资源
 * \param[in] inp 输入框控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputRelease(AM_GUI_Input_t *inp)
{
}

/**\brief 设定一个输入框为数字输入框
 * \param[in] inp 输入框控件
 * \param len 数字位数
 * \param min 数字最大值
 * \param max 数字最小值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputSetNumberMode(AM_GUI_Input_t *inp, int len, int min, int max)
{
}

/**\brief 设定一个输入框为文本输入框
 * \param[in] inp 输入框控件
 * \param len 文字长度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputSetTextMode(AM_GUI_Input_t *inp, int len)
{
}

/**\brief 设定一个输入框的标记模式
 * \param[in] inp 输入框控件
 * \param mode 编辑模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputSetMode(AM_GUI_Input_t *inp, AM_GUI_InputMode_t mode)
{
}

/**\brief 设定一个文本输入框内的文字
 * \param[in] inp 输入框控件
 * \param[in] text 文本字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputSetText(AM_GUI_Input_t *inp, const char *text)
{
}

/**\brief 设定一个数字输入框内数字
 * \param[in] inp 输入框控件
 * \param num 数字
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputSetNumber(AM_GUI_Input_t *inp, int num)
{
}

/**\brief 取得一个文本输入框内的文字
 * \param[in] inp 输入框控件
 * \param[out] ptext 返回文本字符串指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputGetText(AM_GUI_Input_t *inp, const char **ptext)
{
}

/**\brief 取得一个数字输入框内的数字
 * \param[in] inp 输入框控件
 * \param[out] num 返回当前数字
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_GUI_InputGetNumber(AM_GUI_Input_t *inp, int *num)
{
}

