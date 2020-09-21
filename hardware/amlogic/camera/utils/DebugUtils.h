/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
use "dumpsys media.camera -t x" to change log level to x or
use "adb shell dumpsys media.camera -t x" to change log level to x
*/

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H
#include <stdint.h>

//Uncomment to enable more verbose/debug logs
#define DEBUG_LOG
extern volatile int32_t gCamHal_LogLevel;

///Camera HAL Logging Functions
#ifndef DEBUG_LOG

#define CAMHAL_LOGDA(str)
#define CAMHAL_LOGDB(str, ...)
#define CAMHAL_LOGVA(str)
#define CAMHAL_LOGVB(str, ...)

#define CAMHAL_LOGIA ALOGD
#define CAMHAL_LOGIB ALOGD
#define CAMHAL_LOGWA ALOGE
#define CAMHAL_LOGWB ALOGE
#define CAMHAL_LOGEA ALOGE
#define CAMHAL_LOGEB ALOGE
#define CAMHAL_LOGFA ALOGE
#define CAMHAL_LOGFB ALOGE

#undef LOG_FUNCTION_NAME
#undef LOG_FUNCTION_NAME_EXIT
#define LOG_FUNCTION_NAME
#define LOG_FUNCTION_NAME_EXIT

#else

///Defines for debug statements - Macro LOG_TAG needs to be defined in the respective files
#define CAMHAL_LOGVA(str)         ALOGV_IF(gCamHal_LogLevel >=6,"%5d %s - " str, __LINE__,__FUNCTION__);
#define CAMHAL_LOGVB(str,...)     ALOGV_IF(gCamHal_LogLevel >=6,"%5d %s - " str, __LINE__, __FUNCTION__, __VA_ARGS__);
#define CAMHAL_LOGDA(str)         ALOGD_IF(gCamHal_LogLevel >=5,"%5d %s - " str, __LINE__,__FUNCTION__);
#define CAMHAL_LOGDB(str, ...)    ALOGD_IF(gCamHal_LogLevel >=5,"%5d %s - " str, __LINE__, __FUNCTION__, __VA_ARGS__);
#define CAMHAL_LOGIA(str)         ALOGI_IF(gCamHal_LogLevel >=4,"%5d %s - " str, __LINE__, __FUNCTION__);
#define CAMHAL_LOGIB(str, ...)    ALOGI_IF(gCamHal_LogLevel >=4,"%5d %s - " str, __LINE__,__FUNCTION__, __VA_ARGS__);
#define CAMHAL_LOGWA(str)         ALOGW_IF(gCamHal_LogLevel >=3,"%5d %s - " str, __LINE__, __FUNCTION__);
#define CAMHAL_LOGWB(str, ...)    ALOGW_IF(gCamHal_LogLevel >=3,"%5d %s - " str, __LINE__,__FUNCTION__, __VA_ARGS__);
#define CAMHAL_LOGEA(str)         ALOGE_IF(gCamHal_LogLevel >=2,"%5d %s - " str, __LINE__, __FUNCTION__);
#define CAMHAL_LOGEB(str, ...)    ALOGE_IF(gCamHal_LogLevel >=2,"%5d %s - " str, __LINE__,__FUNCTION__, __VA_ARGS__);
#define CAMHAL_LOGFA(str)         ALOGF_IF(gCamHal_LogLevel >=1,"%5d %s - " str, __LINE__, __FUNCTION__);
#define CAMHAL_LOGFB(str, ...)    ALOGF_IF(gCamHal_LogLevel >=1,"%5d %s - " str, __LINE__,__FUNCTION__, __VA_ARGS__);

#define LOG_FUNCTION_NAME         CAMHAL_LOGVA("ENTER");
#define LOG_FUNCTION_NAME_EXIT    CAMHAL_LOGVA("EXIT");
#define DBG_LOGA(str)             ALOGI_IF(gCamHal_LogLevel >=4,"%10s-%5d %s - " str, CAMHAL_BUILD_NAME, __LINE__,__FUNCTION__)
#define DBG_LOGB(str, ...)        ALOGI_IF(gCamHal_LogLevel >=4,"%10s-%5d %s - " str, CAMHAL_BUILD_NAME, __LINE__,__FUNCTION__, __VA_ARGS__);

#endif

#endif //DEBUG_UTILS_H
