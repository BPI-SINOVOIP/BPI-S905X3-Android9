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

#include <hardware/hardware.h>
#include <inttypes.h>
#include <atomic>

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
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_ion.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_buffer_priv.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_debug.h"
#include <utils/CallStack.h>

#define AFBC_PIXELS_PER_BLOCK 16
#define AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY 16

#define AFBC_BODY_BUFFER_BYTE_ALIGNMENT 1024
#define AFBC_NORMAL_WIDTH_ALIGN 16
#define AFBC_NORMAL_HEIGHT_ALIGN 16
#define AFBC_WIDEBLK_WIDTH_ALIGN 32
#define AFBC_WIDEBLK_HEIGHT_ALIGN 16
// Regarding Tiled Headers AFBC mode, both header and body buffer should aligned to 4KB
// and in non-wide mode (16x16), the width and height should be both rounded up to 128
// in wide mode (32x8) the width should be rounded up to 256, the height should be rounded up to 64
#define AFBC_TILED_HEADERS_BASIC_WIDTH_ALIGN 128
#define AFBC_TILED_HEADERS_BASIC_HEIGHT_ALIGN 128
#define AFBC_TILED_HEADERS_WIDEBLK_WIDTH_ALIGN 256
#define AFBC_TILED_HEADERS_WIDEBLK_HEIGHT_ALIGN 64

// This value is platform specific and should be set according to hardware YUV planes restrictions.
// Please note that EGL winsys platform config file needs to use the same value when importing buffers.
#define YUV_MALI_PLANE_ALIGN 32

// Default YUV stride aligment in Android
#define YUV_ANDROID_PLANE_ALIGN 16


/*
 * Type of allocation
 */
enum AllocType
{
	UNCOMPRESSED = 0,
	AFBC,
	/* AFBC_WIDEBLK mode requires buffer to have 32 * 16 pixels alignment */
	AFBC_WIDEBLK,
	/* AN AFBC buffer with additional padding to ensure a 64-bte alignment
	 * for each row of blocks in the header */
	AFBC_PADDED,
	/* AFBC_TILED_HEADERS_AFBC_BASIC mode requires buffer to have 128*128 pixels alignment(16x16 superblocks) */
	AFBC_TILED_HEADERS_BASIC,
	/* AFBC_TILED_HEADERS_AFBC_WIDEBLK mode requires buffer to have 256*64 pixels alignment(32x8 superblocks) */
	AFBC_TILED_HEADERS_WIDEBLK,
};

static int mali_gralloc_buffer_free_internal(buffer_handle_t *pHandle, uint32_t num_hnds);

/*
 * Get a global unique ID
 */
static uint64_t getUniqueId()
{
	static std::atomic<uint32_t> counter(0);
	uint64_t id = static_cast<uint64_t>(getpid()) << 32;
	return id | counter++;
}

/*
 * Computes the strides and size for an RGB buffer
 *
 * width               width of the buffer in pixels
 * height              height of the buffer in pixels
 * pixel_size          size of one pixel in bytes
 *
 * pixel_stride (out)  stride of the buffer in pixels
 * byte_stride  (out)  stride of the buffer in bytes
 * size         (out)  size of the buffer in bytes
 * type         (in)   if buffer should be allocated for afbc
 */
static void get_rgb_stride_and_size(int width, int height, int pixel_size, int *pixel_stride, int *byte_stride,
                                    size_t *size, AllocType type)
{
	int stride;

	stride = width * pixel_size;

	/* Align the lines to 64 bytes.
	 * It's more efficient to write to 64-byte aligned addresses because it's the burst size on the bus */
	stride = GRALLOC_ALIGN(stride, 64);

	if (size != NULL)
	{
		*size = stride *height;
	}

	if (byte_stride != NULL)
	{
		*byte_stride = stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = stride / pixel_size;
	}

	if (type != UNCOMPRESSED)
	{
		int w_aligned;
		int h_aligned = GRALLOC_ALIGN(height, AFBC_NORMAL_HEIGHT_ALIGN);
		int nblocks;
		int buffer_byte_alignment = AFBC_BODY_BUFFER_BYTE_ALIGNMENT;

		if (type == AFBC_TILED_HEADERS_BASIC)
		{
			w_aligned = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_BASIC_WIDTH_ALIGN);
			h_aligned = GRALLOC_ALIGN(height, AFBC_TILED_HEADERS_BASIC_HEIGHT_ALIGN);
			buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
		}
		else if (type == AFBC_TILED_HEADERS_WIDEBLK)
		{
			w_aligned = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_WIDEBLK_WIDTH_ALIGN);
			h_aligned = GRALLOC_ALIGN(height, AFBC_TILED_HEADERS_WIDEBLK_HEIGHT_ALIGN);
			buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
		}
		else if (type == AFBC_PADDED)
		{
			w_aligned = GRALLOC_ALIGN(width, 64);
		}
		else if (type == AFBC_WIDEBLK)
		{
			w_aligned = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
			h_aligned = GRALLOC_ALIGN(height, AFBC_WIDEBLK_HEIGHT_ALIGN);
		}
		else
		{
			w_aligned = GRALLOC_ALIGN(width, AFBC_NORMAL_WIDTH_ALIGN);
		}

		stride = w_aligned * pixel_size;
		stride = GRALLOC_ALIGN(stride, 64);

		if (byte_stride != NULL)
		{
			*byte_stride = stride;
		}

		if (pixel_stride != NULL)
		{
			*pixel_stride = stride / pixel_size;
		}

		nblocks = w_aligned / AFBC_PIXELS_PER_BLOCK * h_aligned / AFBC_PIXELS_PER_BLOCK;

		if (size != NULL)
		{
			*size = stride *h_aligned +
			        GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, buffer_byte_alignment);
		}
	}
}

