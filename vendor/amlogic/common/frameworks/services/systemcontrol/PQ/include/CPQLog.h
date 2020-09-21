/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#include <utils/Log.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#ifndef _C_TV_LOG_H_
#define _C_TV_LOG_H_


//1024 + 128
#define DEFAULT_LOG_BUFFER_LEN 1152

extern int __pq_log_print(int prio, const char *tag, const char *pq_tag, const char *fmt, ...);

#ifdef LOG_TV_TAG
#ifndef SYS_LOGD
#define SYS_LOGD(...) \
    __pq_log_print(ANDROID_LOG_DEBUG, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

#ifndef SYS_LOGE
#define SYS_LOGE(...) \
    __pq_log_print(ANDROID_LOG_ERROR, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

#ifndef SYS_LOGV
#define SYS_LOGV(...) \
    __pq_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

#ifndef SYS_LOGI
#define SYS_LOGI(...) \
    __pq_log_print(ANDROID_LOG_INFO, LOG_TAG, LOG_TV_TAG, __VA_ARGS__)
#endif

//#else
//#define SYS_LOGD(...)    ALOGD(__VA_ARGS__)
//#define SYS_LOGE(...)    ALOGE(__VA_ARGS__)
//#define SYS_LOGV(...)    ALOGV(__VA_ARGS__)
//#define SYS_LOGI(...)    ALOGI(__VA_ARGS__)
#endif//end #ifdef LOG_TV_TAG

#endif//end #ifndef _C_TV_LOG_H_
