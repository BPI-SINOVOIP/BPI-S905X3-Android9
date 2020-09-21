/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef DROIDLOGIC_LOG_CONTROL_H_
#define DROIDLOGIC_LOG_CONTROL_H_

#include <utils/Log.h>

/**
 * the highest level is 2. Less value of the level and less logs can be printed.
 * LOG_LEVEL_0 print nothing
 * LOG_LEVEL_1 print error logs and warning logs
 * LOG_LEVEL_2 print debug logs base on LOG_LEVEL_1
 */
enum {
    LOG_LEVEL_0 = 0, LOG_LEVEL_1 = 1, LOG_LEVEL_2 = 2,
};

extern int mLogLevel;

#ifdef LOG_CEE_TAG
#ifndef LOGD
#define LOGD(...) \
    do {\
        if (mLogLevel >= LOG_LEVEL_2) {\
            __unit_log_print(ANDROID_LOG_DEBUG, LOG_UNIT_TAG, LOG_CEE_TAG, __VA_ARGS__);\
        }\
    } while(0)
#endif

#ifndef LOGE
#define LOGE(...) \
    do {\
        if (mLogLevel >= LOG_LEVEL_1) {\
            __unit_log_print(ANDROID_LOG_ERROR, LOG_UNIT_TAG, LOG_CEE_TAG, __VA_ARGS__);\
        }\
    } while(0)
#endif

#ifndef LOGW
#define LOGW(...) \
    do {\
        if (mLogLevel >= LOG_LEVEL_1) {\
            __unit_log_print(ANDROID_LOG_WARN, LOG_UNIT_TAG, LOG_CEE_TAG, __VA_ARGS__);\
        }\
    } while(0)
#endif

#ifndef LOGI
#define LOGI(...) \
    do {\
        if (mLogLevel >= LOG_LEVEL_1) {\
            __unit_log_print(ANDROID_LOG_INFO, LOG_UNIT_TAG, LOG_CEE_TAG, __VA_ARGS__);\
        }\
    } while(0)
#endif

#if LOG_NDEBUG
#define LOGV(...) \
    do {if (0) { __unit_log_print(ANDROID_LOG_VERBOSE, LOG_UNIT_TAG, LOG_CEE_TAG, __VA_ARGS__);} } while(0)
#else
#define LOGV(...) \
    __unit_log_print(ANDROID_LOG_VERBOSE, LOG_UNIT_TAG, LOG_CEE_TAG, __VA_ARGS__)
#endif

#else
#define LOGD(...)    ALOGD(__VA_ARGS__)
#define LOGE(...)    ALOGE(__VA_ARGS__)
#define LOGV(...)    ALOGV(__VA_ARGS__)
#define LOGW(...)    ALOGW(__VA_ARGS__)
#define LOGI(...)    ALOGW(__VA_ARGS__)

#endif//end #ifdef LOG_CEE_TAG
#endif //DROIDLOGIC_LOG_CONTROL_H_