/*
 * Computes the strides and size for an AFBC 8BIT YUV 4:2:0 buffer
 *
 * width                Public known width of the buffer in pixels
 * height               Public known height of the buffer in pixels
 *
 * pixel_stride   (out) stride of the buffer in pixels
 * byte_stride    (out) stride of the buffer in bytes
 * size           (out) size of the buffer in bytes
 * type                 if buffer should be allocated for a certain afbc type
 * internalHeight (out) The internal height, which may be greater than the public known height.
 */
static bool get_afbc_yuv420_8bit_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride,
                                                 size_t *size, AllocType type, int *internalHeight)
{
	int yuv420_afbc_luma_stride, yuv420_afbc_chroma_stride;
	int buffer_byte_alignment = AFBC_BODY_BUFFER_BYTE_ALIGNMENT;

	*internalHeight = height;

#if MALI_VIDEO_VERSION != 0

	/* If we have a greater internal height than public we set the internalHeight. This
	 * implies that cropping will be applied of internal dimensions to fit the public one.
	 *
	 * NOTE: This should really only be done when the producer is determined to be VPU decoder.
	 */
	*internalHeight += AFBC_PIXELS_PER_BLOCK;
#endif

	/* The actual height used in size calculation must include the possible extra row. But
	 * it must also be AFBC-aligned. Only the extra row-padding should be reported back in
	 * internalHeight. This as only this row needs to be considered when cropping. */

	if (type == UNCOMPRESSED)
	{
		AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV420_8BIT_AFBC!");
		return false;
	}
	else if (type == AFBC_TILED_HEADERS_BASIC)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_BASIC_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight, AFBC_TILED_HEADERS_BASIC_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_TILED_HEADERS_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight, AFBC_TILED_HEADERS_WIDEBLK_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_PADDED)
	{
		AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
		return false;
	}
	else if (type == AFBC_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight, AFBC_WIDEBLK_HEIGHT_ALIGN);
	}
	else
	{
		width = GRALLOC_ALIGN(width, AFBC_NORMAL_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight, AFBC_NORMAL_HEIGHT_ALIGN);
	}

	yuv420_afbc_luma_stride = width;
	yuv420_afbc_chroma_stride = GRALLOC_ALIGN(yuv420_afbc_luma_stride / 2, 16); /* Horizontal downsampling*/

	if (size != NULL)
	{
		int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;
		/* Simplification of (height * luma-stride + 2 * (height /2 * chroma_stride) */
		*size = (yuv420_afbc_luma_stride + yuv420_afbc_chroma_stride) * height +
		        GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, buffer_byte_alignment);
	}

	if (byte_stride != NULL)
	{
		*byte_stride = yuv420_afbc_luma_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = yuv420_afbc_luma_stride;
	}

	return true;
}

/*
 * Computes the strides and size for an YV12 buffer
 *
 * width                  Public known width of the buffer in pixels
 * height                 Public known height of the buffer in pixels
 *
 * pixel_stride     (out) stride of the buffer in pixels
 * byte_stride      (out) stride of the buffer in bytes
 * size             (out) size of the buffer in bytes
 * type             (in)  if buffer should be allocated for a certain afbc type
 * internalHeight   (out) The internal height, which may be greater than the public known height.
 * stride_alignment (in)  stride aligment value in bytes.
 */
static bool get_yv12_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride, size_t *size,
                                     AllocType type, int *internalHeight, int stride_alignment)
{
	int luma_stride;

	if (type != UNCOMPRESSED)
	{
		return get_afbc_yuv420_8bit_stride_and_size(width, height, pixel_stride, byte_stride, size, type,
		                                            internalHeight);
	}

	/* 4:2:0 formats must have buffers with even height and width as the clump size is 2x2 pixels.
	 * Width will be even stride aligned anyway so just adjust height here for size calculation. */
	height = GRALLOC_ALIGN(height, 2);

	luma_stride = GRALLOC_ALIGN(width, stride_alignment);

	if (size != NULL)
	{
		int chroma_stride = GRALLOC_ALIGN(luma_stride / 2, stride_alignment);
		/* Simplification of ((height * luma_stride ) + 2 * ((height / 2) * chroma_stride)). */
		*size = height *(luma_stride + chroma_stride);
	}

	if (byte_stride != NULL)
	{
		*byte_stride = luma_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = luma_stride;
	}

	return true;
}

