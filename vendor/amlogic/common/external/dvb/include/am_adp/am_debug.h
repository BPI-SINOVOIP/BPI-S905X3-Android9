/***************************************************************************
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description:
 */
/**\file
 * \brief DEBUG 信息输出设置
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#ifndef _AM_DEBUG_H
#define _AM_DEBUG_H

#include "am_util.h"
#include <stdio.h>
#include "am_misc.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#ifndef AM_DEBUG_LEVEL
#define AM_DEBUG_LEVEL 500
#endif

#define AM_DEBUG_LOGLEVEL_DEFAULT 1

#define AM_DEBUG_WITH_FILE_LINE
#ifdef AM_DEBUG_WITH_FILE_LINE
#define AM_DEBUG_FILE_LINE fprintf(stderr, "(\"%s\" %d)", __FILE__, __LINE__);
#else
#define AM_DEBUG_FILE_LINE
#endif

/**\brief 输出调试信息
 *如果_level小于等于文件中定义的宏AM_DEBUG_LEVEL,则输出调试信息。
 */
#ifndef ANDROID
#define AM_DEBUG(_level,_fmt...) \
	AM_MACRO_BEGIN\
	if ((_level)<=(AM_DEBUG_LEVEL))\
	{\
		fprintf(stderr, "AM_DEBUG:");\
		AM_DEBUG_FILE_LINE\
		fprintf(stderr, _fmt);\
		fprintf(stderr, "\n");\
	}\
	AM_MACRO_END
#else
#include <android/log.h>
#ifndef TAG_EXT
#define TAG_EXT
#endif

#define log_print(...) __android_log_print(ANDROID_LOG_INFO, "AM_DEBUG" TAG_EXT, __VA_ARGS__)
#define AM_DEBUG(_level,_fmt...) \
	AM_MACRO_BEGIN\
	if ((_level)<=(AM_DebugGetLogLevel()))\
	{\
		log_print(_fmt);\
	}\
	AM_MACRO_END
#endif
/****************************************************************************
 * Error code definitions
 ****************************************************************************/

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

