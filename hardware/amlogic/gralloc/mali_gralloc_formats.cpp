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
#include <dlfcn.h>
#include <inttypes.h>
#include <cutils/log.h>

#if PLATFORM_SDK_VERSION < 26
#include <fcntl.h>
#endif

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "gralloc_priv.h"
#include <cutils/properties.h>
#include <stdlib.h>
#define DEBUG_IO 0
#if DEBUG_IO
#include <linux/time.h>
#endif


static mali_gralloc_format_caps dpu_runtime_caps;
static mali_gralloc_format_caps vpu_runtime_caps;
static mali_gralloc_format_caps gpu_runtime_caps;
static mali_gralloc_format_caps cam_runtime_caps;
static pthread_mutex_t caps_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool runtime_caps_read = false;

#define MALI_GRALLOC_GPU_LIB_NAME "libGLES_mali.so"
#if defined(__LP64__)
#define MALI_GRALLOC_GPU_LIBRARY_PATH1 "/vendor/lib64/egl/"
#define MALI_GRALLOC_GPU_LIBRARY_PATH2 "/system/lib64/egl/"
#else
#define MALI_GRALLOC_GPU_LIBRARY_PATH1 "/vendor/lib/egl/"
#define MALI_GRALLOC_GPU_LIBRARY_PATH2 "/system/lib/egl/"
#endif

static bool get_block_capabilities(bool hal_module, const char *name, mali_gralloc_format_caps *block_caps)
{
	void *dso_handle = NULL;
	bool rval = false;

	/* Look for MALI_GRALLOC_FORMATCAPS_SYM_NAME_STR symbol in user-space drivers
	 * to determine hw format capabilities.
	 */
	if (!hal_module)
	{
		dso_handle = dlopen(name, RTLD_LAZY);
	}
	else
	{
		/* libhardware does some heuristics to find hal modules
		 * and then stores the dso handle internally. Use this.
		 */
		const struct hw_module_t *module = { NULL };

		if (hw_get_module(name, &module) >= 0)
		{
			dso_handle = module->dso;
		}
	}

	if (dso_handle)
	{
		void *sym = dlsym(dso_handle, MALI_GRALLOC_FORMATCAPS_SYM_NAME_STR);

		if (sym)
		{
			memcpy((void *)block_caps, sym, sizeof(mali_gralloc_format_caps));
			rval = true;
		}

		if (!hal_module)
		{
			dlclose(dso_handle);
		}
	}

	return rval;
}

