/*
 * Copyright (C) 2017 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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

#ifndef MALI_GRALLOC_PRIVATE_INTERFACE_H_
#define MALI_GRALLOC_PRIVATE_INTERFACE_H_

#include "mali_gralloc_private_interface_types.h"

typedef enum
{

	MALI_GRALLOC1_LAST_PUBLIC_FUNCTION = GRALLOC1_LAST_FUNCTION,

	MALI_GRALLOC1_FUNCTION_GET_BUFF_INT_FMT = MALI_GRALLOC1_LAST_PUBLIC_FUNCTION + 100,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_FD,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_INTERNAL_DIMS,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_OFFSET,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_BYTESTRIDE,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_YUVINFO,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_SIZE,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_FLAGS,
	MALI_GRALLOC1_FUNCTION_GET_BUFF_MIN_PAGESIZE,
	MALI_GRALLOC1_FUNCTION_SET_PRIV_FMT,

	/* API related to the shared attribute region */
	MALI_GRALLOC1_FUNCTION_GET_ATTR_PARAM,
	MALI_GRALLOC1_FUNCTION_SET_ATTR_PARAM,

	MALI_GRALLOC1_LAST_PRIVATE_FUNCTION
} mali_gralloc1_function_descriptor_t;

typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_INT_FMT)(gralloc1_device_t *device, buffer_handle_t handle,
                                                         uint64_t *internal_format);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_FD)(gralloc1_device_t *device, buffer_handle_t handle, int *fd);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_INTERNAL_DIMS)(gralloc1_device_t *device, buffer_handle_t handle,
                                                               int *internalWidth, int *internalHeight);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_OFFSET)(gralloc1_device_t *device, buffer_handle_t handle,
                                                        int64_t *offset);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_BYTESTRIDE)(gralloc1_device_t *device, buffer_handle_t handle,
                                                            int *bytestride);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_YUVINFO)(gralloc1_device_t *device, buffer_handle_t handle,
                                                         mali_gralloc_yuv_info *yuvinfo);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_SIZE)(gralloc1_device_t *device, buffer_handle_t handle, int *size);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_FLAGS)(gralloc1_device_t *device, buffer_handle_t handle, int *flags);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_BUFF_MIN_PAGESIZE)(gralloc1_device_t *device, buffer_handle_t handle,
                                                              int *min_pgsz);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_GET_ATTR_PARAM)(gralloc1_device_t *device, buffer_handle_t handle, buf_attr attr,
                                                       int32_t *val, int32_t last_call);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_SET_ATTR_PARAM)(gralloc1_device_t *device, buffer_handle_t handle, buf_attr attr,
                                                       int32_t *val, int32_t last_call);
typedef int32_t (*GRALLOC1_PFN_PRIVATE_SET_PRIV_FMT)(gralloc1_device_t *device, gralloc1_buffer_descriptor_t desc,
                                                     uint64_t internal_format);

#if defined(GRALLOC_LIBRARY_BUILD)
gralloc1_function_pointer_t mali_gralloc_private_interface_getFunction(int32_t descriptor);
#endif

#endif /* MALI_GRALLOC_PRIVATE_INTERFACE_H_ */
