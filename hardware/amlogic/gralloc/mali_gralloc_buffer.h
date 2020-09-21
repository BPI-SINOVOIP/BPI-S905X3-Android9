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
#ifndef MALI_GRALLOC_BUFFER_H_
#define MALI_GRALLOC_BUFFER_H_

#include <unistd.h>
#include <linux/ion.h>
#include <sys/mman.h>
#include <errno.h>
#include "mali_gralloc_private_interface_types.h"

/* NOTE:
 * If your framebuffer device driver is integrated with dma_buf, you will have to
 * change this IOCTL definition to reflect your integration with the framebuffer
 * device.
 * Expected return value is a structure filled with a file descriptor
 * backing your framebuffer device memory.
 */
struct fb_dmabuf_export
{
	__u32 fd;
	__u32 flags;
};
#define FBIOGET_DMABUF _IOR('F', 0x21, struct fb_dmabuf_export)

/* the max string size of GRALLOC_HARDWARE_GPU0 & GRALLOC_HARDWARE_FB0
 * 8 is big enough for "gpu0" & "fb0" currently
 */
#define MALI_GRALLOC_HARDWARE_MAX_STR_LEN 8
#define NUM_FB_BUFFERS 2

/* Define number of shared file descriptors */
#define GRALLOC_ARM_NUM_FDS 2

#define NUM_INTS_IN_PRIVATE_HANDLE ((sizeof(struct private_handle_t) - sizeof(native_handle)) / sizeof(int) - sNumFds)

#define SZ_4K 0x00001000
#define SZ_2M 0x00200000

struct private_handle_t;

#ifndef __cplusplus
/* C99 with pedantic don't allow anonymous unions which is used in below struct
 * Disable pedantic for C for this struct only.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#ifdef __cplusplus
struct private_handle_t : public native_handle
{
#else
struct private_handle_t
{
	struct native_handle nativeHandle;
#endif

#ifdef __cplusplus
	/* Never intended to be used from C code */
	enum
	{
		PRIV_FLAGS_FRAMEBUFFER = 0x00000001,
		PRIV_FLAGS_USES_ION_COMPOUND_HEAP = 0x00000002,
		PRIV_FLAGS_USES_ION = 0x00000004,
		PRIV_FLAGS_USES_ION_DMA_HEAP = 0x00000008,

		/*
			!!Dont use these flags directly.
		*/
		PRIV_FLAGS_VIDEO_OVERLAY = 0x00000010,
		PRIV_FLAGS_VIDEO_OMX     = 0x00000020,
		PRIV_FLAGS_VIDEO_AMCODEX = 0x00000040, // Notice usage in Hwc2Layer.cpp!!
		PRIV_FLAGS_CURSOR = 0x00000080,
		//PRIV_FLAGS_OSD_VIDEO_OMX = 0x00000080,
		PRIV_FLAGS_CONTINUOUS_BUF = 0x00000100,
		PRIV_FLAGS_SECURE_PROTECTED = 0x00000200,
	};

	enum
	{
		LOCK_STATE_WRITE = 1 << 31,
		LOCK_STATE_MAPPED = 1 << 30,
		LOCK_STATE_READ_MASK = 0x3FFFFFFF
	};
#endif

	/*
	 * Shared file descriptor for dma_buf sharing. This must be the first element in the
	 * structure so that binder knows where it is and can properly share it between
	 * processes.
	 * DO NOT MOVE THIS ELEMENT!
	 */
	int share_fd;
	int share_attr_fd;

	// ints
	int magic;
	int req_format;
	uint64_t internal_format;
	int byte_stride;
	int flags;
	int size;
	int width;
	int height;
	int format;
	int internalWidth;
	int internalHeight;
	int stride;

	/*
	 * Multi-layer buffers.
	 *
	 * Gralloc 1.0 supports multiple image layers within the same
	 * buffer allocation, where GRALLOC1_CAPABILITY_LAYERED_BUFFERS is enabled.
	 * 'layer_count' defines the number of layers that have been allocated.
	 * All layers are the same size (in bytes) and 'size' defines the
	 * number of bytes in the whole allocation.
	 * Size of each layer = 'size' / 'layer_count'.
	 * Offset to nth layer = n * ('size' / 'layer_count'),
	 * where n=0 for the first layer.
	 */
	uint32_t layer_count;
	union
	{
		void *base;
		uint64_t padding;
	};
	int      usage;
	uint64_t consumer_usage;
	uint64_t producer_usage;
	uint64_t backing_store_id;
	int backing_store_size;
	int writeOwner;
	int allocating_pid;
	int remote_pid;
	int ref_count;
	// locally mapped shared attribute area
	union
	{
		void *attr_base;
		uint64_t padding3;
	};

	mali_gralloc_yuv_info yuv_info;

	// Following members is for framebuffer only
	int fd;
	union
	{
		off_t offset;
		uint64_t padding4;
	};

	/*
	 * min_pgsz denotes minimum phys_page size used by this buffer.
	 * if buffer memory is physical contiguous set min_pgsz to buff->size
	 * if not sure buff's real phys_page size, you can use SZ_4K for safe.
	 */
	int min_pgsz;
	//for request width and height
	uint32_t req_width;
	uint32_t req_height;
	uint64_t padding_1;
	uint64_t padding_2;