/*
 * Computes the strides and size for an 8 bit YUYV 422 buffer
 *
 * width                  Public known width of the buffer in pixels
 * height                 Public known height of the buffer in pixels
 *
 * pixel_stride     (out) stride of the buffer in pixels
 * byte_stride      (out) stride of the buffer in bytes
 * size             (out) size of the buffer in bytes
 */
static bool get_yuv422_8bit_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride, size_t *size)
{
	int local_byte_stride, local_pixel_stride;

	/* 4:2:2 formats must have buffers with even width as the clump size is 2x1 pixels.
	 * This is taken care of by the even stride alignment. */

	local_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
	local_byte_stride = GRALLOC_ALIGN(width * 2, YUV_MALI_PLANE_ALIGN); /* 4 bytes per 2 pixels */

	if (size != NULL)
	{
		*size = local_byte_stride *height;
	}

	if (byte_stride != NULL)
	{
		*byte_stride = local_byte_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = local_pixel_stride;
	}

	return true;
}

/*
 * Computes the strides and size for an AFBC 8BIT YUV 4:2:2 buffer
 *
 * width               width of the buffer in pixels
 * height              height of the buffer in pixels
 *
 * pixel_stride (out)  stride of the buffer in pixels
 * byte_stride  (out)  stride of the buffer in bytes
 * size         (out)  size of the buffer in bytes
 * type                if buffer should be allocated for a certain afbc type
 */
static bool get_afbc_yuv422_8bit_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride,
                                                 size_t *size, AllocType type)
{
	int yuv422_afbc_luma_stride;
	int buffer_byte_alignment = AFBC_BODY_BUFFER_BYTE_ALIGNMENT;

	if (type == UNCOMPRESSED)
	{
		AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV422_8BIT_AFBC!");
		return false;
	}
	else if (type == AFBC_TILED_HEADERS_BASIC)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_BASIC_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_TILED_HEADERS_BASIC_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_TILED_HEADERS_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_TILED_HEADERS_WIDEBLK_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_PADDED)
	{
		AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
		return false;
	}
	else if (type == AFBC_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_WIDEBLK_HEIGHT_ALIGN);
	}
	else
	{
		width = GRALLOC_ALIGN(width, AFBC_NORMAL_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_NORMAL_HEIGHT_ALIGN);
	}

	yuv422_afbc_luma_stride = width;

	if (size != NULL)
	{
		int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;
		/* YUV 4:2:2 luma size equals chroma size */
		*size = yuv422_afbc_luma_stride *height * 2 +
		        GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, buffer_byte_alignment);
	}

	if (byte_stride != NULL)
	{
		*byte_stride = yuv422_afbc_luma_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = yuv422_afbc_luma_stride;
	}

	return true;
}

/*
 * Calculate strides and sizes for a P010 (Y-UV 4:2:0) or P210 (Y-UV 4:2:2) buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param vss           [in]    Vertical sub-sampling factor (2 for P010, 1 for
 *                              P210. Anything else is invalid).
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv_pX10_stride_and_size(int width, int height, int vss, int *pixel_stride, int *byte_stride,
                                         size_t *size)
{
	int luma_pixel_stride, luma_byte_stride;

	if (vss < 1 || vss > 2)
	{
		AERR("Invalid vertical sub-sampling factor: %d, should be 1 or 2", vss);
		return false;
	}

	/* 4:2:2 must have even width as the clump size is 2x1 pixels. This will be taken care of by the
	 * even stride alignment */
	if (vss == 2)
	{
		/* 4:2:0 must also have even height as the clump size is 2x2 */
		height = GRALLOC_ALIGN(height, 2);
	}

	luma_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
	luma_byte_stride = GRALLOC_ALIGN(width * 2, YUV_MALI_PLANE_ALIGN);

	if (size != NULL)
	{
		int chroma_size = GRALLOC_ALIGN(width * 2, YUV_MALI_PLANE_ALIGN) * (height / vss);
		*size = luma_byte_stride *height + chroma_size;
	}

	if (byte_stride != NULL)
	{
		*byte_stride = luma_byte_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = luma_pixel_stride;
	}

	return true;
}

/*
 *  Calculate strides and strides for Y210 (10 bit YUYV packed, 4:2:2) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv_y210_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride, size_t *size)
{
	int y210_byte_stride, y210_pixel_stride;

	/* 4:2:2 formats must have buffers with even width as the clump size is 2x1 pixels.
	 * This is taken care of by the even stride alignment */

	y210_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
	/* 4x16 bits per 2 pixels */
	y210_byte_stride = GRALLOC_ALIGN(width * 4, YUV_MALI_PLANE_ALIGN);

	if (size != NULL)
	{
		*size = y210_byte_stride *height;
	}

	if (byte_stride != NULL)
	{
		*byte_stride = y210_byte_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = y210_pixel_stride;
	}

	return true;
}

