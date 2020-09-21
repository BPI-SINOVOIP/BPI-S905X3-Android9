/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef _INIT_LOG_H_
#define _INIT_LOG_H_

#ifdef USE_KERNEL_LOG
#include <cutils/klog.h>
#define ERROR(x...)   KLOG_ERROR("dig", x)
#define NOTICE(x...)  KLOG_NOTICE("dig", x)
#define INFO(x...)    KLOG_INFO("dig", x)
#else
#define ERROR(x...)
#define NOTICE(x...)
#define INFO(x...)
#endif

#define LOG_UEVENTS        0  /* log uevent messages if 1. verbose */

#endif
