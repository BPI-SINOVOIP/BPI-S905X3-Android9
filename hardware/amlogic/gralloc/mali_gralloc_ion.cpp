/*
 * Copyright (C) 2016-2017 ARM Limited. All rights reserved.
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
#include <inttypes.h>
#include <pthread.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <linux/ion.h>
#include <ion/ion.h>
#include <sys/ioctl.h>

#include <hardware/hardware.h>

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif

#ifdef GRALLOC_AML_EXTEND
#include "amlogic/am_gralloc_internal.h"
#else
#include "gralloc_usage_ext.h"
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"
#include "mali_gralloc_bufferdescriptor.h"

#include <map>

static std::map<int, ion_user_handle_t> imported_user_hnd;
static std::map<int, int> imported_ion_client;

#ifdef GRALLOC_AML_EXTEND
unsigned int am_pick_ion_heap(
	buffer_descriptor_t *bufDescriptor, uint64_t usage);
void am_set_ion_flags(unsigned int heap_type, uint64_t usage,
	unsigned int *priv_heap_flag, int *ion_flags);
#endif

static void mali_gralloc_ion_free_internal(buffer_handle_t *pHandle, uint32_t num_hnds);

static void init_afbc(uint8_t *buf, uint64_t internal_format, int w, int h)
{
	uint32_t n_headers = (w * h) / 64;
	uint32_t body_offset = n_headers * 16;
	uint32_t headers[][4] = {
		{ body_offset, 0x1, 0x0, 0x0 }, /* Layouts 0, 3, 4 */
		{ (body_offset + (1 << 28)), 0x200040, 0x4000, 0x80 } /* Layouts 1, 5 */
	};
	uint32_t i, layout;

	/* map format if necessary (also removes internal extension bits) */
	uint64_t base_format = internal_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	switch (base_format)
	{
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_8888:
		/* Note that AFBC isn't supported for RGBX_8888 but the display drivers will treat it as RGBA8888
		 * as a workaround for not supporting partial pre-fetch. However this format will be less
		 * efficient for GPU. */
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBX_8888:
	case MALI_GRALLOC_FORMAT_INTERNAL_RGB_888:
	case MALI_GRALLOC_FORMAT_INTERNAL_RGB_565:
	case MALI_GRALLOC_FORMAT_INTERNAL_BGRA_8888:
		layout = 0;
		break;

	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
	case HAL_PIXEL_FORMAT_YCbCr_420_888:
	case MALI_GRALLOC_FORMAT_INTERNAL_YV12:
	case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
	case MALI_GRALLOC_FORMAT_INTERNAL_NV21:
		layout = 1;
		break;

	default:
		layout = 0;
	}

	ALOGV("Writing AFBC header layout %d for format %" PRIu64, layout, base_format);

	for (i = 0; i < n_headers; i++)
	{
		memcpy(buf, headers[layout], sizeof(headers[layout]));
		buf += sizeof(headers[layout]);
	}
}

