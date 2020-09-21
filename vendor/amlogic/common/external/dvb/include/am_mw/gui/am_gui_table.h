/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 表格布局器
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-08: create the document
 ***************************************************************************/

#ifndef _AM_GUI_TABLE_H
#define _AM_GUI_TABLE_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_TABLE(t)  ((AM_GUI_Table_t*)(t))

#define AM_GUI_TABLE_CELL(c)  ((AM_GUI_TableCell_t*)(c))

/**\brief 行高度扩展*/
#define AM_GUI_TABLE_ROW_FL_EXPAND   1

/**\brief 行高读以百分比表示*/
#define AM_GUI_TABLE_ROW_FL_PERCENT  2

/**\brief 行高度*/
#define AM_GUI_TABLE_ROW_FL_SIZE     4

/**\brief 单元格宽度扩展*/
#define AM_GUI_TABLE_CELL_FL_EXPAND  1

/**\brief 单元格宽度以百分比表示*/
#define AM_GUI_TABLE_CELL_FL_PERCENT 2

/**\brief 单元格宽度*/
#define AM_GUI_TABLE_CELL_FL_SIZE    4

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 单元格控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;   /**< 控件基本数据*/
	int                row;      /**< 单元格开始的行数*/
	int                col;      /**< 单元格开始的列数*/
	int                hspan;    /**< 单元格占用的列数*/
	int                vspan;    /**< 单元格占用的行数*/
} AM_GUI_TableCell_t;

/**\brief 表格控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;   /**< 控件基本数据*/
	int                curr_row; /**< 当前行*/
	int                row;      /**< 列表行数*/
	int                col;      /**< 列表列数*/
	AM_GUI_Widget_t  **items;    /**< 元素表*/
	int               *min_size; /**< 列最小宽度*/
	int               *max_size; /**< 列最大宽度*/
} AM_GUI_Table_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新表格控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] tab 返回新的表格控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TableCreate(AM_GUI_t *gui, AM_GUI_Table_t **tab);

/**\brief 初始化一个表格控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] tab 表格控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TableInit(AM_GUI_t *gui, AM_GUI_Table_t *tab);

/**\brief 释放一个表格控件内部相关资源
 * \param[in] tab 表格控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TableRelease(AM_GUI_Table_t *tab);

/**\brief 向表格中添加一行
 * \param[in] tab 表格控件指针
 * \param flags 行高度标志
 * \param height 行占用的高度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TableAppendRow(AM_GUI_Table_t *tab, int flags, int height);

/**\brief 向表格中增加一个单元格
 * \param[in] tab 表格控件指针
 * \param[in] widget 添加的控件
 * \param flags 控件宽度标志
 * \param width 控件占用的宽度
 * \param col 控件开始的列数
 * \param hspan 控件占用的列数
 * \param vspan 控件所占的行数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_TableAppendCell(AM_GUI_Table_t *tab, AM_GUI_Widget_t *widget, int flags, int width, int col, int hspan, int vspan);

#ifdef __cplusplus
}
#endif

#endif

