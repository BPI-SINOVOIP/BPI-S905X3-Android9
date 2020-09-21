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
 * \brief 配置文件输出
 *保存配置文件。
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_cfg.h>
#include <am_mem.h>
#include <am_debug.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief Output prefixed space*/
static void AM_INLINE cfg_prefix(AM_CFG_OutputContext_t *cx)
{
	int i;
	
	for(i=0; i<cx->sec_level; i++)
	{
		fputc('\t', cx->fp);
	}
}

/**\brief Output key name*/
static void AM_INLINE cfg_key(AM_CFG_OutputContext_t *cx, const char *name)
{
	cfg_prefix(cx);
	fprintf(cx->fp, "%s=", name);
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 开始配置文件输出过程
 *开始输出过程包括打开文件和初始化AM_CFG_OutputContext_t结构
 * \param[in] path 输出配置文件路径
 * \param[out] cx 输出过程中间数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_BeginOutput(const char *path,
	AM_CFG_OutputContext_t *cx)
{
	assert(path && cx);
	
	memset(cx, 0, sizeof(AM_CFG_OutputContext_t));
	
	cx->fp = fopen(path, "w");
	if(!cx->fp)
	{
		AM_DEBUG(1, "%s", strerror(errno));
		return AM_CFG_ERR_CANNOT_OPEN_FILE;
	}
	
	return AM_SUCCESS;
}

/**\brief 结束配置文件输出过程
 *释放AM_CFG_OutputContext_t中分配的资源并关闭文件
 * \param[in] cx 输出过程中间数据
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_EndOutput(AM_CFG_OutputContext_t *cx)
{
	assert(cx);
	
	if(cx->fp)
	{
		fclose(cx->fp);
	}
	
	if(cx->sec_level)
	{
		AM_DEBUG(1, "section mismatch");
	}
	
	return AM_SUCCESS;
}

/**\brief 保存一个Boolean型的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreBool(AM_CFG_OutputContext_t *cx, const char *key, AM_Bool_t v)
{
	assert(cx && key);
	
	return AM_CFG_StoreStr(cx, key, v?"yes":"no");
}

/**\brief 保存一个十进制整数格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreDec(AM_CFG_OutputContext_t *cx, const char *key, int v)
{
	char buf[32];
	
	assert(cx && key);
	
	snprintf(buf, sizeof(buf), "%d", v);
	
	return AM_CFG_StoreStr(cx, key, buf);
}

/**\brief 保存一个八进制整数格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreOct(AM_CFG_OutputContext_t *cx, const char *key, int v)
{
	char buf[32];
	
	assert(cx && key);
	
	snprintf(buf, sizeof(buf), "0%o", v);
	
	return AM_CFG_StoreStr(cx, key, buf);
}

/**\brief 保存一个十六进制整数格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreHex(AM_CFG_OutputContext_t *cx, const char *key, int v)
{
	char buf[32];
	
	assert(cx && key);
	
	snprintf(buf, sizeof(buf), "0x%x", v);
	
	return AM_CFG_StoreStr(cx, key, buf);
}

/**\brief 保存一个双精度格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] v 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreDouble(AM_CFG_OutputContext_t *cx, const char *key, double v)
{
	char buf[64];
	
	assert(cx && key);
	
	snprintf(buf, sizeof(buf), "%le", v);
	
	return AM_CFG_StoreStr(cx, key, buf);
}

/**\brief 保存一个IP地址的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] addr IP地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreIP(AM_CFG_OutputContext_t *cx, const char *key, struct in_addr *addr)
{
	char buf[INET_ADDRSTRLEN];
	
	assert(cx && key && addr);
	
	if (!inet_ntop(AF_INET, addr, buf, sizeof(buf)))
	{
		AM_DEBUG(1, "%s", strerror(errno));
		return AM_CFG_ERR_NOT_SUPPORT;
	}
	
	return AM_CFG_StoreStr(cx, key, buf);
}

/**\brief 保存一个IPV6地址的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] addr IPV6地址
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreIP6(AM_CFG_OutputContext_t *cx, const char *key, struct in6_addr *addr)
{
	char buf[INET6_ADDRSTRLEN];
	
	assert(cx && key && addr);
	
	if (!inet_ntop(AF_INET6, addr, buf, sizeof(buf)))
	{
		AM_DEBUG(1, "%s", strerror(errno));
		return AM_CFG_ERR_NOT_SUPPORT;
	}
	
	return AM_CFG_StoreStr(cx, key, buf);
}

/**\brief 保存一个字符串格式的键值
 * \param[in,out] cx 配置输出上下文
 * \param[in] key 键名称
 * \param[in] str 键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_StoreStr(AM_CFG_OutputContext_t *cx, const char *key, const char *str)
{
	assert(cx && key);
	
	cfg_prefix(cx);
	fprintf(cx->fp, "%s=\"", key);
	
	if(str)
	{
		const char *pch=str;
		
		while(*pch)
		{
			switch(*pch)
			{
				case '\n':
					fputc('\\', cx->fp);
					fputc('n', cx->fp);
				break;
				case '\r':
					fputc('\\', cx->fp);
					fputc('r', cx->fp);
				break;
				case '\t':
					fputc('\\', cx->fp);
					fputc('t', cx->fp);
				break;
				case '\v':
					fputc('\\', cx->fp);
					fputc('v', cx->fp);
				break;
				case '\f':
					fputc('\\', cx->fp);
					fputc('f', cx->fp);
				break;
				case '\a':
					fputc('\\', cx->fp);
					fputc('a', cx->fp);
				break;
				case '\b':
					fputc('\\', cx->fp);
					fputc('b', cx->fp);
				break;
				case '\\':
					fputc('\\', cx->fp);
					fputc('\\', cx->fp);
				break;
				default:
					fputc(*pch, cx->fp);
				break;
			}
			pch++;
		}
	}
	
	fprintf(cx->fp, "\"\n");
	
	return AM_SUCCESS;
}

/**\brief 开始保存一个section
 *保存一个Section到栈中，此后所有的Store操作都在Section中进行，直到调用AM_CFG_EndSection
 * \param[in,out] cx 配置输出上下文
 * \param[in] sec Section名称
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_BeginSection(AM_CFG_OutputContext_t *cx, const char *sec)
{
	assert(cx && sec);
	
	cfg_prefix(cx);
	fprintf(cx->fp, "%s{\n", sec);
	
	cx->sec_level++;
	
	return AM_SUCCESS;
}

/**\brief 结束一个section的保存。此后的store操作在上一级section中进行
 * \param[in,out] cx 配置输出上下文
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_EndSection(AM_CFG_OutputContext_t *cx)
{
	assert(cx);
	
	if(!cx->sec_level)
	{
		AM_DEBUG(1, "section mismatch");
		return AM_CFG_ERR_SYNTAX;
	}
	
	cx->sec_level--;
	cfg_prefix(cx);
	fprintf(cx->fp, "}\n");
	
	return AM_SUCCESS;
}

