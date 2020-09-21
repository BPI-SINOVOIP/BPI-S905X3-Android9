/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#include <utils/Log.h>
#include <string.h>

#ifndef _C_TV_LOG_H_
#define _C_TV_LOG_H_


//1024 + 128
#define DEFAULT_LOG_BUFFER_LEN 1152

extern int __tv_log_print(int prio, const char *tag, const char *tv_tag, const char *fmt, ...);

#ifdef LOG_TV_TAG
#ifndef LOGD
#define LOGD(...) \
    __tv_log_print(ANDROID_LOG_DEBUG, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...) \
    __tv_log_print(ANDROID_LOG_ERROR, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(...) \
    __tv_log_print(ANDROID_LOG_WARN, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

#ifndef LOGI
#define LOGI(...) \
    __tv_log_print(ANDROID_LOG_INFO, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif


#if LOG_NDEBUG
#define LOGV(...) \
    do {if (0) { __tv_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, LOG_TV_TAG, __VA_ARGS__);} } while(0)
#else
#define LOGV(...) \
    __tv_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

#else
#define LOGD(...)    ALOGD(__VA_ARGS__)
#define LOGE(...)    ALOGE(__VA_ARGS__)
#define LOGV(...)    ALOGV(__VA_ARGS__)
#define LOGW(...)    ALOGW(__VA_ARGS__)
#endif//end #ifdef LOG_TV_TAG

#endif//end #ifndef _C_TV_LOG_H_