static int alloc_from_ion_heap(int ion_fd, size_t size, unsigned int *type, unsigned int flags, int *min_pgsz)
{
	int shared_fd, ret;
	unsigned int heap_type = *type;

	if ((ion_fd < 0) || (size <= 0) || (min_pgsz == NULL))
	{
		return -1;
	}

	/**
	 * step 1: ion_alloc new ion_hnd
	 * step 2: ion_share from ion_hnd and get shared_fd
	 * step 3: ion free the given ion_hnd
	 * step 4: when we need to free this ion buffer, just close the shared_fd,
	 *            kernel will count the reference of file struct, so it's safe to
	 *            be transfered between processes.
	 */
	ret = ion_alloc_fd(ion_fd, size, 0, 1<<heap_type, flags, &shared_fd);

	if (ret < 0)
	{
		if (heap_type == ION_HEAP_TYPE_CUSTOM) {
			ALOGE("ION_HEAP_TYPE_CUSTOM alloc failed!\n");
			return -1;
		}
#if defined(ION_HEAP_SECURE_MASK)
		if (heap_type == ION_HEAP_SECURE)
		{
			return -1;
		}
		else
#endif
		{
			/* If everything else failed try system heap */
			flags = 0; /* Fallback option flags are not longer valid */
			heap_type = ION_HEAP_TYPE_SYSTEM;
			ret = ion_alloc_fd(ion_fd, size, 0, 1<<heap_type, flags, &shared_fd);
		}
	}

	if (ret >= 0)
	{
		switch (heap_type)
		{
		case ION_HEAP_TYPE_SYSTEM:
			*min_pgsz = SZ_4K;
			break;

		case ION_HEAP_TYPE_SYSTEM_CONTIG:
		case ION_HEAP_TYPE_CARVEOUT:
		case ION_HEAP_TYPE_DMA:
			*min_pgsz = size;
			break;
#ifdef ION_HEAP_CHUNK_MASK

		/* NOTE: if have this heap make sure your ION chunk size is 2M*/
		case ION_HEAP_CHUNK_MASK:
			*min_pgsz = SZ_2M;
			break;
#endif
#ifdef ION_HEAP_COMPOUND_PAGE_MASK

		case ION_HEAP_COMPOUND_PAGE_MASK:
			*min_pgsz = SZ_2M;
			break;
#endif
/* If have customized heap please set the suitable pg type according to
		 * the customized ION implementation
		 */
#ifdef ION_HEAP_CUSTOM_MASK

		case ION_HEAP_CUSTOM_MASK:
			*min_pgsz = SZ_4K;
			break;
#endif

		default:
			*min_pgsz = SZ_4K;
			break;
		}
	}
	*type = heap_type;

	return shared_fd;
}

unsigned int pick_ion_heap(uint64_t usage)
{
	unsigned int heap_type;

	if (usage & GRALLOC_USAGE_PROTECTED)
	{
#if defined(ION_HEAP_SECURE_MASK)
		heap_type = ION_HEAP_SECURE_MASK;
#else
		AERR("Protected ION memory is not supported on this platform.");
		return 0;
#endif
	}
#if GRALLOC_USE_ION_COMPOUND_PAGE_HEAP
	else if (!(usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) && (usage & (GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_COMPOSER)))
	{
		heap_type = ION_HEAP_TYPE_COMPOUND_PAGE;
	}
#elif GRALLOC_USE_ION_DMA_HEAP
	else if (!(usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) && (usage & (GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_COMPOSER)))
	{
		heap_type = ION_HEAP_TYPE_DMA;
	}

#endif
	else
	{
		heap_type = ION_HEAP_TYPE_SYSTEM;
	}

	return heap_type;
}

#ifndef GRALLOC_AML_EXTEND
void set_ion_flags(unsigned int heap_type, uint64_t usage, unsigned int *priv_heap_flag, int *ion_flags)
{
#if !GRALLOC_USE_ION_DMA_HEAP
	GRALLOC_UNUSED(heap_type);
#endif

	if (priv_heap_flag)
	{
#if GRALLOC_USE_ION_DMA_HEAP
		if (heap_type == ION_HEAP_TYPE_DMA)
		{
			*priv_heap_flag = private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP;
		}
#endif

		if (heap_type == ION_HEAP_TYPE_SYSTEM)
		{
			*priv_heap_flag &= ~private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP;
		}

		if (usage & GRALLOC_USAGE_AML_SECURE || usage & GRALLOC_USAGE_PROTECTED)
		{
			*priv_heap_flag |= private_handle_t::PRIV_FLAGS_SECURE_PROTECTED;
		}
	}

	if (ion_flags)
	{
#if GRALLOC_USE_ION_DMA_HEAP
		if ((heap_type != ION_HEAP_TYPE_DMA))
		{
#endif
			if ((usage & (GRALLOC_USAGE_SW_WRITE_MASK | GRALLOC_USAGE_SW_READ_MASK))
				|| (usage == GRALLOC_USAGE_HW_TEXTURE))
			{
				*ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
			}
#if GRALLOC_USE_ION_DMA_HEAP
		}
#endif
	}
}
#endif