#ifdef __cplusplus
	/*
	 * We track the number of integers in the structure. There are 16 unconditional
	 * integers (magic - pid, yuv_info, fd and offset). Note that the fd element is
	 * considered an int not an fd because it is not intended to be used outside the
	 * surface flinger process. The GRALLOC_ARM_NUM_INTS variable is used to track the
	 * number of integers that are conditionally included. Similar considerations apply
	 * to the number of fds.
	 */
	static const int sNumFds = GRALLOC_ARM_NUM_FDS;
	static const int sMagic = 0x3141592;

	private_handle_t(int _flags, int _size, void *_base, uint64_t _consumer_usage, uint64_t _producer_usage,
	                 int fb_file, off_t fb_offset)
	    : share_fd(-1)
	    , share_attr_fd(-1)
	    , magic(sMagic)
	    , flags(_flags)
	    , size(_size)
	    , width(0)
	    , height(0)
	    , stride(0)
	    , layer_count(0)
	    , base(_base)
	    , consumer_usage(_consumer_usage)
	    , producer_usage(_producer_usage)
	    , backing_store_id(0x0)
	    , backing_store_size(0)
	    , writeOwner(0)
	    , allocating_pid(getpid())
	    , remote_pid(-1)
	    , ref_count(1)
	    , attr_base(MAP_FAILED)
	    , yuv_info(MALI_YUV_NO_INFO)
	    , fd(fb_file)
	    , offset(fb_offset)
	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = NUM_INTS_IN_PRIVATE_HANDLE;
	}

	private_handle_t(int _flags, int _size, int _min_pgsz, uint64_t _consumer_usage, uint64_t _producer_usage,
	                 int _shared_fd, int _req_format, uint64_t _internal_format, int _byte_stride, int _width,
	                 int _height, int _stride, int _internalWidth, int _internalHeight, int _backing_store_size,
			 uint64_t _layer_count)
	    : share_fd(_shared_fd)
	    , share_attr_fd(-1)
	    , magic(sMagic)
	    , req_format(_req_format)
	    , internal_format(_internal_format)
	    , byte_stride(_byte_stride)
	    , flags(_flags)
	    , size(_size)
	    , width(_width)
	    , height(_height)
	    , internalWidth(_internalWidth)
	    , internalHeight(_internalHeight)
	    , stride(_stride)
	    , layer_count(_layer_count)
	    , base(NULL)
	    , consumer_usage(_consumer_usage)
	    , producer_usage(_producer_usage)
	    , backing_store_id(0x0)
	    , backing_store_size(_backing_store_size)
	    , writeOwner(0)
	    , allocating_pid(getpid())
	    , remote_pid(-1)
	    , ref_count(1)
	    , attr_base(MAP_FAILED)
	    , yuv_info(MALI_YUV_NO_INFO)
	    , fd(-1)
	    , offset(0)
	    , min_pgsz(_min_pgsz)
	{
		version = sizeof(native_handle);
		numFds = sNumFds;
		numInts = NUM_INTS_IN_PRIVATE_HANDLE;
		format  = _req_format;
	}

	~private_handle_t()
	{
		magic = 0;
	}

	bool usesPhysicallyContiguousMemory()
	{
		return (flags & PRIV_FLAGS_FRAMEBUFFER) ? true : false;
	}

	static int validate(const native_handle *h)
	{
		const private_handle_t *hnd = (const private_handle_t *)h;

		if (!h || h->version != sizeof(native_handle) || h->numInts != NUM_INTS_IN_PRIVATE_HANDLE ||
		    h->numFds != sNumFds || hnd->magic != sMagic)
		{
			return -EINVAL;
		}

		return 0;
	}

	static private_handle_t *dynamicCast(const native_handle *in)
	{
		if (validate(in) == 0)
		{
			return (private_handle_t *)in;
		}

		return NULL;
	}
#endif
};
#ifndef __cplusplus
/* Restore previous diagnostic for pedantic */
#pragma GCC diagnostic pop
#endif

#endif /* MALI_GRALLOC_BUFFER_H_ */