/*
 *  Calculate strides and strides for Y0L2 (YUYAAYVYAA, 4:2:0) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 *
 * @note Each YUYAAYVYAA clump encodes a 2x2 area of pixels. YU&V are 10 bits. A is 1 bit. total 8 bytes
 *
 */
static bool get_yuv_y0l2_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride, size_t *size)
{
	int y0l2_byte_stride, y0l2_pixel_stride;

	/* 4:2:0 formats must have buffers with even height and width as the clump size is 2x2 pixels.
	 * Width is take care of by the even stride alignment so just adjust height here for size calculation. */
	height = GRALLOC_ALIGN(height, 2);

	y0l2_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
	y0l2_byte_stride = GRALLOC_ALIGN(width * 4, YUV_MALI_PLANE_ALIGN); /* 2 horiz pixels per 8 byte clump */

	if (size != NULL)
	{
		*size = y0l2_byte_stride *height / 2; /* byte stride covers 2 vert pixels */
	}

	if (byte_stride != NULL)
	{
		*byte_stride = y0l2_byte_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = y0l2_pixel_stride;
	}

	return true;
}

/*
 *  Calculate strides and strides for Y410 (AVYU packed, 4:4:4) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv_y410_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride, size_t *size)
{
	int y410_byte_stride, y410_pixel_stride;

	y410_pixel_stride = GRALLOC_ALIGN(width, YUV_MALI_PLANE_ALIGN);
	y410_byte_stride = GRALLOC_ALIGN(width * 4, YUV_MALI_PLANE_ALIGN);

	if (size != NULL)
	{
		/* 4x8bits per pixel */
		*size = y410_byte_stride *height;
	}

	if (byte_stride != NULL)
	{
		*byte_stride = y410_byte_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = y410_pixel_stride;
	}

	return true;
}

/*
 *  Calculate strides and strides for YUV420_10BIT_AFBC (Compressed, 4:2:0) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 * @param type          [in]    afbc mode that buffer should be allocated with.
 *
 * @param internalHeight [out]  Internal buffer height that used by consumer or producer
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv420_10bit_afbc_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride,
                                                  size_t *size, AllocType type, int *internalHeight)
{
	int yuv420_afbc_byte_stride, yuv420_afbc_pixel_stride;
	int buffer_byte_alignment = AFBC_BODY_BUFFER_BYTE_ALIGNMENT;

	if (width & 3)
	{
		return false;
	}

	*internalHeight = height;
#if MALI_VIDEO_VERSION
	/* If we have a greater internal height than public we set the internalHeight. This
	 * implies that cropping will be applied of internal dimensions to fit the public one. */
	*internalHeight += AFBC_PIXELS_PER_BLOCK;
#endif

	/* The actual height used in size calculation must include the possible extra row. But
	 * it must also be AFBC-aligned. Only the extra row-padding should be reported back in
	 * internalHeight. This as only this row needs to be considered when cropping. */
	if (type == UNCOMPRESSED)
	{
		AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV420_10BIT_AFBC!");
		return false;
	}
	else if (type == AFBC_TILED_HEADERS_BASIC)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_BASIC_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight / 2, AFBC_TILED_HEADERS_BASIC_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_TILED_HEADERS_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight / 2, AFBC_TILED_HEADERS_WIDEBLK_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_PADDED)
	{
		AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
		return false;
	}
	else if (type == AFBC_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight / 2, AFBC_WIDEBLK_HEIGHT_ALIGN);
	}
	else
	{
		width = GRALLOC_ALIGN(width, AFBC_NORMAL_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(*internalHeight / 2, AFBC_NORMAL_HEIGHT_ALIGN);
	}

	yuv420_afbc_pixel_stride = GRALLOC_ALIGN(width, 16);
	yuv420_afbc_byte_stride = GRALLOC_ALIGN(width * 4, 16); /* 64-bit packed and horizontally downsampled */

	if (size != NULL)
	{
		int nblocks = width / AFBC_PIXELS_PER_BLOCK * (*internalHeight) / AFBC_PIXELS_PER_BLOCK;
		*size = yuv420_afbc_byte_stride *height +
		        GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, buffer_byte_alignment);
	}

	if (byte_stride != NULL)
	{
		*byte_stride = yuv420_afbc_pixel_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = yuv420_afbc_pixel_stride;
	}

	return true;
}

