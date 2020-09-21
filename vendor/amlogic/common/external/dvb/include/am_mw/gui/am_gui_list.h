/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 列表控件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-02: create the document
 ***************************************************************************/

#ifndef _AM_GUI_LIST_H
#define _AM_GUI_LIST_H

#include "am_gui_label.h"
#include "am_gui_scroll.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AM_GUI_LIST(l)    ((AM_GUI_List_t*)(l))

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 列表控件*/
typedef struct AM_GUI_List AM_GUI_List_t;

/**\brief 列表行控件*/
typedef struct
{
	AM_GUI_Widget_t    widget;  /**< 基本控件数据*/
	AM_GUI_List_t     *list;    /**< 行所属的列表*/
	AM_GUI_Flow_t     *flows;   /**< 单元格数据数组*/
} AM_GUI_ListRow_t;

/**\brief 列表控件*/
struct AM_GUI_List
{
	AM_GUI_Widget_t    widget;  /**< 基本控件数据*/
	AM_GUI_ListRow_t  *caption; /**< 标题行*/
	AM_GUI_ListRow_t **rows;    /**< 行数组*/
	AM_GUI_Scroll_t   *scroll;  /**< 列表中的滚动条控件*/
	int                row;     /**< 行数*/
	int                col;     /**< 列数*/
	int                focus;   /**< 当前焦点*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 分配一个新列表控件并初始化
 * \param[in] gui 控件所属的GUI控制器
 * \param[out] list 返回新的列表控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListCreate(AM_GUI_t *gui, AM_GUI_List_t **list);

/**\brief 初始化一个列表控件
 * \param[in] gui 控件所属的GUI控制器
 * \param[in] list 列表控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListInit(AM_GUI_t *gui, AM_GUI_List_t *list);

/**\brief 释放一个列表控件内部相关资源
 * \param[in] list 列表控件指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListRelease(AM_GUI_List_t *list);

/**\brief 释放一个列表控件内部资源并释放内存
 * \param[in] list 要释放的列表控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListDestroy(AM_GUI_List_t *list);

/**\brief 设定列表控件的列数
 * \param[in] list 列表控件
 * \param col 列数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListSetColumn(AM_GUI_List_t *list, int col);

/**\brief 删除列表中的全部内容
 * \param[in] list 列表控件
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListClear(AM_GUI_List_t *list);

/**\brief 取得列表中的一行,当row==NULL时表示去删除一行
 * \param[in] list 列表控件
 * \param rid 行ID
 * \param[out] row 返回标题行，row==NULL表示去删除此行
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListGetRow(AM_GUI_List_t *list, int rid, AM_GUI_ListRow_t **row);

/**\brief 取得列表的标题行,当row==NULL时表示去除列表的标题行
 * \param[in] list 列表控件
 * \param[out] row 返回标题行，row==NULL表示去除列表的标题行
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListGetCaptionRow(AM_GUI_List_t *list, AM_GUI_ListRow_t **row);

/**\brief 向列表中添加一行
 * \param[in] list 列表控件
 * \param[out] row 返回新添加的行
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListAppendRow(AM_GUI_List_t *list, AM_GUI_ListRow_t **row);

/**\brief 设定列表行单元格中的文字
 * \param[in] row 列表行控件
 * \param col 列ID
 * \param[in] text 文本字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListRowSetCellText(AM_GUI_ListRow_t *row, int col, const char *text);

/**\brief 设定列表行单元格中的图像
 * \param[in] row 列表行控件
 * \param col 列ID
 * \param[in] image 图像数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListRowSetCellImage(AM_GUI_ListRow_t *row, int cell, AM_OSD_Surface_t *image);

/**\brief 设定列表标题行每个单元格内容
 * \param[in] list 列表控件
 * \param[in] fmt 数据类型字符串,字符串中每个字符表示一列的数据类型,'i'表示图像数据,'t'表示文字数据,'n'表示数据为空，其他值则不更新
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListSetCaptionData(AM_GUI_List_t *list, const char *fmt,...);

/**\brief 向列表中添加一行，并设定行每个单元格内容
 * \param[in] list 列表控件
 * \param[in] fmt 数据类型字符串,字符串中每个字符表示一列的数据类型,'i'表示图像数据,'t'表示文字数据,'n'表示数据为空，其他值则不更新
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListAppendRowData(AM_GUI_List_t *list, const char *fmt,...);

/**\brief 设定当前列表的焦点行
 * \param[in] list 列表控件
 * \param row 焦点行ID
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
extern AM_ErrorCode_t AM_GUI_ListSetFocus(AM_GUI_List_t *list, int row);

#ifdef __cplusplus
}
#endif

#endif

