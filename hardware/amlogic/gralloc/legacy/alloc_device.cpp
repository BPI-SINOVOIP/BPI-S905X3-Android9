/*
 * Copyright (C) 2010-2017 ARM Limited. All rights reserved.
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

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/ioctl.h>

#include "mali_gralloc_module.h"
#include "alloc_device.h"
#include "gralloc_priv.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"
#include "mali_gralloc_ion.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"

static int alloc_device_alloc(alloc_device_t *dev, int w, int h, int format, int usage, buffer_handle_t *pHandle,
                              int *pStride)
{
	mali_gralloc_module *m;
	int err = -EINVAL;

	if (!dev || !pHandle || !pStride)
	{
		return err;
	}

	m = reinterpret_cast<private_module_t *>(dev->common.module);

#if GRALLOC_FB_SWAP_RED_BLUE == 1

	/* match the framebuffer format */
	if (usage & GRALLOC_USAGE_HW_FB)
	{
#ifdef GRALLOC_16_BITS
		format = HAL_PIXEL_FORMAT_RGB_565;
#else
		format = HAL_PIXEL_FORMAT_BGRA_8888;
#endif
	}

#endif

#if DISABLE_FRAMEBUFFER_HAL != 1

	if (usage & GRALLOC_USAGE_HW_FB)
	{
		int byte_stride;
		int pixel_stride;

		err = fb_alloc_framebuffer(m, usage, usage, pHandle, &pixel_stride, &byte_stride);

		if (err >= 0)
		{
			private_handle_t *hnd = (private_handle_t *)*pHandle;

			/* Allocate a meta-data buffer for framebuffer too. fbhal
			 * ones wont need it but it will lead to binder IPC fail
			 * without a valid share_attr_fd.
			 *
			 * Explicitly ignore allocation errors since it is not critical to have
			 */
			(void)gralloc_buffer_attr_allocate(hnd);

			hnd->req_format = format;
			hnd->yuv_info = MALI_YUV_BT601_NARROW;
			hnd->internal_format = format;
			hnd->byte_stride = byte_stride;
			hnd->width = w;
			hnd->height = h;
			hnd->stride = pixel_stride;
			hnd->internalWidth = w;
			hnd->internalHeight = h;
		}
	}
	else
#endif
	{
		/* share the same allocation interface with gralloc1.*/
		buffer_descriptor_t buffer_descriptor;
		gralloc_buffer_descriptor_t gralloc_buffer_descriptor[1];

		memset((void*)&buffer_descriptor, 0, sizeof(buffer_descriptor));
		buffer_descriptor.hal_format = format;
		buffer_descriptor.consumer_usage = usage;
		buffer_descriptor.producer_usage = usage;
		buffer_descriptor.width = w;
		buffer_descriptor.height = h;
		buffer_descriptor.layer_count = 1;
		buffer_descriptor.format_type = MALI_GRALLOC_FORMAT_TYPE_USAGE;
		gralloc_buffer_descriptor[0] = (gralloc_buffer_descriptor_t)(&buffer_descriptor);

		if (mali_gralloc_buffer_allocate(m, gralloc_buffer_descriptor, 1, pHandle, NULL) < 0)
		{
			ALOGE("Failed to allocate buffer.");
			err = -ENOMEM;
		}
		else
		{
			mali_gralloc_query_getstride(*pHandle, pStride);
			err = 0;
		}
	}

	return err;
}

static int alloc_device_free(alloc_device_t *dev, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		return -EINVAL;
	}

	private_handle_t const *hnd = reinterpret_cast<private_handle_t const *>(handle);
	private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		// free this buffer
		close(hnd->fd);
	}
	else
	{
		mali_gralloc_buffer_free(handle);
	}

	delete hnd;

	return 0;
}

int alloc_device_open(hw_module_t const *module, const char *name, hw_device_t **device)
{
	alloc_device_t *dev;

	GRALLOC_UNUSED(name);

	dev = new alloc_device_t;

	if (NULL == dev)
	{
		return -1;
	}

	/* initialize our state here */
	memset(dev, 0, sizeof(*dev));

	/* initialize the procs */
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = const_cast<hw_module_t *>(module);
	dev->common.close = mali_gralloc_ion_device_close;
	dev->alloc = alloc_device_alloc;
	dev->free = alloc_device_free;

	*device = &dev->common;

	return 0;
}
