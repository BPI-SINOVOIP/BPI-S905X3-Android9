/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 一些常用宏和辅助函数
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#ifndef _AM_UTIL_H
#define _AM_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief 函数inline属性*/
#define AM_INLINE        inline

/**\brief 计算数值_a和_b中的最大值*/
#define AM_MAX(_a,_b)    ((_a)>(_b)?(_a):(_b))

/**\brief 计算数值_a和_b中的最小值*/
#define AM_MIN(_a,_b)    ((_a)<(_b)?(_a):(_b))

/**\brief 计算数值_a的绝对值*/
#define AM_ABS(_a)       ((_a)>0?(_a):-(_a))

/**\brief 计算数值a与b差值的绝对值*/
#define AM_ABSSUB(a,b) ((a>=b)?(a-b):(b-a))

/**\brief 添加在命令行式宏定义的开头
 * 一些宏需要完成一系列语句，为了使这些语句形成一个整体不被打断，需要用
 * AM_MACRO_BEGIN和AM_MACRO_END将这些语句括起来。如:
 * \code
 * #define CHECK(_n)    \
 *    AM_MACRO_BEGIN \
 *    if ((_n)>0) printf(">0"); \
 *    else if ((n)<0) printf("<0"); \
 *    else printf("=0"); \
 *    AM_MACRO_END
 * \endcode
 */
#define AM_MACRO_BEGIN   do {

/**\brief 添加在命令行式定义的末尾*/
#define AM_MACRO_END     } while(0)

/**\brief 计算数组常数的元素数*/
#define AM_ARRAY_SIZE(_a)    (sizeof(_a)/sizeof((_a)[0]))

/**\brief 检查如果返回值是否错误，返回错误代码给调用函数*/
#define AM_TRY(_func) \
	AM_MACRO_BEGIN\
	AM_ErrorCode_t _ret;\
	if ((_ret=(_func))!=AM_SUCCESS)\
		return _ret;\
	AM_MACRO_END

/**\brief 检查返回值是否错误，如果错误，跳转到final标号。注意:函数中必须定义"AM_ErrorCode_t ret"和标号"final"*/
#define AM_TRY_FINAL(_func)\
	AM_MACRO_BEGIN\
	if ((ret=(_func))!=AM_SUCCESS)\
		goto final;\
	AM_MACRO_END

/**\brief 开始解析一个被指定字符隔开的字符串*/
#define AM_TOKEN_PARSE_BEGIN(_str, _delim, _token) \
	{\
	char *_strb =  strdup(_str);\
	if (_strb) {\
		_token = strtok(_strb, _delim);\
		while (_token != NULL) {
	
#define AM_TOKEN_PARSE_END(_str, _delim, _token) \
		_token = strtok(NULL, _delim);\
		}\
		free(_strb);\
	}\
	}
	

/**\brief 从一个被指定字符隔开的字符串中取指定位置的值，int类型，如未找到指定位置，则使用默认值_default代替*/
#define AM_TOKEN_VALUE_INT(_str, _delim, _index, _default) \
	({\
		char *token;\
		char *_strbak = strdup(_str);\
		int counter = 0;\
		int val = _default;\
		if (_strbak != NULL) {\
			AM_TOKEN_PARSE_BEGIN(_strbak, _delim, token)\
				if (counter == (_index)) {\
					val = atoi(token);\
					break;\
				}\
				counter++;\
			AM_TOKEN_PARSE_END(_strbak, _delim, token)\
			free(_strbak);\
		}\
		val;\
	})

/****************************************************************************
 * Type definitions
 ***************************************************************************/


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

