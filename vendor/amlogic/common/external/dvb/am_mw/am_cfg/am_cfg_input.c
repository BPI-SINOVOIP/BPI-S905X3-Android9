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
 * \brief 配置文件输入
 *解析配置文件。
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_cfg.h>
#include <am_mem.h>
#include <am_debug.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief Section栈帧*/
typedef struct AM_CFG_SecStack AM_CFG_SecStack_t;

/**\brief Section栈帧*/
struct AM_CFG_SecStack
{
	AM_CFG_SecStack_t   *bottom;         /**< 指向底部的Section栈帧*/
	char                *name;           /**< Section的名称*/
};

/**\brief 配置文件分析器*/
typedef struct
{
	FILE                *fp;             /**< 输入文件*/
	const char          *path;           /**< 输入文件路径*/
	int                  line;           /**< 当前行号*/
	AM_CFG_SecStack_t   *sec_stack;      /**< Section栈*/
	char                *buffer;         /**< 输入字符串缓冲区*/
	int                  size;           /**< 输入字符串缓冲区大小*/
	int                  leng;           /**< 输入字符串的长度*/
	AM_CFG_SecBeginCb_t  sec_begin_cb;   /**< Section开始回调*/
	AM_CFG_KeyCb_t       key_cb;         /**< 键回调*/
	AM_CFG_SecEndCb_t    sec_end_cb;     /**< Section结束回调*/
	void                *user_data;      /**< 回调的用户数据参数*/
} AM_CFG_Parser_t;

/****************************************************************************
 * Static functions
 ***************************************************************************/

/**\brief output error message*/
static void cfg_error(AM_CFG_Parser_t *parser)
{
	fprintf(stderr, "AM_CFG: \"%s\" error in line %d\n", parser->path, parser->line);
}

/**\brief 重新设置分析器内的字符缓冲区大小*/
static AM_INLINE AM_ErrorCode_t cfg_resize_buffer (AM_CFG_Parser_t *parser, int size)
{
	char *nbuf;
	
	if(size<=parser->size)
	{
		return AM_SUCCESS;
	}
	
	size = AM_MAX(size, parser->size*2);
	size = AM_MAX(size, 64);
	
	nbuf = AM_MEM_Realloc(parser->buffer, size*sizeof(char));
	if(!nbuf)
	{
		return AM_CFG_ERR_NO_MEM;
	}
	
	parser->buffer = nbuf;
	parser->size = size;
	
	return AM_SUCCESS;
}

/**\brief 从输入文件中读一个字符*/
static AM_INLINE AM_ErrorCode_t cfg_getc (AM_CFG_Parser_t *parser, int *pch)
{
	int ch = fgetc(parser->fp);
	
	if ((ch==EOF) && ferror(parser->fp))
	{
		AM_DEBUG(1, "%s", strerror(ferror(parser->fp)));
		return AM_CFG_ERR_FILE_IO;
	}
	
	if(ch=='\n')
		parser->line++;
	
	*pch = ch;
	return AM_SUCCESS;
}

/**\brief 将一个字符放入缓冲区*/
static AM_INLINE AM_ErrorCode_t cfg_putc (AM_CFG_Parser_t *parser, char ch)
{
	AM_TRY(cfg_resize_buffer(parser, parser->leng+1));
	
	parser->buffer[parser->leng++]=ch;
	return AM_SUCCESS;
}

/**\brief 取得缓冲区中的字符串指针*/
static AM_INLINE AM_ErrorCode_t cfg_get_str (AM_CFG_Parser_t *parser, int begin, const char **pstr)
{
	if(parser->leng==begin)
	{
		*pstr = NULL;
		return AM_SUCCESS;
	}
	
	AM_TRY(cfg_putc(parser, 0));
	
	*pstr = parser->buffer+begin;
	return AM_SUCCESS;
}

/**\brief 将一个section压入section栈*/
static AM_ErrorCode_t cfg_push_sec (AM_CFG_Parser_t *parser)
{
	AM_CFG_SecStack_t *sec;
	const char *name;
	
	AM_TRY(cfg_get_str(parser, 0, &name));
	
	sec = AM_MEM_ALLOC_TYPE0(AM_CFG_SecStack_t);
	if(!sec)
		return AM_CFG_ERR_NO_MEM;
	
	sec->name = AM_MEM_Strdup(name);
	if(!sec->name)
		return AM_CFG_ERR_NO_MEM;
	
	sec->bottom = parser->sec_stack;
	parser->sec_stack = sec;
	
	return AM_SUCCESS;
}