static int map_flex_formats(uint64_t req_format)
{
	/* Map Android flexible formats to internal base formats */
	if (req_format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
	{
		req_format = MALI_GRALLOC_FORMAT_INTERNAL_NV12;
	}
	else if (req_format == HAL_PIXEL_FORMAT_YCbCr_420_888)
	{
		req_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
	}
	else if (req_format == HAL_PIXEL_FORMAT_YCbCr_422_888)
	{
		/* To be determined */
	}
	else if (req_format == HAL_PIXEL_FORMAT_YCbCr_444_888)
	{
		/* To be determined */
	}

	return req_format;
}

static bool is_afbc_supported(int req_format_mapped)
{
	bool rval = true;

	/* These base formats we currently don't support with compression */
	switch (req_format_mapped)
	{
	case MALI_GRALLOC_FORMAT_INTERNAL_RAW16:
	case MALI_GRALLOC_FORMAT_INTERNAL_RAW12:
	case MALI_GRALLOC_FORMAT_INTERNAL_RAW10:
	case MALI_GRALLOC_FORMAT_INTERNAL_BLOB:
	case MALI_GRALLOC_FORMAT_INTERNAL_P010:
	case MALI_GRALLOC_FORMAT_INTERNAL_P210:
	case MALI_GRALLOC_FORMAT_INTERNAL_Y410:
#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616:
#endif
	case HAL_PIXEL_FORMAT_YCbCr_422_I:
		rval = false;
		break;
	}

	return rval;
}

static bool is_android_yuv_format(int req_format)
{
	bool rval = false;

	switch (req_format)
	{
	case HAL_PIXEL_FORMAT_YV12:
	case HAL_PIXEL_FORMAT_Y8:
	case HAL_PIXEL_FORMAT_Y16:
	case HAL_PIXEL_FORMAT_YCbCr_420_888:
	case HAL_PIXEL_FORMAT_YCbCr_422_888:
	case HAL_PIXEL_FORMAT_YCbCr_444_888:
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
	case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
	case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
		rval = true;
		break;
	}

	return rval;
}

static bool is_afbc_format(uint64_t internal_format)
{
	return (internal_format & MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK) != 0;
}

static void apply_gpu_producer_limitations(int req_format, uint64_t *producer_runtime_mask)
{
	char gpu_afbc[PROPERTY_VALUE_MAX];
	char prop[PROPERTY_VALUE_MAX];
	int gpu_afbc_limit = 0;
	if (is_android_yuv_format(req_format))
	{
		if (gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_NOWRITE)
		{
			*producer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		}
		else
		{
			/* All GPUs that can write YUV AFBC can only do it in 16x16, optionally with tiled */
			*producer_runtime_mask &=
				~(MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK | MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK);
		}
	}
	if (property_get("media.afbc.limit", gpu_afbc, "0") > 0)
	{
		gpu_afbc_limit = atoi(gpu_afbc);
	}
	if (property_get("ro.bootimage.build.fingerprint", prop, NULL) > 0)
	{
		if (strstr(prop, "generic"))
		{
			gpu_afbc_limit = 1;
		}
	}
	if (gpu_afbc_limit)
	{
		ALOGD("Gralloc Deubg: media.afbc.limit is set to 1");
		*producer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	}
}

static void apply_video_consumer_limitations(int req_format, uint64_t *consumer_runtime_mask)
{
	if (is_android_yuv_format(req_format))
	{
		if (vpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_NOREAD)
		{
			*consumer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		}
	}
	else
	{
		*consumer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	}
}

/*
 * NOTE: this code assumes that all layers that don't have AFBC disabled are pre-rotated.
 * */
static void apply_display_consumer_limitations(int req_format, int buffer_size, uint64_t *display_consumer_runtime_mask)
{
	/* Disable AFBC based on buffer dimensions */
	bool afbc_allowed = false;

	(void)buffer_size;

#if MALI_DISPLAY_VERSION == 550 || MALI_DISPLAY_VERSION == 650
#if GRALLOC_DISP_W != 0 && GRALLOC_DISP_H != 0
	afbc_allowed = ((buffer_size * 100) / (GRALLOC_DISP_W * GRALLOC_DISP_H)) >= GRALLOC_AFBC_MIN_SIZE;

#else
	/* If display size is not valid then always allow AFBC */
	afbc_allowed = true;

#endif
#else
	/* For cetus, always allow AFBC */
	afbc_allowed = true;
#endif

	if (!afbc_allowed)
	{
		*display_consumer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	}
	else if (is_android_yuv_format(req_format))
	{
		/* YUV formats don't support split or wide block. */
		*display_consumer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
		*display_consumer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
	}
	else
	{
		/* Some RGB formats don't support split block. */
		switch(req_format)
		{
		case MALI_GRALLOC_FORMAT_INTERNAL_RGB_565:
			*display_consumer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
			break;
		}
	}
}

static uint64_t determine_best_format(int req_format, mali_gralloc_producer_type producer,
                                      mali_gralloc_consumer_type consumer, uint64_t producer_runtime_mask,
                                      uint64_t consumer_runtime_mask)
{
	/* Default is to return the requested format */
	uint64_t internal_format = req_format;
	uint64_t dpu_mask = dpu_runtime_caps.caps_mask;
	uint64_t gpu_mask = gpu_runtime_caps.caps_mask;
	uint64_t vpu_mask = vpu_runtime_caps.caps_mask;
	uint64_t cam_mask = cam_runtime_caps.caps_mask;

	if (producer == MALI_GRALLOC_PRODUCER_GPU &&
	    gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		gpu_mask &= producer_runtime_mask;
		if (consumer == MALI_GRALLOC_CONSUMER_GPU_OR_DISPLAY)
		{
			gpu_mask &= consumer_runtime_mask;
			dpu_mask &= consumer_runtime_mask;

			if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC &&
				dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC)
			{
				internal_format |= MALI_GRALLOC_INTFMT_AFBC_BASIC;
			}

			/* For pre-Cetus displays split block will be selected without wide block
			 * as this is preferred. For Cetus, wide block and split block will be enabled
			 * together. When in future, wide block is disabled for layers that may
			 * not be pre-rotated, split block should also be disabled. */
			if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK &&
			    dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK)
			{
				internal_format |= MALI_GRALLOC_INTFMT_AFBC_SPLITBLK;
			}

			if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK &&
			    dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK)
			{
				internal_format |= MALI_GRALLOC_INTFMT_AFBC_WIDEBLK;
			}

			if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS &&
				dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS)
			{
				internal_format |= MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;
			}