static bool check_buffers_sharable(const gralloc_buffer_descriptor_t *descriptors, uint32_t numDescriptors)
{
	unsigned int shared_backend_heap_mask = 0;
	int shared_ion_flags = 0;
	uint64_t usage;
	uint32_t i;

	if (numDescriptors <= 1)
	{
		return false;
	}

	for (i = 0; i < numDescriptors; i++)
	{
		unsigned int heap_type;
		int ion_flags;
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)descriptors[i];

		usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

#ifdef GRALLOC_AML_EXTEND
		heap_type = am_pick_ion_heap(bufDescriptor, usage);
#else
		heap_type = pick_ion_heap(usage);
#endif
		if (0 == heap_type)
		{
			return false;
		}

#ifdef GRALLOC_AML_EXTEND
		am_set_ion_flags(heap_type, usage, NULL, &ion_flags);
#else
		set_ion_flags(heap_type, usage, NULL, &ion_flags);
#endif
		if (0 != shared_backend_heap_mask)
		{
			if (shared_backend_heap_mask != (1<<heap_type) || shared_ion_flags != ion_flags)
			{
				return false;
			}
		}
		else
		{
			shared_backend_heap_mask = (1<<heap_type);
			shared_ion_flags = ion_flags;
		}
	}

	return true;
}

static int get_max_buffer_descriptor_index(const gralloc_buffer_descriptor_t *descriptors, uint32_t numDescriptors)
{
	uint32_t i, max_buffer_index = 0;
	size_t max_buffer_size = 0;

	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)descriptors[i];

		if (max_buffer_size < bufDescriptor->size)
		{
			max_buffer_index = i;
			max_buffer_size = bufDescriptor->size;
		}
	}

	return max_buffer_index;
}