/**\brief 从section中弹出一个栈帧*/
static void cfg_pop_sec (AM_CFG_Parser_t *parser)
{
	AM_CFG_SecStack_t *sec = parser->sec_stack;
	
	if(!sec)
		return;
	
	parser->sec_stack = sec->bottom;
	
	AM_MEM_Free(sec->name);
	AM_MEM_Free(sec);
}

/**\brief 从输入文件中读取一行数据*/
static AM_ErrorCode_t cfg_read_line (AM_CFG_Parser_t *parser, int *pend)
{
	int ch, leng;
	const char *key, *value;
	
	/*Clear the buffer*/
	parser->leng = 0;
	
	/*Eatup space*/
	do
	{
		AM_TRY(cfg_getc(parser, &ch));
	}
	while((ch!=EOF) && isspace(ch));
	
	if(ch==EOF)
	{
		*pend = 1;
		return AM_SUCCESS;
	}
	
	/*Section结束标志*/
	if(ch=='}')
	{
		if(!parser->sec_stack)
		{
			AM_DEBUG(1, "\'{\' mismatch");
			cfg_error(parser);
			return AM_CFG_ERR_SYNTAX;
		}
		
		if (parser->sec_end_cb)
		{
			const char *sec_name = parser->sec_stack->name;
			AM_TRY(parser->sec_end_cb(parser->user_data, sec_name?sec_name:""));
		}
		cfg_pop_sec(parser);
		return AM_SUCCESS;
	}
	else
	{
		/*读取名字*/
		while((ch!=EOF) && (ch!='=') && (ch!='{') && (ch!='#') && (ch!='\n'))
		{
			AM_TRY(cfg_putc(parser, ch));
			AM_TRY(cfg_getc(parser, &ch));
		}
	
		/*Section开始*/
		if(ch=='{')
		{
			AM_TRY(cfg_push_sec(parser));
			if(parser->sec_begin_cb)
			{
				const char *sec_name = parser->sec_stack->name;
				AM_TRY(parser->sec_begin_cb(parser->user_data, sec_name?sec_name:""));
			}
			return AM_SUCCESS;
		}
	
		/*去除键名末尾的空格*/
		leng = parser->leng;
	
		while((leng>0) && isspace(parser->buffer[leng-1]))
		{
			leng--;
		}
		parser->leng = leng;
	
		/*得到键名*/
		AM_TRY(cfg_get_str(parser, 0, &key));
	
		/*读取键值*/
		if(ch=='=')
		{
			int vpos, quot;
		
			/*清除空格/TAB*/
			do
			{
				AM_TRY(cfg_getc(parser, &ch));
			}
			while((ch!=EOF) && isspace(ch) && (ch!='\n'));
		
			vpos = parser->leng;
			quot = (ch=='\"')?AM_TRUE:AM_FALSE;
			if(quot)
			{
				while(1)
				{
					AM_TRY(cfg_getc(parser, &ch));
					if((ch==EOF)||(ch=='\n')) break;
					if(ch=='\\')
					{
						AM_TRY(cfg_getc(parser, &ch));
						if(ch==EOF) break;
						switch(ch)
						{
							case 'n':
								ch='\n';
							break;
							case 'r':
								ch='\r';
							break;
							case 't':
								ch='\t';
							break;
							case 'v':
								ch='\v';
							break;
							case 'f':
								ch='\f';
							break;
							case 'a':
								ch='\a';
							break;
							case 'b':
								ch='\b';
							break;
							case '\n':
								AM_TRY(cfg_getc(parser, &ch));
								if((ch==EOF)||(ch=='\n')) break;
							break;
							default:
							break;
						}
					}
				
					AM_TRY(cfg_putc(parser, ch));
				}
			}
			else
			{
				do
				{
					AM_TRY(cfg_putc(parser, ch));
					AM_TRY(cfg_getc(parser, &ch));
				}
				while((ch!=EOF) && (ch!='\n'));
			}
		
			/*去除键名末尾的空格*/
			leng = parser->leng;
		
			while((leng>vpos) && isspace(parser->buffer[leng-1]))
			{
				leng--;
			}
			parser->leng = leng;
		
			if (quot && (parser->buffer[leng-1]!='\"'))
			{
				AM_DEBUG(1, "\'\"\' mismatch");
				cfg_error(parser);
				return AM_CFG_ERR_SYNTAX;
			}
		
			if (quot) parser->leng--;
			AM_TRY(cfg_get_str(parser, vpos, &value));
		}
		else
		{
			value=NULL;
		}
	
		if ((key || value) && parser->key_cb)
		{
			AM_TRY(parser->key_cb(parser->user_data, key?key:"", value?value:""));
		}
	}
	
	/*Eatup comment*/
	while((ch!='\n') && (ch!=EOF))
	{
		AM_TRY(cfg_getc(parser, &ch));
	}
	
	if (ch==EOF) {
		*pend = 1;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t cfg_parse (AM_CFG_Parser_t *parser)
{
	int end = 0;
	
	do
	{
		AM_TRY(cfg_read_line(parser, &end));
	}
	while(!end);
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
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
 
AM_ErrorCode_t AM_CFG_Input(const char *path,
	AM_CFG_SecBeginCb_t sec_begin_cb,
	AM_CFG_KeyCb_t key_cb,
	AM_CFG_SecEndCb_t sec_end_cb,
	void *user_data)
{
	AM_CFG_Parser_t parser;
	FILE *fp;
	AM_ErrorCode_t ret;
	
	assert(path);
	
	if (!(fp = fopen(path, "r")))
	{
		AM_DEBUG(1, "%s", strerror(errno));
		return AM_CFG_ERR_CANNOT_OPEN_FILE;
	}
	
	memset(&parser, 0, sizeof(AM_CFG_Parser_t));
	
	parser.path = path;
	parser.line = 1;
	parser.fp = fp;
	parser.sec_begin_cb = sec_begin_cb;
	parser.sec_end_cb = sec_end_cb;
	parser.key_cb = key_cb;
	parser.user_data = user_data;
	
	ret = cfg_parse(&parser);
	
	if(parser.sec_stack)
	{
		AM_DEBUG(1, "\'}\' mismatch");
		cfg_error(&parser);
	}
	
	while(parser.sec_stack)
	{
		cfg_pop_sec(&parser);
	}
	
	if(parser.buffer)
		AM_MEM_Free(parser.buffer);
	
	fclose(fp);
	return ret;
}

/**\brief 将字符串值转化为boolean值
 * \param[in] value 字符串值
 * \param[out] pv 输出整数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_Value2Bool(const char *value, AM_Bool_t *pv)
{
	assert(value && pv);
	
	if (!strcasecmp(value,"y") || !strcasecmp(value,"yes") ||
		!strcasecmp(value,"t") || !strcasecmp(value,"true") ||
		!strcasecmp(value,"on") || !strcasecmp(value,"1"))
	{
		*pv = 1;
		return AM_SUCCESS;
	}
	
	if (!strcasecmp(value,"n") || !strcasecmp(value,"no") ||
		!strcasecmp(value,"f") || !strcasecmp(value,"false") ||
		!strcasecmp(value,"off") || !strcasecmp(value,"0"))
	{
		*pv = 0;
		return AM_SUCCESS;
	}
	
	return AM_CFG_ERR_BAD_FOTMAT;
}

/**\brief 将字符串值转化为整数
 * \param[in] value 字符串值
 * \param[out] pv 输出整数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_Value2Int(const char *value, int *pv)
{
	int rc;
	
	assert(value && pv);
	
	if(value[0]=='0')
	{
		if((value[1]=='x')||(value[1]=='X'))
		{
			rc = sscanf(value+2, "%x", pv);
		}
		else
		{
			rc = sscanf(value, "%o", pv);
		}
	}
	else
	{
		rc = sscanf(value, "%d", pv);
	}
	
	if(rc!=1)
	{
		AM_DEBUG(1, "key is not an integer");
		return AM_CFG_ERR_BAD_FOTMAT;
	}
	
	return AM_SUCCESS;
}

/**\brief 将字符串值转化为双精度数
 * \param[in] value 字符串值
 * \param[out] pv 输出双精度数的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t
AM_CFG_Value2Double(const char *value, double *pv)
{
	assert(value && pv);
	
	if(sscanf(value, "%le", pv)!=1)
	{
		AM_DEBUG(1, "key is not a double value");
		return AM_CFG_ERR_BAD_FOTMAT;
	}
	
	return AM_SUCCESS;
}

/**\brief 将字符串值转化为IP地址
 * \param[in] value 字符串值
 * \param[out] addr 输出IP地址的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_Value2IP(const char *value, struct in_addr *addr)
{
	assert(value && addr);
	
	if(inet_pton(AF_INET, value, addr)<=0)
	{
		AM_DEBUG(1, "key is not an IP address");
		return AM_CFG_ERR_BAD_FOTMAT;
	}
	
	return AM_SUCCESS;
}

/**\brief 将字符串值转化为IPV6地址
 * \param[in] value 字符串值
 * \param[out] addr 输出IPV6地址的指针
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_cfg.h)
 */
AM_ErrorCode_t AM_CFG_Value2IPV6(const char *value, struct in6_addr *addr)
{
	assert(value && addr);
	
	if(inet_pton(AF_INET6, value, addr)<=0)
	{
		AM_DEBUG(1, "key is not an IPV6 address");
		return AM_CFG_ERR_BAD_FOTMAT;
	}
	
	return AM_SUCCESS;
}