/*
 *  Calculate strides and strides for YUV422_10BIT_AFBC (Compressed, 4:2:2) format buffer.
 *
 * @param width         [in]    Buffer width.
 * @param height        [in]    Buffer height.
 * @param pixel_stride  [out]   Pixel stride; number of pixels between
 *                              consecutive rows.
 * @param byte_stride   [out]   Byte stride; number of bytes between
 *                              consecutive rows.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 * @param type          [in]    afbc mode that buffer should be allocated with.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_yuv422_10bit_afbc_stride_and_size(int width, int height, int *pixel_stride, int *byte_stride,
                                                  size_t *size, AllocType type)
{
	int yuv422_afbc_byte_stride, yuv422_afbc_pixel_stride;
	int buffer_byte_alignment = AFBC_BODY_BUFFER_BYTE_ALIGNMENT;

	if (width & 3)
	{
		return false;
	}

	if (type == UNCOMPRESSED)
	{
		AERR(" Buffer must be allocated with AFBC mode for internal pixel format YUV422_10BIT_AFBC!");
		return false;
	}
	else if (type == AFBC_TILED_HEADERS_BASIC)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_BASIC_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_TILED_HEADERS_BASIC_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_TILED_HEADERS_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_TILED_HEADERS_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_TILED_HEADERS_WIDEBLK_HEIGHT_ALIGN);
		buffer_byte_alignment = 4 * AFBC_BODY_BUFFER_BYTE_ALIGNMENT;
	}
	else if (type == AFBC_PADDED)
	{
		AERR("GRALLOC_USAGE_PRIVATE_2 (64byte header row alignment for AFBC) is not supported for YUV");
		return false;
	}
	else if (type == AFBC_WIDEBLK)
	{
		width = GRALLOC_ALIGN(width, AFBC_WIDEBLK_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_WIDEBLK_HEIGHT_ALIGN);
	}
	else
	{
		width = GRALLOC_ALIGN(width, AFBC_NORMAL_WIDTH_ALIGN);
		height = GRALLOC_ALIGN(height, AFBC_NORMAL_HEIGHT_ALIGN);
	}

	yuv422_afbc_pixel_stride = GRALLOC_ALIGN(width, 16);
	yuv422_afbc_byte_stride = GRALLOC_ALIGN(width * 2, 16);

	if (size != NULL)
	{
		int nblocks = width / AFBC_PIXELS_PER_BLOCK * height / AFBC_PIXELS_PER_BLOCK;
		/* YUV 4:2:2 chroma size equals to luma size */
		*size = yuv422_afbc_byte_stride *height * 2 +
		        GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, buffer_byte_alignment);
	}

	if (byte_stride != NULL)
	{
		*byte_stride = yuv422_afbc_byte_stride;
	}

	if (pixel_stride != NULL)
	{
		*pixel_stride = yuv422_afbc_pixel_stride;
	}

	return true;
}

/*
 *  Calculate strides and strides for Camera RAW and Blob formats
 *
 * @param w             [in]    Buffer width.
 * @param h             [in]    Buffer height.
 * @param format        [in]    Requested HAL format
 * @param out_stride    [out]   Pixel stride; number of pixels/bytes between
 *                              consecutive rows. Format description calls for
 *                              either bytes or pixels.
 * @param size          [out]   Size of the buffer in bytes. Cumulative sum of
 *                              sizes of all planes.
 *
 * @return true if the calculation was successful; false otherwise (invalid
 * parameter)
 */
static bool get_camera_formats_stride_and_size(int w, int h, uint64_t format, int *out_stride, size_t *out_size)
{
	int stride, size;

	switch (format)
	{
	case HAL_PIXEL_FORMAT_RAW16:
		stride = w; /* Format assumes stride in pixels */
		stride = GRALLOC_ALIGN(stride, 16); /* Alignment mandated by Android */
		size = stride * h * 2; /* 2 bytes per pixel */
		break;

	case HAL_PIXEL_FORMAT_RAW12:
		if (w % 4 != 0)
		{
			ALOGE("ERROR: Width for HAL_PIXEL_FORMAT_RAW12 buffers has to be multiple of 4.");
			return false;
		}

		stride = (w / 2) * 3; /* Stride in bytes; 2 pixels in 3 bytes */
		size = stride * h;
		break;

	case HAL_PIXEL_FORMAT_RAW10:
		if (w % 4 != 0)
		{
			ALOGE("ERROR: Width for HAL_PIXEL_FORMAT_RAW10 buffers has to be multiple of 4.");
			return false;
		}

		stride = (w / 4) * 5; /* Stride in bytes; 4 pixels in 5 bytes */
		size = stride * h;
		break;

	case HAL_PIXEL_FORMAT_BLOB:
		if (h != 1)
		{
			ALOGE("ERROR: Height for HAL_PIXEL_FORMAT_BLOB must be 1.");
			return false;
		}

		stride = w; /* No 'rows', it's effectively a long one dimensional array */
		size = w;
		break;

	default:
		return false;
	}

	if (out_size != NULL)
	{
		*out_size = size;
	}

	if (out_stride != NULL)
	{
		*out_stride = stride;
	}

	return true;
}