int mali_gralloc_ion_allocate(mali_gralloc_module *m, const gralloc_buffer_descriptor_t *descriptors,
                              uint32_t numDescriptors, buffer_handle_t *pHandle, bool *shared_backend)
{
	static int support_protected = 1; /* initially, assume we support protected memory */
	unsigned int heap_type, priv_heap_flag = 0;
	unsigned char *cpu_ptr = NULL;
	uint64_t usage;
	uint32_t i, max_buffer_index = 0;
	int shared_fd, ret, ion_flags = 0;
	int min_pgsz = 0;

	if (m->ion_client < 0)
	{
		m->ion_client = ion_open();

		if (m->ion_client < 0)
		{
			AERR("ion_open failed with %s", strerror(errno));
			return -1;
		}
	}

	*shared_backend = check_buffers_sharable(descriptors, numDescriptors);

	if (*shared_backend)
	{
		buffer_descriptor_t *max_bufDescriptor;

		max_buffer_index = get_max_buffer_descriptor_index(descriptors, numDescriptors);
		max_bufDescriptor = (buffer_descriptor_t *)(descriptors[max_buffer_index]);
		usage = max_bufDescriptor->consumer_usage | max_bufDescriptor->producer_usage;

#ifdef GRALLOC_AML_EXTEND
		heap_type = am_pick_ion_heap(max_bufDescriptor, usage);
#else
		heap_type = pick_ion_heap(usage);
#endif

#ifdef GRALLOC_AML_EXTEND
		am_set_ion_flags(heap_type, usage, &priv_heap_flag, &ion_flags);
#else
		set_ion_flags(heap_type, usage, &priv_heap_flag, &ion_flags);
#endif

		shared_fd = alloc_from_ion_heap(m->ion_client, max_bufDescriptor->size, &heap_type, ion_flags, &min_pgsz);

#ifdef GRALLOC_AML_EXTEND
		am_set_ion_flags(heap_type, usage, &priv_heap_flag, NULL);
#else
		set_ion_flags(heap_type, usage, &priv_heap_flag, NULL);
#endif

		if (shared_fd < 0)
		{
			AERR("ion_alloc failed form client: ( %d )", m->ion_client);
			return -1;
		}

		for (i = 0; i < numDescriptors; i++)
		{
			buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
			int tmp_fd;

			if (i != max_buffer_index)
			{
				tmp_fd = dup(shared_fd);

				if (tmp_fd < 0)
				{
					/* need to free already allocated memory. */
					mali_gralloc_ion_free_internal(pHandle, numDescriptors);
					return -1;
				}
			}
			else
			{
				tmp_fd = shared_fd;
			}
			private_handle_t *hnd = new private_handle_t(
			    private_handle_t::PRIV_FLAGS_USES_ION | priv_heap_flag, bufDescriptor->size, min_pgsz,
			    bufDescriptor->consumer_usage, bufDescriptor->producer_usage, tmp_fd, bufDescriptor->hal_format,
			    bufDescriptor->internal_format, bufDescriptor->byte_stride, bufDescriptor->width, bufDescriptor->height,
			    bufDescriptor->pixel_stride, bufDescriptor->internalWidth, bufDescriptor->internalHeight,
			    max_bufDescriptor->size, bufDescriptor->layer_count);

			if (NULL == hnd)
			{
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}

			pHandle[i] = hnd;
		}
	}
	else
	{
		for (i = 0; i < numDescriptors; i++)
		{
			buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
			usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

#ifdef GRALLOC_AML_EXTEND
			heap_type = am_pick_ion_heap(bufDescriptor, usage);
#else
			heap_type = pick_ion_heap(usage);
#endif

#ifdef GRALLOC_AML_EXTEND
			am_set_ion_flags(heap_type, usage, &priv_heap_flag, &ion_flags);
#else
			set_ion_flags(heap_type, usage, &priv_heap_flag, &ion_flags);
#endif

			shared_fd = alloc_from_ion_heap(m->ion_client, bufDescriptor->size, &heap_type, ion_flags, &min_pgsz);

#ifdef GRALLOC_AML_EXTEND
			am_set_ion_flags(heap_type, usage, &priv_heap_flag, NULL);
#else
			set_ion_flags(heap_type, usage, &priv_heap_flag, NULL);
#endif
			if (shared_fd < 0)
			{
				AERR("ion_alloc failed from client ( %d )", m->ion_client);

				/* need to free already allocated memory. not just this one */
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);

				return -1;
			}

			private_handle_t *hnd = new private_handle_t(
			    private_handle_t::PRIV_FLAGS_USES_ION | priv_heap_flag, bufDescriptor->size, min_pgsz,
			    bufDescriptor->consumer_usage, bufDescriptor->producer_usage, shared_fd, bufDescriptor->hal_format,
			    bufDescriptor->internal_format, bufDescriptor->byte_stride, bufDescriptor->width, bufDescriptor->height,
			    bufDescriptor->pixel_stride, bufDescriptor->internalWidth, bufDescriptor->internalHeight,
			    bufDescriptor->size, bufDescriptor->layer_count);

			if (NULL == hnd)
			{
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}
			if (usage & GRALLOC_USAGE_HW_FB) {
			AINF("FrameBuffer: usage:0x%" PRIx64 " format:%" PRIx64 " pixel_stride:%d width:%d height:%d heap_type:%d priv_heap_flag:%d",
							usage, bufDescriptor->hal_format, bufDescriptor->pixel_stride,bufDescriptor->width, bufDescriptor->height,
							heap_type, priv_heap_flag);
			}
			pHandle[i] = hnd;
		}
	}

	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
		private_handle_t *hnd = (private_handle_t *)(pHandle[i]);

		usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;
		hnd->usage = usage;

		if (!(usage & GRALLOC_USAGE_PROTECTED))
		{
			cpu_ptr =
			    (unsigned char *)mmap(NULL, bufDescriptor->size, PROT_READ | PROT_WRITE, MAP_SHARED, hnd->share_fd, 0);

			if (MAP_FAILED == cpu_ptr)
			{
				AERR("mmap failed from client ( %d ), fd ( %d )", m->ion_client, hnd->share_fd);
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}

#if GRALLOC_INIT_AFBC == 1

			if ((bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK) && (!(*shared_backend)))
			{
				init_afbc(cpu_ptr, bufDescriptor->internal_format, bufDescriptor->width, bufDescriptor->height);
			}

#endif
			hnd->base = cpu_ptr;
		}
	}

	return 0;
}

