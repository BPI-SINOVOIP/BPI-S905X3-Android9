/*
 * Copyright (C) 2017 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <linux/fb.h>
#include <linux/ion.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cutils/native_handle.h>
#include <utils/Log.h>

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif
/**
 * mali_gralloc_formats.h needs the define for GRALLOC_MODULE_API_VERSION_0_3 and
 * GRALLOC_MODULE_API_VERSION_1_0, so include <gralloc1.h> or <gralloc.h> before
 * including mali_gralloc_formats.h
 **/
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"
#include "gralloc_helper.h"

#if defined(GRALLOC_MODULE_API_VERSION_0_3) || \
    (defined(GRALLOC_MODULE_API_VERSION_1_0) && !defined(GRALLOC_DISABLE_PRIVATE_BUFFER_DEF))

/*
 * This header file contains the private buffer definition. For gralloc 0.3 it will
 * always be exposed, but for gralloc 1.0 it will be removed at some point in the future.
 *
 * GRALLOC_DISABLE_PRIVATE_BUFFER_DEF is intended for DDKs to test while implementing
 * the new private API.
 */
#include "mali_gralloc_buffer.h"
#endif

#if defined(GRALLOC_MODULE_API_VERSION_1_0)

/* gralloc 1.0 supports the new private interface that abstracts
 * the private buffer definition to a set of defined APIs.
 */
#include "mali_gralloc_private_interface.h"
#endif
#ifndef HAL_PIXEL_FORMAT_YCbCr_420_SP
#define HAL_PIXEL_FORMAT_YCbCr_420_SP 0x100
#endif

#endif /* GRALLOC_PRIV_H_ */
