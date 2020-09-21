/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef CHRE_PLATFORM_SLPI_LOG_H_
#define CHRE_PLATFORM_SLPI_LOG_H_

#include "ash/debug.h"
#include "chre/util/toolchain.h"

#ifndef __FILENAME__
#define __FILENAME__ CHRE_FILENAME
#endif

// TODO: Replace ashLog with a builtin logging destination that does not wake
// the AP unless necessary.
#define CHRE_SLPI_LOG(level, fmt, ...) \
    do { \
      CHRE_LOG_PREAMBLE \
      ashLog(ASH_SOURCE_CHRE, level, fmt, ##__VA_ARGS__); \
      CHRE_LOG_EPILOGUE \
    } while (0)

#define LOGE(fmt, ...) CHRE_SLPI_LOG(ASH_LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) CHRE_SLPI_LOG(ASH_LOG_WARN, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) CHRE_SLPI_LOG(ASH_LOG_INFO, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) CHRE_SLPI_LOG(ASH_LOG_DEBUG, fmt, ##__VA_ARGS__)

#endif  // CHRE_PLATFORM_SLPI_LOG_H_
