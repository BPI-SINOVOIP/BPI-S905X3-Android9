/*
 * Copyright (C) 2016 ARM Limited. All rights reserved.
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

#ifndef MALI_GRALLOC_DEBUG_H_
#define MALI_GRALLOC_DEBUG_H_

#include <utils/String8.h>
#include <hardware/hardware.h>
#include "gralloc_priv.h"
#include "mali_gralloc_module.h"

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif

void mali_gralloc_dump_buffer_add(private_handle_t *handle);
void mali_gralloc_dump_buffer_erase(private_handle_t *handle);

void mali_gralloc_dump_string(android::String8 &buf, const char *fmt, ...);
void mali_gralloc_dump_buffers(android::String8 &dumpBuffer, uint32_t *outSize);
void mali_gralloc_dump_internal(uint32_t *outSize, char *outBuffer);
#endif