int mali_gralloc_buffer_allocate(mali_gralloc_module *m, const gralloc_buffer_descriptor_t *descriptors,
                                 uint32_t numDescriptors, buffer_handle_t *pHandle, bool *shared_backend)
{
	bool shared = false;
	uint64_t backing_store_id = 0x0;
	AllocType alloc_type = UNCOMPRESSED;
	uint64_t usage;
	uint32_t i = 0;
	int err;
	int64_t *req_wh = new int64_t[numDescriptors];

	int yv12_align = YUV_MALI_PLANE_ALIGN;
	int buffer_width;
	/* This flag is to avoid the mutually exclusive modifier bits warning being continuously emitted.
	 * (see comment below for explanation of warning).
	 */
	static bool warn_about_mutual_exclusive = true;

	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);

		/* Some formats require an internal width and height that may be used by
		 * consumers/producers.
		 */
		buffer_width = bufDescriptor->width;
		usage = bufDescriptor->producer_usage | bufDescriptor->consumer_usage;

		req_wh[i] = bufDescriptor->width | (bufDescriptor->height << 16);
#ifdef GRALLOC_AML_EXTEND
#if PLATFORM_SDK_VERSION >= 24
		if (am_gralloc_is_omx_metadata_extend_usage(usage))
		{
			bufDescriptor->width	= OMX_VIDEOLAYER_ALLOC_BUFFER_WIDTH;
			bufDescriptor->height   = OMX_VIDEOLAYER_ALLOC_BUFFER_HEIGHT;
		}