#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
			/*
			 * Ensure requested format is supported by producer/consumer.
			 * GPU composition must always be supported in case of fallback from DPU.
			 * It is therefore not necessary to enforce DPU support.
			 */
			if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 &&
			    (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0)
			{
				internal_format = 0;
			}
			else if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 &&
			         (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0)
			{
				internal_format = 0;
			}
#endif
		}
		else if (consumer == MALI_GRALLOC_CONSUMER_GPU_EXCL)
		{
			gpu_mask &= consumer_runtime_mask;

			/* When GPU acts as both producer and consumer it prefers 16x16 superblocks */
			if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC)
			{
				internal_format |= MALI_GRALLOC_INTFMT_AFBC_BASIC;
			}

			if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS)
			{
				internal_format |= MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;
			}

#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
			/* Reject unsupported formats */
			if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 &&
			    (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0)
			{
				internal_format = 0;
			}
			else if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 &&
			         (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0)
			{
				internal_format = 0;
			}
#endif
		}
		else if (consumer == MALI_GRALLOC_CONSUMER_VIDEO_ENCODER)
		{
			vpu_mask &= consumer_runtime_mask;

			if (internal_format == HAL_PIXEL_FORMAT_YV12 || internal_format == MALI_GRALLOC_FORMAT_INTERNAL_NV12)
			{
				if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC &&
				    vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC)
				{
					internal_format |= MALI_GRALLOC_INTFMT_AFBC_BASIC;
				}

				if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS &&
				    vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS)
				{
					internal_format |= MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;
				}
			}

