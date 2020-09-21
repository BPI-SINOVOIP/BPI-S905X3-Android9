/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 配置文件管理模块
 *提供配置文件的解析、保存等基本功能。
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#ifndef _AM_CFG_H
#define _AM_CFG_H

#include "am_types.h"
#include <stdio.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief 配置文件模块错误代码*/
enum AM_CFG_ErrorCode
{
	AM_CFG_ERROR_BASE=AM_ERROR_BASE(AM_MOD_CFG),
	AM_CFG_ERR_CANNOT_OPEN_FILE,    /**< 无法打开配置文件*/
	AM_CFG_ERR_FILE_IO,		/**< 文件输入输出错误*/
	AM_CFG_ERR_NO_MEM,		/**< 内存不足*/
	AM_CFG_ERR_SYNTAX,		/**< 配置文件语法错误*/
	AM_CFG_ERR_BAD_FOTMAT,		/**< 键值格式错误*/
	AM_CFG_ERR_UNKNOWN_TAG,		/**< 解析到未知的标记*/
	AM_CFG_ERR_NOT_SUPPORT,		/**< 不支持的格式*/
	AM_CFG_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 配置文件输出过程中使用的数据*/
typedef struct
{
	FILE    *fp;                    /**< 输出文件*/
	int      sec_level;             /**< Section嵌套层数*/
} AM_CFG_OutputContext_t;

/**\brief 配置文件解析过程中的键回调函数
 *AM_CFG_Parse过程中，每次解析到一个形如"keyname=value"的键值，调用回调函数
 *将键值返回。
 */
typedef AM_ErrorCode_t (*AM_CFG_KeyCb_t) (void *user_data, const char *key, const char *value);

/**\brief 配置文件解析过程中Section开始回调函数
 *AM_CFG_Parse过程中，每次解析到一个形如"sectionname{"的section起始标志，调用回调函数。
 */
typedef AM_ErrorCode_t (*AM_CFG_SecBeginCb_t) (void *user_data, const char *section);

/**\brief 配置文件解析过程中Section结束回调函数
 *AM_CFG_Parse过程中，每次解析到一个形如"}"的section结束标志，调用回调函数。
 */
typedef AM_ErrorCode_t (*AM_CFG_SecEndCb_t) (void *user_data, const char *section);


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 解析一个配置文件
 * \param[in] path 配置文件路径
 * \param sec_begin_cb section开始回调函数
 * \param key_cb 键回调函数
 * \param sec_end_cb section结束回调函数
 * \param[in] user_data 传递给回调函数的用户数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_Input(const char *path,
                                   AM_CFG_SecBeginCb_t sec_begin_cb,
                                   AM_CFG_KeyCb_t key_cb,
                                   AM_CFG_SecEndCb_t sec_end_cb,
                                   void *user_data);

/**\brief 开始配置文件输出过程
 *开始输出过程包括打开文件和初始化AM_CFG_OutputContext_t结构
 * \param[in] path 输出配置文件路径
 * \param[out] cx 输出过程中间数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_BeginOutput(const char *path,
                                   AM_CFG_OutputContext_t *cx);

/**\brief 结束配置文件输出过程
 *释放AM_CFG_OutputContext_t中分配的资源并关闭文件
 * \param[in] cx 输出过程中间数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_EndOutput(AM_CFG_OutputContext_t *cx);

/**\brief 将字符串值转化为boolean值
 * \param[in] value 字符串值
 * \param[out] pv 输出整数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_Value2Bool(const char *value, AM_Bool_t *pv);

/**\brief 将字符串值转化为整数
 * \param[in] value 字符串值
 * \param[out] pv 输出整数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_Value2Int(const char *value, int *pv);

/**\brief 将字符串值转化为双精度数
 * \param[in] value 字符串值
 * \param[out] pv 输出双精度数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_Value2Double(const char *value, double *pv);

/**\brief 将字符串值转化为IP地址
 * \param[in] value 字符串值
 * \param[out] addr 输出IP地址的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_Value2IP(const char *value, struct in_addr *addr);

/**\brief 将字符串值转化为IPV6
 * \param[in] value 字符串值
 * \param[out] addr 输出IPV6地址的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_Value2IP6(const char *value, struct in6_addr *addr);

/**\brief 保存一个Boolean型的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreBool(AM_CFG_OutputContext_t *cx, const char *key, AM_Bool_t v);

/**\brief 保存一个十进制整数格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreDec(AM_CFG_OutputContext_t *cx, const char *key, int v);

/**\brief 保存一个八进制整数格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreOct(AM_CFG_OutputContext_t *cx, const char *key, int v);

/**\brief 保存一个十六进制整数格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreHex(AM_CFG_OutputContext_t *cx, const char *key, int v);

/**\brief 保存一个双精度格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreDouble(AM_CFG_OutputContext_t *cx, const char *key, double v);

/**\brief 保存一个IP地址的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] addr IP地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreIP(AM_CFG_OutputContext_t *cx, const char *key, struct in_addr *addr);

/**\brief 保存一个IPV6地址的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] addr IPV6地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreIP6(AM_CFG_OutputContext_t *cx, const char *key, struct in6_addr *addr);

/**\brief 保存一个字符串格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] str 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_StoreStr(AM_CFG_OutputContext_t *cx, const char *key, const char *str);

/**\brief 开始保存一个section
 *保存一个Section到栈中，此后所有的Store操作都在Section中进行，直到调用AM_CFG_EndSection
 * \param[in,out] cx 配置输出上下文
 * \param[in] sec Section名称
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_BeginSection(AM_CFG_OutputContext_t *cx, const char *sec);

/**\brief 结束一个section的保存。此后的store操作在上一级section中进行
 * \param[in,out] cx 配置输出上下文
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
extern AM_ErrorCode_t AM_CFG_EndSection(AM_CFG_OutputContext_t *cx);

#ifdef __cplusplus
}
#endif

#endif