void mali_gralloc_ion_free(private_handle_t const *hnd)
{
	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		return;
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
		/* Buffer might be unregistered already so we need to assure we have a valid handle*/
		if (0 != hnd->base)
		{
			if (0 != munmap((void *)hnd->base, hnd->size))
			{
				AERR("Failed to munmap handle %p", hnd);
			}
		}

		close(hnd->share_fd);
		memset((void *)hnd, 0, sizeof(*hnd));
	}
}

static void mali_gralloc_ion_free_internal(buffer_handle_t *pHandle, uint32_t num_hnds)
{
	uint32_t i = 0;

	for (i = 0; i < num_hnds; i++)
	{
		if (NULL != pHandle[i])
		{
			mali_gralloc_ion_free((private_handle_t *)(pHandle[i]));
		}
	}

	return;
}

void mali_gralloc_ion_sync(const mali_gralloc_module *m, private_handle_t *hnd)
{
	if (m != NULL && hnd != NULL)
	{
		switch (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
		{
		case private_handle_t::PRIV_FLAGS_USES_ION:
			if (!(hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP))
			{
				ion_sync_fd(m->ion_client, hnd->share_fd);
			}

			break;
		}
	}
}

int mali_gralloc_ion_map(private_handle_t *hnd)
{
	int retval = -EINVAL;

	switch (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
	case private_handle_t::PRIV_FLAGS_USES_ION:
		unsigned char *mappedAddress;
		size_t size = hnd->size;
		hw_module_t *pmodule = NULL;
		private_module_t *m = NULL;

		if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&pmodule) == 0)
		{
			m = reinterpret_cast<private_module_t *>(pmodule);
		}
		else
		{
			AERR("Could not get gralloc module for handle: %p", hnd);
			retval = -errno;
			break;
		}

		/* the test condition is set to m->ion_client <= 0 here, because:
		 * 1) module structure are initialized to 0 if no initial value is applied
		 * 2) a second user process should get a ion fd greater than 0.
		 */
		if (m->ion_client <= 0)
		{
			/* a second user process must obtain a client handle first via ion_open before it can obtain the shared ion buffer*/
			m->ion_client = ion_open();

			if (m->ion_client < 0)
			{
				AERR("Could not open ion device for handle: %p", hnd);
				retval = -errno;
				break;
			}
		}

		ion_user_handle_t user_hnd;
		ion_import(m->ion_client, hnd->share_fd, &user_hnd);
		mappedAddress = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, hnd->share_fd, 0);

		imported_ion_client.emplace(hnd->share_fd, m->ion_client);
		imported_user_hnd.emplace(hnd->share_fd, user_hnd);
		AINF("ddebug, pair (share_fd=%d, user_hnd=%x, ion_client=%d)", hnd->share_fd, user_hnd, m->ion_client);

		if (MAP_FAILED == mappedAddress)
		{
			AERR("mmap( share_fd:%d ) failed with %s", hnd->share_fd, strerror(errno));
			retval = -errno;
			break;
		}

		hnd->base = (void *)(uintptr_t(mappedAddress) + hnd->offset);
		retval = 0;
		break;
	}

	return retval;
}

void mali_gralloc_ion_unmap(private_handle_t *hnd)
{
	switch (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
	case private_handle_t::PRIV_FLAGS_USES_ION:
		void *base = (void *)hnd->base;
		size_t size = hnd->size;

		if (munmap(base, size) < 0)
		{
			AERR("Could not munmap base:%p size:%zd '%s'", base, size, strerror(errno));
		}

		auto user_hnd_iter = imported_user_hnd.find(hnd->share_fd);
		auto ion_client_iter = imported_ion_client.find(hnd->share_fd);
		if (user_hnd_iter != imported_user_hnd.end()
				&& ion_client_iter != imported_ion_client.end()) {
			AINF("ddebug, free share_fd=%d, user_hnd=0x%x, ion client=%d\n",
				hnd->share_fd, user_hnd_iter->second, ion_client_iter->second);
			ion_free(ion_client_iter->second, user_hnd_iter->second);
			imported_user_hnd.erase(user_hnd_iter);
			imported_ion_client.erase(ion_client_iter);
		}

		break;
	}
}