#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
			/* Reject unsupported formats */
			if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 &&
			    ((gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0 ||
			     (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0))
			{
				internal_format = 0;
			}
			else if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 &&
			         ((gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0 ||
			          (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0))
			{
				internal_format = 0;
			}
#endif
		}
	}
	else if (producer == MALI_GRALLOC_PRODUCER_VIDEO_DECODER &&
	         vpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		vpu_mask &= producer_runtime_mask;

		if (consumer == MALI_GRALLOC_CONSUMER_GPU_OR_DISPLAY)
		{
			gpu_mask &= consumer_runtime_mask;
			dpu_mask &= consumer_runtime_mask;

			if (internal_format == HAL_PIXEL_FORMAT_YV12)
			{
				if (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC &&
				    gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC &&
				    dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC)
				{
					internal_format |= MALI_GRALLOC_INTFMT_AFBC_BASIC;
				}

				if (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS &&
				    gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS &&
				    dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS)
				{
					internal_format |= MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;
				}
			}

#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
			/*
			 * Ensure requested format is supported by producer/consumer.
			 * GPU composition must always be supported in case of fallback from DPU.
			 * It is therefore not necessary to enforce DPU support.
			*/
			if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 &&
			    ((vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0 ||
			     (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0))
			{
				internal_format = 0;
			}
			else if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 &&
			         ((vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0 ||
			          (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0 ))
			{
				internal_format = 0;
			}
#endif
		}
		else if (consumer == MALI_GRALLOC_CONSUMER_GPU_EXCL)
		{
			gpu_mask &= consumer_runtime_mask;

			if (internal_format == HAL_PIXEL_FORMAT_YV12)
			{
				if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC &&
				    vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC)
				{
					internal_format |= MALI_GRALLOC_INTFMT_AFBC_BASIC;
				}

				if (gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS &&
				    vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS)
				{
					internal_format |= MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;
				}
			}

#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
			/* Reject unsupported formats */
			if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 &&
			    ((gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0 ||
			     (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0))
			{
				internal_format = 0;
			}
			else if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 &&
			         ((gpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0 ||
			          (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0))
			{
				internal_format = 0;
			}
#endif
		}
		else if (consumer == MALI_GRALLOC_CONSUMER_VIDEO_ENCODER)
		{
			/* Fall-through. To be decided.*/

#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
			/* Reject unsupported formats */
			if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102 &&
			    (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102) == 0)
			{
				internal_format = 0;
			}
			else if (req_format == MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616 &&
			         (vpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616) == 0)
			{
				internal_format = 0;
			}
#endif
		}
	}
	else if (producer == MALI_GRALLOC_PRODUCER_CAMERA &&
	         cam_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		if (consumer == MALI_GRALLOC_CONSUMER_GPU_OR_DISPLAY)
		{
			/* Fall-through. To be decided.*/
		}
		else if (consumer == MALI_GRALLOC_CONSUMER_GPU_EXCL)
		{
			/* Fall-through. To be decided.*/
		}
		else if (consumer == MALI_GRALLOC_CONSUMER_VIDEO_ENCODER)
		{
			/* Fall-through. To be decided.*/
		}
	}
	else if (producer == MALI_GRALLOC_PRODUCER_DISPLAY_AEU &&
			 consumer == MALI_GRALLOC_CONSUMER_DISPLAY_EXCL &&
			 dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		dpu_mask &= producer_runtime_mask;
		dpu_mask &= consumer_runtime_mask;

		if (dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC)
		{
			internal_format |= MALI_GRALLOC_INTFMT_AFBC_BASIC;
			if (dpu_mask & MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS)
			{
				internal_format |= MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS;
			}
		}
	}
	/* For MALI_GRALLOC_PRODUCER_DISPLAY or MALI_GRALLOC_PRODUCER_GPU_OR_DISPLAY
	 * the requested format without AFBC is used, so no extra flags need to be set.
	 */

	return internal_format;
}

static uint64_t decode_internal_format(uint64_t req_format, mali_gralloc_format_type type)
{
	uint64_t internal_format, me_mask, base_format, mapped_base_format;

	if (type == MALI_GRALLOC_FORMAT_TYPE_USAGE)
	{
		internal_format = GRALLOC_PRIVATE_FORMAT_UNWRAP((int)req_format);
	}
	else if (type == MALI_GRALLOC_FORMAT_TYPE_INTERNAL)
	{
		internal_format = req_format;
	}
	else
	{
		internal_format = 0;
		goto out;
	}

	base_format = internal_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	/* Even though private format allocations are intended to be for specific
	 * formats, certain test cases uses the flexible formats that needs to be mapped
	 * to internal ones.
	 */
	mapped_base_format = map_flex_formats((uint32_t)base_format);

	/* Validate the internal base format passed in */
	switch (mapped_base_format)
	{
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_8888:
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBX_8888:
	case MALI_GRALLOC_FORMAT_INTERNAL_RGB_888:
	case MALI_GRALLOC_FORMAT_INTERNAL_RGB_565:
	case MALI_GRALLOC_FORMAT_INTERNAL_BGRA_8888:
#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_1010102:
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616:
#endif
	case MALI_GRALLOC_FORMAT_INTERNAL_YV12:
	case MALI_GRALLOC_FORMAT_INTERNAL_Y8:
	case MALI_GRALLOC_FORMAT_INTERNAL_Y16:
	case MALI_GRALLOC_FORMAT_INTERNAL_RAW16:
	case MALI_GRALLOC_FORMAT_INTERNAL_RAW12:
	case MALI_GRALLOC_FORMAT_INTERNAL_RAW10:
	case MALI_GRALLOC_FORMAT_INTERNAL_BLOB:
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
	case HAL_PIXEL_FORMAT_YCbCr_420_888:
	case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
	case MALI_GRALLOC_FORMAT_INTERNAL_NV21:
	case MALI_GRALLOC_FORMAT_INTERNAL_YUV422_8BIT:
	case MALI_GRALLOC_FORMAT_INTERNAL_Y0L2:
	case MALI_GRALLOC_FORMAT_INTERNAL_P010:
	case MALI_GRALLOC_FORMAT_INTERNAL_P210:
	case MALI_GRALLOC_FORMAT_INTERNAL_Y210:
	case MALI_GRALLOC_FORMAT_INTERNAL_Y410:
		if (mapped_base_format != base_format)
		{
			internal_format = (internal_format & MALI_GRALLOC_INTFMT_EXT_MASK) | mapped_base_format;
		}

		break;

	default:
		ALOGE("Internal base format requested is unrecognized: %" PRIx64, internal_format);
		internal_format = 0;
		break;
	}

out:
	return internal_format;
}

static bool determine_producer(mali_gralloc_producer_type *producer, int usage)
{
	bool rval = true;

	/* Default to GPU */
	*producer = MALI_GRALLOC_PRODUCER_GPU;

	if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
	{
		*producer = MALI_GRALLOC_PRODUCER_CPU;
		rval = false;
	}
	/* This is a specific case where GRALLOC_USAGE_HW_COMPOSER can indicate display as a producer.
	 * This is the case because video encoder is assumed to be the only consumer. */
	else if ((usage & (GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER)) ==
	         (GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER))
	{
		*producer = MALI_GRALLOC_PRODUCER_GPU_OR_DISPLAY;
	}
	else if (usage & GRALLOC_USAGE_HW_RENDER)
	{
		*producer = MALI_GRALLOC_PRODUCER_GPU;
	}
	else if (usage & GRALLOC_USAGE_HW_CAMERA_MASK)
	{
		*producer = MALI_GRALLOC_PRODUCER_CAMERA;
	}
	/* HW_TEXTURE+HW_COMPOSER+EXTERNAL_DISP is a definition set by
	 * stagefright for "video decoder". We check for it here.
	 */
	else if ((usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_EXTERNAL_DISP)) ==
	         (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_EXTERNAL_DISP))
	{
		*producer = MALI_GRALLOC_PRODUCER_VIDEO_DECODER;
	}
	else if ((usage & (GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER)) ==
	         (GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER))
	{
		*producer = MALI_GRALLOC_PRODUCER_DISPLAY;
	}
	else if (usage == GRALLOC_USAGE_HW_COMPOSER)
	{
		*producer = MALI_GRALLOC_PRODUCER_GPU_OR_DISPLAY;
	}
	return rval;
}

