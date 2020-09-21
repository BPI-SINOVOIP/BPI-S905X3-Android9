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

#include <hardware/hardware.h>
#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_private_interface.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_helper.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_bufferdescriptor.h"

#define CHECK_FUNCTION(A, B, C)                    \
	do                                             \
	{                                              \
		if (A == B)                                \
			return (gralloc1_function_pointer_t)C; \
	} while (0)

static int32_t mali_gralloc_private_get_buff_int_fmt(gralloc1_device_t *device, buffer_handle_t handle,
                                                     uint64_t *internal_format)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || internal_format == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*internal_format = hnd->internal_format;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_fd(gralloc1_device_t *device, buffer_handle_t handle, int *fd)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || fd == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*fd = hnd->share_fd;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_int_dims(gralloc1_device_t *device, buffer_handle_t handle,
                                                      int *internalWidth, int *internalHeight)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || internalWidth == NULL || internalHeight == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*internalWidth = hnd->internalWidth;
	*internalHeight = hnd->internalHeight;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_offset(gralloc1_device_t *device, buffer_handle_t handle, int64_t *offset)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || offset == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*offset = hnd->offset;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_bytestride(gralloc1_device_t *device, buffer_handle_t handle,
                                                        int *bytestride)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || bytestride == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*bytestride = hnd->byte_stride;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_yuvinfo(gralloc1_device_t *device, buffer_handle_t handle,
                                                     mali_gralloc_yuv_info *yuvinfo)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || yuvinfo == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*yuvinfo = hnd->yuv_info;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_size(gralloc1_device_t *device, buffer_handle_t handle, int *size)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || size == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*size = hnd->size;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_flags(gralloc1_device_t *device, buffer_handle_t handle, int *flags)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || flags == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*flags = hnd->flags;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_buff_min_page_size(gralloc1_device_t *device, buffer_handle_t handle,
                                                           int *min_pgsz)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || min_pgsz == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *hnd = static_cast<const private_handle_t *>(handle);
	*min_pgsz = hnd->min_pgsz;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_get_attr_param(gralloc1_device_t *device, buffer_handle_t handle, buf_attr attr,
                                                   int32_t *val, int32_t last_call)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || val == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *const_hnd = static_cast<const private_handle_t *>(handle);
	private_handle_t *hnd = const_cast<private_handle_t *>(const_hnd);

	if (hnd->attr_base == MAP_FAILED)
	{
		if (gralloc_buffer_attr_map(hnd, 1) < 0)
		{
			return GRALLOC1_ERROR_BAD_HANDLE;
		}
	}

	if (gralloc_buffer_attr_read(hnd, attr, val) < 0)
	{
		gralloc_buffer_attr_unmap(hnd);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (last_call)
	{
		gralloc_buffer_attr_unmap(hnd);
	}

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_set_attr_param(gralloc1_device_t *device, buffer_handle_t handle, buf_attr attr,
                                                   int32_t *val, int32_t last_call)
{
	GRALLOC_UNUSED(device);

	if (private_handle_t::validate(handle) < 0 || val == NULL)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	const private_handle_t *const_hnd = static_cast<const private_handle_t *>(handle);
	private_handle_t *hnd = const_cast<private_handle_t *>(const_hnd);

	if (hnd->attr_base == MAP_FAILED)
	{
		if (gralloc_buffer_attr_map(hnd, 1) < 0)
		{
			return GRALLOC1_ERROR_BAD_HANDLE;
		}
	}

	if (gralloc_buffer_attr_write(hnd, attr, val) < 0)
	{
		gralloc_buffer_attr_unmap(hnd);
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (last_call)
	{
		gralloc_buffer_attr_unmap(hnd);
	}

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_private_set_priv_fmt(gralloc1_device_t *device, gralloc1_buffer_descriptor_t desc,
                                                 uint64_t internal_format)
{
	GRALLOC_UNUSED(device);

	buffer_descriptor_t *priv_desc = reinterpret_cast<buffer_descriptor_t *>(desc);

	if (priv_desc == NULL)
	{
		return GRALLOC1_ERROR_BAD_DESCRIPTOR;
	}

	priv_desc->hal_format = internal_format;
	priv_desc->format_type = MALI_GRALLOC_FORMAT_TYPE_INTERNAL;

	return GRALLOC1_ERROR_NONE;
}

gralloc1_function_pointer_t mali_gralloc_private_interface_getFunction(int32_t descriptor)
{
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_INT_FMT, mali_gralloc_private_get_buff_int_fmt);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_FD, mali_gralloc_private_get_buff_fd);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_INTERNAL_DIMS, mali_gralloc_private_get_buff_int_dims);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_OFFSET, mali_gralloc_private_get_buff_offset);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_BYTESTRIDE, mali_gralloc_private_get_buff_bytestride);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_YUVINFO, mali_gralloc_private_get_buff_yuvinfo);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_SIZE, mali_gralloc_private_get_buff_size);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_FLAGS, mali_gralloc_private_get_buff_flags);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_BUFF_MIN_PAGESIZE,
	               mali_gralloc_private_get_buff_min_page_size);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_GET_ATTR_PARAM, mali_gralloc_private_get_attr_param);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_SET_ATTR_PARAM, mali_gralloc_private_set_attr_param);
	CHECK_FUNCTION(descriptor, MALI_GRALLOC1_FUNCTION_SET_PRIV_FMT, mali_gralloc_private_set_priv_fmt);

	return NULL;
}