#endif
#endif
		bufDescriptor->internalWidth = bufDescriptor->width;
		bufDescriptor->internalHeight = bufDescriptor->height;

		bufDescriptor->internal_format = mali_gralloc_select_format(
		    bufDescriptor->hal_format, bufDescriptor->format_type, usage, bufDescriptor->width * bufDescriptor->height);

		if (bufDescriptor->internal_format == 0)
		{
			ALOGE("Unrecognized and/or unsupported format 0x%" PRIx64 " and usage 0x%" PRIx64,
			      bufDescriptor->hal_format, usage);
			delete []req_wh;
			return -EINVAL;
		}
		/*
		 * Modifier bits are no longer mutually exclusive. Warn when
		 * any bits are set in addition to AFBC basic since these might
		 * have been handled differently by clients under the old scheme.
		 * AFBC basic is guaranteed to be signalled when any other AFBC
		 * flags are set.
		 */
		else if (warn_about_mutual_exclusive &&
		         (bufDescriptor->internal_format & 0x0000000100000000ULL) &&
		         (bufDescriptor->internal_format & 0x0000000e00000000ULL))
		{
			warn_about_mutual_exclusive = false;
			ALOGW("WARNING: internal format modifier bits not mutually exclusive. AFBC basic bit is always set, so extended AFBC support bits must always be checked.");
		}

		/* Determine AFBC type for this format. This is used to decide alignment.
		 * Split block does not affect alignment, and therefore doesn't affect the AllocType. */
		if (bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
		{
			if (bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS)
			{
				if (bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK)
				{
					alloc_type = AFBC_TILED_HEADERS_WIDEBLK;
				}
				else if (bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBC_BASIC)
				{
					alloc_type = AFBC_TILED_HEADERS_BASIC;
				}
			}
			else if (usage & MALI_GRALLOC_USAGE_AFBC_PADDING)
			{
				alloc_type = AFBC_PADDED;
			}
			else if (bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK)
			{
				alloc_type = AFBC_WIDEBLK;
			}
			else
			{
				alloc_type = AFBC;
			}
		}

		uint64_t base_format = bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_FMT_MASK;

		switch (base_format)
		{
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
		case HAL_PIXEL_FORMAT_RGBA_1010102:
#endif
			get_rgb_stride_and_size(bufDescriptor->width, bufDescriptor->height, 4, &bufDescriptor->pixel_stride,
			                        &bufDescriptor->byte_stride, &bufDescriptor->size, alloc_type);
			break;

		case HAL_PIXEL_FORMAT_RGB_888:
			if (usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER ))
			{
				ALOGD("format 0x%" PRIx64 " and usage 0x%" PRIx64,
			      base_format, usage);
				delete []req_wh;
				return -EINVAL;
			}
			get_rgb_stride_and_size(bufDescriptor->width, bufDescriptor->height, 3, &bufDescriptor->pixel_stride,
			                        &bufDescriptor->byte_stride, &bufDescriptor->size, alloc_type);
			break;

		case HAL_PIXEL_FORMAT_RGB_565:
			get_rgb_stride_and_size(bufDescriptor->width, bufDescriptor->height, 2, &bufDescriptor->pixel_stride,
			                        &bufDescriptor->byte_stride, &bufDescriptor->size, alloc_type);
			break;

#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
		case HAL_PIXEL_FORMAT_RGBA_FP16:
			get_rgb_stride_and_size(bufDescriptor->width, bufDescriptor->height, 8, &bufDescriptor->pixel_stride,
			                        &bufDescriptor->byte_stride, &bufDescriptor->size, alloc_type);
			break;
#endif

		case MALI_GRALLOC_FORMAT_INTERNAL_YV12:
			/* Mali subsystem prefers higher stride alignment values (128 bytes) for YUV, but software components assume
			 * default of 16. We only need to care about YV12 as it's the only, implicit, HAL YUV format in Android.
			 */

			if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
			{
				yv12_align = YUV_ANDROID_PLANE_ALIGN;
			}
            /*
             * fallthrough
             */

		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case HAL_PIXEL_FORMAT_YCbCr_420_888:
		case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
		case MALI_GRALLOC_FORMAT_INTERNAL_NV21:
		{
			if (!get_yv12_stride_and_size(bufDescriptor->width, bufDescriptor->height, &bufDescriptor->pixel_stride,
			                              &bufDescriptor->byte_stride, &bufDescriptor->size, alloc_type,
			                              &bufDescriptor->internalHeight, yv12_align))
			{
				delete []req_wh;
				return -EINVAL;
			}
#ifdef GRALLOC_AML_EXTEND
#if PLATFORM_SDK_VERSION >= 24
			if (am_gralloc_is_omx_metadata_extend_usage(usage))
			{
				bufDescriptor->pixel_stride = buffer_width;
			}
#endif
#endif
			break;
		}

		case HAL_PIXEL_FORMAT_YCbCr_422_I:
		{
			/* YUYV 4:2:2 */
			if (alloc_type != UNCOMPRESSED ||
			    !get_yuv422_8bit_stride_and_size(bufDescriptor->width, bufDescriptor->height,
			                                     &bufDescriptor->pixel_stride, &bufDescriptor->byte_stride,
			                                     &bufDescriptor->size))
			{
				delete []req_wh;
				return -EINVAL;
			}

			break;
		}

		case HAL_PIXEL_FORMAT_RAW16:
		case HAL_PIXEL_FORMAT_RAW12:
		case HAL_PIXEL_FORMAT_RAW10:
		case HAL_PIXEL_FORMAT_BLOB:
			if (alloc_type != UNCOMPRESSED)
			{
				delete []req_wh;
				return -EINVAL;
			}

			get_camera_formats_stride_and_size(bufDescriptor->width, bufDescriptor->height, base_format,
			                                   &bufDescriptor->pixel_stride, &bufDescriptor->size);
			/* For Raw/Blob formats stride is defined to be either in bytes or pixels per format */
			bufDescriptor->byte_stride = bufDescriptor->pixel_stride;
			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_Y0L2:

			/* YUYAAYUVAA 4:2:0 with and without AFBC */
			if (alloc_type != UNCOMPRESSED)
			{
				if (!get_yuv420_10bit_afbc_stride_and_size(
				        bufDescriptor->width, bufDescriptor->height, &bufDescriptor->pixel_stride,
				        &bufDescriptor->byte_stride, &bufDescriptor->size, alloc_type, &bufDescriptor->internalHeight))
				{
					delete []req_wh;
					return -EINVAL;
				}
			}
			else
			{
				if (!get_yuv_y0l2_stride_and_size(bufDescriptor->width, bufDescriptor->height,
				                                  &bufDescriptor->pixel_stride, &bufDescriptor->byte_stride,
				                                  &bufDescriptor->size))
				{
					delete []req_wh;
					return -EINVAL;
				}
			}

			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_P010:

			/* Y-UV 4:2:0 */
			if (alloc_type != UNCOMPRESSED ||
			    !get_yuv_pX10_stride_and_size(bufDescriptor->width, bufDescriptor->height, 2,
			                                  &bufDescriptor->pixel_stride, &bufDescriptor->byte_stride,
			                                  &bufDescriptor->size))
			{
				delete []req_wh;
				return -EINVAL;
			}

			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_P210:

			/* Y-UV 4:2:2 */
			if (alloc_type != UNCOMPRESSED ||
			    !get_yuv_pX10_stride_and_size(bufDescriptor->width, bufDescriptor->height, 1,
			                                  &bufDescriptor->pixel_stride, &bufDescriptor->byte_stride,
			                                  &bufDescriptor->size))
			{
				delete []req_wh;
				return -EINVAL;
			}

			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_Y210:

			/* YUYV 4:2:2 with and without AFBC */
			if (alloc_type != UNCOMPRESSED)
			{
				if (!get_yuv422_10bit_afbc_stride_and_size(bufDescriptor->width, bufDescriptor->height,
				                                           &bufDescriptor->pixel_stride, &bufDescriptor->byte_stride,
				                                           &bufDescriptor->size, alloc_type))
				{
					delete []req_wh;
					return -EINVAL;
				}
			}
			else
			{
				if (!get_yuv_y210_stride_and_size(bufDescriptor->width, bufDescriptor->height,
				                                  &bufDescriptor->pixel_stride, &bufDescriptor->byte_stride,
				                                  &bufDescriptor->size))
				{
					delete []req_wh;
					return -EINVAL;
				}
			}

			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_Y410:

			/* AVYU 2-10-10-10 */
			if (alloc_type != UNCOMPRESSED ||
			    !get_yuv_y410_stride_and_size(bufDescriptor->width, bufDescriptor->height, &bufDescriptor->pixel_stride,
			                                  &bufDescriptor->byte_stride, &bufDescriptor->size))
			{
				delete []req_wh;
				return -EINVAL;
			}

			break;

		case MALI_GRALLOC_FORMAT_INTERNAL_YUV422_8BIT:

			/* 8BIT AFBC YUV4:2:2 testing usage */

			/* We only support compressed for this format right now.
			 * Below will fail in case format is uncompressed.
			 */
			if (!get_afbc_yuv422_8bit_stride_and_size(bufDescriptor->width, bufDescriptor->height,
			                                          &bufDescriptor->pixel_stride, &bufDescriptor->byte_stride,
			                                          &bufDescriptor->size, alloc_type))
			{
				delete []req_wh;
				return -EINVAL;
			}

			break;

		/*
		 * Additional custom formats can be added here
		 * and must fill the variables pixel_stride, byte_stride and size.
		 */
		default:
			AERR("unsupport format 0x%" PRIx64, base_format);
			delete []req_wh;
			return -EINVAL;
		}

		/*
		 * Each layer of a multi-layer buffer must be aligned so that
		 * it is accessible by both producer and consumer. In most cases,
		 * the stride alignment is also sufficient for each layer, however
		 * for AFBC the header buffer alignment is more constrained (see
		 * AFBC specification v3.4, section 2.15: "Alignment requirements").
		 * Also update the buffer size to accommodate all layers.
		 */
		if (bufDescriptor->layer_count > 1)
		{
			if (bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
			{
				if (bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS)
				{
					bufDescriptor->size = GRALLOC_ALIGN(bufDescriptor->size, 4096);
				}
				else
				{
					bufDescriptor->size = GRALLOC_ALIGN(bufDescriptor->size, 128);
				}
			}

			bufDescriptor->size *= bufDescriptor->layer_count;
		}
	}

	{
		/* Allocate ION backing store memory */
		err = mali_gralloc_ion_allocate(m, descriptors, numDescriptors, pHandle, &shared);

		if (err < 0)
		{
			ALOGE("mali_gralloc_ion_allocate return error");
			delete []req_wh;
			return err;
		}
	}

	if (shared)
	{
		backing_store_id = getUniqueId();
	}

	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)descriptors[i];
		private_handle_t *hnd = (private_handle_t *)pHandle[i];

		usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

		err = gralloc_buffer_attr_allocate(hnd);

		if (err < 0)
		{
			ALOGE("gralloc_buffer_attr_allocate return error");
			/* free all allocated ion buffer& attr buffer here.*/
			mali_gralloc_buffer_free_internal(pHandle, numDescriptors);
			delete []req_wh;
			return err;
		}

		mali_gralloc_dump_buffer_add(hnd);

		switch (usage & MALI_GRALLOC_USAGE_YUV_CONF_MASK)
		{
		case MALI_GRALLOC_USAGE_YUV_CONF_0:
			hnd->yuv_info = MALI_YUV_BT601_NARROW;
			break;

		case MALI_GRALLOC_USAGE_YUV_CONF_1:
			hnd->yuv_info = MALI_YUV_BT601_WIDE;
			break;

		case MALI_GRALLOC_USAGE_YUV_CONF_2:
			hnd->yuv_info = MALI_YUV_BT709_NARROW;
			break;

		case MALI_GRALLOC_USAGE_YUV_CONF_3:
			hnd->yuv_info = MALI_YUV_BT709_WIDE;
			break;
		}

		/* Workaround 10bit YUV only support BT709_WIDE in GPU DDK */
		if ((bufDescriptor->internal_format & MALI_GRALLOC_INTFMT_FMT_MASK) == MALI_GRALLOC_FORMAT_INTERNAL_Y0L2)
		{
			hnd->yuv_info = MALI_YUV_BT709_WIDE;
		}

		if (shared)
		{
			/*each buffer will share the same backing store id.*/
			hnd->backing_store_id = backing_store_id;
		}
		else
		{
			/* each buffer will have an unique backing store id.*/
			hnd->backing_store_id = getUniqueId();
		}

		hnd->req_width = req_wh[i] & 0xffff;
		hnd->req_height = req_wh[i] >> 16;
	}

	if (NULL != shared_backend)
	{
		*shared_backend = shared;
	}

	delete []req_wh;
	return 0;
}

int mali_gralloc_buffer_free(buffer_handle_t pHandle)
{
	int rval = -1;
	private_handle_t *hnd = (private_handle_t *)(pHandle);

	if (hnd != NULL)
	{
		rval = gralloc_buffer_attr_free(hnd);
		mali_gralloc_ion_free(hnd);
	}

	return rval;
}

static int mali_gralloc_buffer_free_internal(buffer_handle_t *pHandle, uint32_t num_hnds)
{
	int err = -1;
	uint32_t i = 0;

	for (i = 0; i < num_hnds; i++)
	{
		private_handle_t *hnd = (private_handle_t *)(pHandle[i]);

		err = gralloc_buffer_attr_free(hnd);
		mali_gralloc_ion_free(hnd);
	}

	return err;
}
