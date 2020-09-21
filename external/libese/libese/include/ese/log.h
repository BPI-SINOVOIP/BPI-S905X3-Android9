/*
 * Copyright (C) 2017 The Android Open Source Project
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
 *
 * Android logging wrapper to make it easy to interchange based on environment.
 */

#ifndef ESE_LOG_H_
#define ESE_LOG_H_ 1

/* Uncomment when doing bring up or other messy debugging.
 * #define LOG_NDEBUG 0
 */

#if defined(ESE_LOG_NONE) || defined(ESE_LOG_STDIO)
#  define ESE_LOG_ANDROID 0
#endif

#if !defined(ESE_LOG_ANDROID)
#define ESE_LOG_ANDROID 1
#endif

#if !defined(LOG_TAG)
#  define LOG_TAG "libese"
#endif

#if ESE_LOG_ANDROID == 1

#include <log/log.h>

#else  /* ESE_LOG_ANDROID */

#ifndef ALOGV
#define __ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#if LOG_NDEBUG
#define ALOGV(...) do { if (0) { __ALOGV(__VA_ARGS__); } } while (0)
#else
#define ALOGV(...) __ALOGV(__VA_ARGS__)
#endif
#endif

#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#if defined(ESE_LOG_STDIO)
#include <stdio.h>
#include <stdlib.h>
#define ALOG(priority, tag, format, ...) \
  fprintf(stderr, "[%s: %s] " format "\n", #priority, tag, ##__VA_ARGS__)
#define LOG_ALWAYS_FATAL(format, ...) { \
  ALOG(LOG_ERROR, LOG_TAG, format, ##__VA_ARGS__);  \
  abort(); \
}

#elif defined(ESE_LOG_NONE)
  #define ALOG(...) {}
  #define LOG_ALWAYS_FATAL(...) while (1);
#endif

#endif  /* !ESE_LOG_ANDROID */

#endif  /* ESE_LOG_H_ */
