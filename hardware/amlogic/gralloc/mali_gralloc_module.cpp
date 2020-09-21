/*
 * Copyright (C) 2016-2017 ARM Limited. All rights reserved.
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

#include <errno.h>
#include <pthread.h>
#include <inttypes.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#if PLATFORM_SDK_VERSION < 27
#include <system/window.h>
#endif
#include <hardware/hardware.h>
#include <hardware/fb.h>

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"
#include "mali_gralloc_bufferaccess.h"
#include "mali_gralloc_reference.h"

#if GRALLOC_USE_GRALLOC1_API == 1
#include "mali_gralloc_public_interface.h"
#else
#include "legacy/alloc_device.h"
#endif

static int mali_gralloc_module_device_open(const hw_module_t *module, const char *name, hw_device_t **device)
{
	int status = -EINVAL;

#if GRALLOC_USE_GRALLOC1_API == 1

	if (!strncmp(name, GRALLOC_HARDWARE_MODULE_ID, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = mali_gralloc_device_open(module, name, device);
	}

#else

	if (!strncmp(name, GRALLOC_HARDWARE_GPU0, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = alloc_device_open(module, name, device);
	}

#endif
	else if (!strncmp(name, GRALLOC_HARDWARE_FB0, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = framebuffer_device_open(module, name, device);
	}

	return status;
}

static int gralloc_register_buffer(gralloc_module_t const *module, buffer_handle_t handle)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_reference_retain(m, handle);
}

static int gralloc_unregister_buffer(gralloc_module_t const *module, buffer_handle_t handle)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_reference_release(m, handle, false);
}

static int gralloc_lock(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w, int h,
                        void **vaddr)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock(m, handle, usage, l, t, w, h, vaddr);
}

static int gralloc_lock_ycbcr(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w,
                              int h, android_ycbcr *ycbcr)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock_ycbcr(m, handle, usage, l, t, w, h, ycbcr);
}

static int gralloc_unlock(gralloc_module_t const *module, buffer_handle_t handle)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_unlock(m, handle);
}

static int gralloc_lock_async(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t, int w,
                              int h, void **vaddr, int32_t fence_fd)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock_async(m, handle, usage, l, t, w, h, vaddr, fence_fd);
}

static int gralloc_lock_ycbcr_async(gralloc_module_t const *module, buffer_handle_t handle, int usage, int l, int t,
                                    int w, int h, android_ycbcr *ycbcr, int32_t fence_fd)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_lock_ycbcr_async(m, handle, usage, l, t, w, h, ycbcr, fence_fd);
}

static int gralloc_unlock_async(gralloc_module_t const *module, buffer_handle_t handle, int32_t *fence_fd)
{
	const mali_gralloc_module *m = reinterpret_cast<const mali_gralloc_module *>(module);

	return mali_gralloc_unlock_async(m, handle, fence_fd);
}

// There is one global instance of the module

static struct hw_module_methods_t mali_gralloc_module_methods = { mali_gralloc_module_device_open };

private_module_t::private_module_t()
{
#define INIT_ZERO(obj) (memset(&(obj), 0, sizeof((obj))))
	base.common.tag = HARDWARE_MODULE_TAG;
#if GRALLOC_USE_GRALLOC1_API == 1
	base.common.version_major = GRALLOC_MODULE_API_VERSION_1_0;
#else
	base.common.version_major = GRALLOC_MODULE_API_VERSION_0_3;
#endif
	base.common.version_minor = 0;
	base.common.id = GRALLOC_HARDWARE_MODULE_ID;
	base.common.name = "Graphics Memory Allocator Module";
	base.common.author = "ARM Ltd.";
	base.common.methods = &mali_gralloc_module_methods;
	base.common.dso = NULL;
	INIT_ZERO(base.common.reserved);

#if GRALLOC_USE_GRALLOC1_API == 0
	base.registerBuffer = gralloc_register_buffer;
	base.unregisterBuffer = gralloc_unregister_buffer;
	base.lock = gralloc_lock;
	base.lock_ycbcr = gralloc_lock_ycbcr;
	base.unlock = gralloc_unlock;
	base.lockAsync = gralloc_lock_async;
	base.lockAsync_ycbcr = gralloc_lock_ycbcr_async;
	base.unlockAsync = gralloc_unlock_async;
	base.perform = NULL;
	INIT_ZERO(base.reserved_proc);
#endif

	framebuffer = NULL;
	flags = 0;
	numBuffers = 0;
	bufferMask = 0;
	pthread_mutex_init(&(lock), NULL);
	currentBuffer = NULL;
	INIT_ZERO(info);
	INIT_ZERO(finfo);
	xdpi = 0.0f;
	ydpi = 0.0f;
	fps = 0.0f;
	swapInterval = 1;
	ion_client = -1;

#undef INIT_ZERO
};

/*
 * HAL_MODULE_INFO_SYM will be initialized using the default constructor
 * implemented above
 */
struct private_module_t HAL_MODULE_INFO_SYM;