int mali_gralloc_ion_device_close(struct hw_device_t *device)
{
#if GRALLOC_USE_GRALLOC1_API == 1
	gralloc1_device_t *dev = reinterpret_cast<gralloc1_device_t *>(device);
#else
	alloc_device_t *dev = reinterpret_cast<alloc_device_t *>(device);
#endif

	if (dev)
	{
		private_module_t *m = reinterpret_cast<private_module_t *>(dev->common.module);

		if (m->ion_client != -1)
		{
			if (0 != ion_close(m->ion_client))
			{
				AERR("Failed to close ion_client: %d err=%s", m->ion_client, strerror(errno));
			}

			m->ion_client = -1;
		}

		delete dev;
	}

	return 0;
}

#ifdef GRALLOC_AML_EXTEND
unsigned int am_pick_ion_heap(
	buffer_descriptor_t *bufDescriptor, uint64_t usage)
{
	if (usage & GRALLOC_USAGE_HW_FB)
	{
		return ION_HEAP_TYPE_DMA;
	}

	if (am_gralloc_is_omx_osd_extend_usage(usage))
	{
		return ION_HEAP_TYPE_CUSTOM;
	}

	if (am_gralloc_is_omx_metadata_extend_usage(usage))
	{
		return ION_HEAP_TYPE_SYSTEM;
	}

#ifdef AML_ALLOC_SCANOUT_FOR_COMPOSE
        #if BOARD_RESOLUTION_RATIO == 720
            static unsigned int max_composer_buf_width = 1280;
            static unsigned int max_composer_buf_height = 720;
        #else
            static unsigned int max_composer_buf_width = 1920;
            static unsigned int max_composer_buf_height = 1080;
        #endif

	if (usage & GRALLOC_USAGE_HW_COMPOSER)
	{
		if ( (bufDescriptor->width <= max_composer_buf_width) &&
			(bufDescriptor->height <= max_composer_buf_height) )
		{
			return ION_HEAP_TYPE_DMA;
		}
	}
 #else
        /*for compile warning.*/
        bufDescriptor;
#endif

	return ION_HEAP_TYPE_SYSTEM;
}

void am_set_ion_flags(unsigned int heap_type, uint64_t usage,
	unsigned int *priv_heap_flag, int *ion_flags)
{
	if (priv_heap_flag)
	{
		int coherent_buffer_flag = am_gralloc_get_coherent_extend_flag();

		if (heap_type == ION_HEAP_TYPE_SYSTEM)
		{
			*priv_heap_flag &= ~coherent_buffer_flag;
		}
		else if (heap_type == ION_HEAP_TYPE_DMA)
		{
			*priv_heap_flag |= coherent_buffer_flag;
		}

		/*Must check omx metadata first,
		*for it have some same bits with video overlay.
		*/
		if (am_gralloc_is_omx_metadata_extend_usage(usage))
		{
			*priv_heap_flag |= am_gralloc_get_omx_metadata_extend_flag();
		}
		else if (am_gralloc_is_video_overlay_extend_usage(usage))
		{
			*priv_heap_flag |= am_gralloc_get_video_overlay_extend_flag();
		}
		if (am_gralloc_is_secure_extend_usage(usage))
		{
		    *priv_heap_flag |= am_gralloc_get_secure_extend_flag();
		}
	}

	if (ion_flags)
	{
		if ((heap_type != ION_HEAP_TYPE_DMA) &&
			(heap_type != ION_HEAP_TYPE_CUSTOM))
		{
			if ((usage & (GRALLOC_USAGE_SW_WRITE_MASK | GRALLOC_USAGE_SW_READ_MASK))
				|| (usage == GRALLOC_USAGE_HW_TEXTURE))
			{
				*ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
			}
		}
	}
}

#endif