static bool determine_consumer(mali_gralloc_consumer_type *consumer, int usage)
{
	bool rval = true;

	/* Default to GPU */
	*consumer = MALI_GRALLOC_CONSUMER_GPU_EXCL;

	if (usage & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
	{
		*consumer = MALI_GRALLOC_CONSUMER_CPU;
		rval = false;
	}
	/* When usage explicitly targets a consumer, as it does with GRALLOC_USAGE_HW_FB,
	 * we pick DPU even if there are no runtime capabilities present.
	 */
	else if (usage & GRALLOC_USAGE_HW_FB)
	{
		*consumer = MALI_GRALLOC_CONSUMER_GPU_OR_DISPLAY;
	}
	else if (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)
	{
		*consumer = MALI_GRALLOC_CONSUMER_VIDEO_ENCODER;
	}
	/* GRALLOC_USAGE_HW_COMPOSER is by default applied by SurfaceFlinger so we can't exclusively rely on it
	 * to determine consumer. When a buffer is targeted for either we reject the DPU when it lacks
	 * runtime capabilities, in favor of the more capable GPU.
	 */
	else if ((usage & (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER)) ==
	             (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_COMPOSER) &&
	         dpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT)
	{
		*consumer = MALI_GRALLOC_CONSUMER_GPU_OR_DISPLAY;
	}
	else if (usage & GRALLOC_USAGE_HW_TEXTURE)
	{
		*consumer = MALI_GRALLOC_CONSUMER_CPU;
	}
	else if (usage == GRALLOC_USAGE_HW_COMPOSER)
	{
		*consumer = MALI_GRALLOC_CONSUMER_DISPLAY_EXCL;
	}

	return rval;
}

int get_meson_dpu_gpu_caps()
{
	char osd_afbcd[PROPERTY_VALUE_MAX];
	int  osd_afbcd_enabled = 0;
	int  num_of_afbcd_type = 0;
	char osd_afbcd_value[] = "00";
	int  osd_afbcd_fd = -1;

	if (property_get("vendor.afbcd.enable", osd_afbcd, "1") > 0)
	{
		osd_afbcd_enabled = atoi(osd_afbcd);
	}

	if (osd_afbcd_enabled) {
		osd_afbcd_fd = open("/sys/class/graphics/fb0/osd_afbcd", O_RDWR);
		if (osd_afbcd_fd < 0) {
			ALOGD("errno=%d, %s", errno, strerror(errno));
			return -EPERM;
		}

		lseek(osd_afbcd_fd, 0, SEEK_SET);
		read (osd_afbcd_fd, osd_afbcd, 1);
		num_of_afbcd_type = atoi(osd_afbcd);
		close (osd_afbcd_fd);
		osd_afbcd_fd = -1;
	}
	ALOGD("AFBC %s\n", num_of_afbcd_type != 0? "enabled":"disabled");
	ALOGD("num of AFBC type %d\n", num_of_afbcd_type);

    if (0 != num_of_afbcd_type) {
        /* Determine DPU format capabilities */
        dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
        dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
        dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;

        /* Determine GPU format capabilities */
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_NOWRITE;
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
#if PLATFORM_SDK_VERSION >= 26 && GPU_FORMAT_LIMIT != 1
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102;
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616;
#endif
    } else {
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_NOWRITE;
    }
    if (2 == num_of_afbcd_type) {
        dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
        gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
    }
    return 0;
}

/*
 * Here we determine format capabilities for the 4 IPs we support.
 * For now these are controlled by build defines, but in the future
 * they should be read out from each user-space driver.
 */
static void determine_format_capabilities()
{
	/* Loading libraries can take some time and
	 * we may see many allocations at boot.
	 */
#if DEBUG_IO
	struct timeval time_IO1, time_IO2;
	int bw_IO;
#endif
	pthread_mutex_lock(&caps_init_mutex);

	if (runtime_caps_read)
	{
		goto already_init;
	}
	memset((void *)&dpu_runtime_caps, 0, sizeof(dpu_runtime_caps));
	memset((void *)&vpu_runtime_caps, 0, sizeof(vpu_runtime_caps));
	memset((void *)&gpu_runtime_caps, 0, sizeof(gpu_runtime_caps));
	memset((void *)&cam_runtime_caps, 0, sizeof(cam_runtime_caps));

	/* Determine DPU format capabilities */
	if (!get_block_capabilities(true, "hwcomposer", &dpu_runtime_caps))
	{
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_NOWRITE;
#if MALI_DISPLAY_VERSION >= 500
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;

#if MALI_DISPLAY_VERSION >= 550
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
#endif
#endif
#if MALI_DISPLAY_VERSION == 71
		/* Cetus adds support for wide block and tiled headers. */
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS;
		dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK_YUV_DISABLE;
#endif
	}

	/* Determine GPU format capabilities in Gralloc
	 * There's no need to load libGLES_mali.so
	 */
/*	if (access(MALI_GRALLOC_GPU_LIBRARY_PATH1 MALI_GRALLOC_GPU_LIB_NAME, R_OK) == 0)
	{
		get_block_capabilities(false, MALI_GRALLOC_GPU_LIBRARY_PATH1 MALI_GRALLOC_GPU_LIB_NAME, &gpu_runtime_caps);
	}
	else if (access(MALI_GRALLOC_GPU_LIBRARY_PATH2 MALI_GRALLOC_GPU_LIB_NAME, R_OK) == 0)
	{
		get_block_capabilities(false, MALI_GRALLOC_GPU_LIBRARY_PATH2 MALI_GRALLOC_GPU_LIB_NAME, &gpu_runtime_caps);
	}
*/
	if ((gpu_runtime_caps.caps_mask & MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT) == 0)
	{

#if MALI_GPU_SUPPORT_AFBC_BASIC == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;

		/* Need to verify when to remove this */
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_NOWRITE;
#endif /* MALI_GPU_SUPPORT_AFBC_BASIC == 1 */
#if MALI_GPU_SUPPORT_AFBC_SPLITBLK == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
#endif

#if MALI_SUPPORT_AFBC_WIDEBLK == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
#endif

#if MALI_USE_YUV_AFBC_WIDEBLK != 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK_YUV_DISABLE;
#endif

#if MALI_SUPPORT_AFBC_TILED_HEADERS == 1
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_SPLITBLK;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_WIDEBLK;
		gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS;
#endif

	}

/* Determine VPU format capabilities */
#if MALI_VIDEO_VERSION == 500 || MALI_VIDEO_VERSION == 550
		vpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
		vpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
		vpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_YUV_NOREAD;
#endif

#if MALI_VIDEO_VERSION == 61
		vpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
		vpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
		vpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_TILED_HEADERS;
#endif

/* Build specific capability changes */
#if GRALLOC_ARM_NO_EXTERNAL_AFBC == 1
	{
		dpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		gpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		vpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
		cam_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	}
#endif

#if DEBUG_IO
	gettimeofday(&time_IO1, NULL);
#endif
	get_meson_dpu_gpu_caps();
#if DEBUG_IO
	gettimeofday(&time_IO2, NULL);
	bw_IO = time_IO2.tv_sec - time_IO1.tv_sec;
	bw_IO = bw_IO*1000000 + time_IO2.tv_usec -time_IO1.tv_usec;
	ALOGD("get_meson_dpu_gpu_caps spend %d us\n", bw_IO);
#endif

	runtime_caps_read = true;

already_init:
	pthread_mutex_unlock(&caps_init_mutex);

	ALOGE("GPU format capabilities 0x%" PRIx64, gpu_runtime_caps.caps_mask);
	ALOGE("DPU format capabilities 0x%" PRIx64, dpu_runtime_caps.caps_mask);
	ALOGE("VPU format capabilities 0x%" PRIx64, vpu_runtime_caps.caps_mask);
	ALOGE("CAM format capabilities 0x%" PRIx64, cam_runtime_caps.caps_mask);
}

uint64_t mali_gralloc_select_format(uint64_t req_format, mali_gralloc_format_type type, uint64_t usage, int buffer_size)
{
	uint64_t internal_format = 0;
	mali_gralloc_consumer_type consumer = MALI_GRALLOC_CONSUMER_CPU;
	mali_gralloc_producer_type producer = MALI_GRALLOC_PRODUCER_CPU;
	uint64_t producer_runtime_mask = ~(0ULL);
	uint64_t consumer_runtime_mask = ~(0ULL);
	uint64_t req_format_mapped = 0;

	if (!runtime_caps_read)
	{
		/*
		 * It is better to initialize these when needed because
		 * not all processes allocates memory.
		 */
		determine_format_capabilities();
	}

	/* A unique usage specifies that an internal format is in req_format */
	if (usage & MALI_GRALLOC_USAGE_PRIVATE_FORMAT || type == MALI_GRALLOC_FORMAT_TYPE_INTERNAL)
	{
		internal_format = decode_internal_format(req_format, type);
		goto out;
	}
	/* Re-map special Android formats */
	req_format_mapped = map_flex_formats(req_format);

	/* Determine producer/consumer */
	if (!determine_producer(&producer, usage) ||
	    !determine_consumer(&consumer, usage))
	{
		/* Failing to determine producer/consumer usually means
		 * client has requested sw rendering.
		 */
		internal_format = req_format_mapped;
		goto out;
	}

	/*
	 * Determine runtime capability limitations
	 */

	/* Disable AFBC based on unique usage */
	if ((usage & MALI_GRALLOC_USAGE_NO_AFBC) == MALI_GRALLOC_USAGE_NO_AFBC)
	{
		if (is_android_yuv_format(req_format_mapped))
		{
			ALOGE("It is invalid to specify NO_AFBC usage flags when allocating YUV formats.\
                   Requested fmt: 0x%" PRIx64 " Re-Mapped fmt: 0x%" PRIx64,
			      req_format, req_format_mapped);
			internal_format = 0;
			goto out;
		}

		producer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	}
	else if (!is_afbc_supported(req_format_mapped))
	{
		producer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	}
	else if ((usage & GRALLOC_USAGE_HW_FB) == GRALLOC_USAGE_HW_FB)
	{
#if PRIMARY_DISP_SUPPORT_AFBC == false
		if ((usage & GRALLOC_USAGE_EXTERNAL_DISP) == 0)
			producer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
#endif
#if EXTEND_DISP_SUPPORT_AFBC == false
		if ((usage & GRALLOC_USAGE_EXTERNAL_DISP) == GRALLOC_USAGE_EXTERNAL_DISP)
			producer_runtime_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
#endif
	}
	else
	{
		/* Check producer limitations and modify runtime mask.*/
		if (producer == MALI_GRALLOC_PRODUCER_GPU || producer == MALI_GRALLOC_PRODUCER_GPU_OR_DISPLAY)
		{
			apply_gpu_producer_limitations(req_format, &producer_runtime_mask);
		}

		/* Check consumer limitations and modify runtime mask. */
		if (consumer == MALI_GRALLOC_CONSUMER_VIDEO_ENCODER)
		{
			apply_video_consumer_limitations(req_format, &consumer_runtime_mask);
		}
		else if (consumer == MALI_GRALLOC_CONSUMER_GPU_OR_DISPLAY ||
		         consumer == MALI_GRALLOC_CONSUMER_DISPLAY_EXCL)
		{
			apply_display_consumer_limitations(req_format, buffer_size, &consumer_runtime_mask);
		}
	}

	/* Automatically select format in case producer/consumer identified */
	internal_format =
	    determine_best_format(req_format_mapped, producer, consumer, producer_runtime_mask, consumer_runtime_mask);
out:
	ALOGV("mali_gralloc_select_format: req_format=0x%08" PRIx64 " req_fmt_mapped=0x%" PRIx64
	      " internal_format=0x%" PRIx64 " usage=0x%" PRIx64,
	      req_format, req_format_mapped, internal_format, usage);

	return internal_format;
}

extern "C" {
void mali_gralloc_get_gpu_caps(struct mali_gralloc_format_caps *gpu_caps)
{
	if (gpu_caps != NULL)
	{
		if (!runtime_caps_read)
		{
			determine_format_capabilities();
		}

		memcpy(gpu_caps, (void *)&gpu_runtime_caps, sizeof(struct mali_gralloc_format_caps));
	}
}
}
