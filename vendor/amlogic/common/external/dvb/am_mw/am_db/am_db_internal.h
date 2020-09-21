/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_db_internal.h
 * \brief 数据库模块内部数据头文件
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-22: create the document
 ***************************************************************************/

#ifndef _AM_DB_INTERNAL_H
#define _AM_DB_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
 
/**\brief 在select语句中最多支持的column个数*/
#define MAX_SELECT_COLMUNS 256

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 数据类型*/
enum AM_DB_DataType
{
	AM_DB_INT,
	AM_DB_FLOAT,
	AM_DB_STRING
};

/**\brief 表内部数据*/
typedef struct
{
	const char *table_name; /**< 表名称，用于创建表*/
	const char **fields;	/**< 字段定义*/
	int (*get_size)(void);  /**< 计算字段个数*/
}AM_DB_Table_t;

/**\brief Select数据*/
typedef struct
{
	int col;							/**< select 中指定的字段个数*/
	int types[MAX_SELECT_COLMUNS];		/**< select 各字段数据类型记录*/
	int sizes[MAX_SELECT_COLMUNS];		/**< select 各字段数据类型长度*/
	void *dat[MAX_SELECT_COLMUNS];		/**< select 各字段数据存储地址记录*/
}AM_DB_TableSelect_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

