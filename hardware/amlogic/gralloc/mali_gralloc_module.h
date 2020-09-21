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
#ifndef MALI_GRALLOC_MODULE_H_
#define MALI_GRALLOC_MODULE_H_

//#include <system/window.h>
#include <linux/fb.h>
#include <pthread.h>

typedef enum
{
	MALI_DPY_TYPE_UNKNOWN = 0,
	MALI_DPY_TYPE_CLCD,
	MALI_DPY_TYPE_HDLCD
} mali_dpy_type;

#if GRALLOC_USE_GRALLOC1_API == 1
typedef struct
{
	struct hw_module_t common;
} gralloc_module_t;

/*
 * Most gralloc code is fairly version agnostic, but certain
 * places still use old usage defines. Make sure it works
 * ok for usages that are backwards compatible.
 */
#define GRALLOC_USAGE_PRIVATE_0 GRALLOC1_CONSUMER_USAGE_PRIVATE_0
#define GRALLOC_USAGE_PRIVATE_1 GRALLOC1_CONSUMER_USAGE_PRIVATE_1
#define GRALLOC_USAGE_PRIVATE_2 GRALLOC1_CONSUMER_USAGE_PRIVATE_2
#define GRALLOC_USAGE_PRIVATE_3 GRALLOC1_CONSUMER_USAGE_PRIVATE_3

#define GRALLOC_USAGE_SW_WRITE_RARELY GRALLOC1_PRODUCER_USAGE_CPU_WRITE
#define GRALLOC_USAGE_SW_WRITE_OFTEN GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN
#define GRALLOC_USAGE_SW_READ_RARELY GRALLOC1_CONSUMER_USAGE_CPU_READ
#define GRALLOC_USAGE_SW_READ_OFTEN GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN
#define GRALLOC_USAGE_HW_FB GRALLOC1_CONSUMER_USAGE_CLIENT_TARGET
#define GRALLOC_USAGE_HW_2D 0x00000400

#define GRALLOC_USAGE_SW_WRITE_MASK 0x000000F0
#define GRALLOC_USAGE_SW_READ_MASK 0x0000000F
#define GRALLOC_USAGE_PROTECTED GRALLOC1_PRODUCER_USAGE_PROTECTED
#define GRALLOC_USAGE_HW_RENDER GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET
#define GRALLOC_USAGE_HW_CAMERA_MASK (GRALLOC1_CONSUMER_USAGE_CAMERA | GRALLOC1_PRODUCER_USAGE_CAMERA)
#define GRALLOC_USAGE_HW_TEXTURE GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE
#define GRALLOC_USAGE_HW_VIDEO_ENCODER GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER
#define GRALLOC_USAGE_HW_COMPOSER GRALLOC1_CONSUMER_USAGE_HWCOMPOSER
#define GRALLOC_USAGE_EXTERNAL_DISP 0x00002000

#endif

struct private_module_t
{
	gralloc_module_t base;

	struct private_handle_t *framebuffer;
	uint32_t flags;
	uint32_t numBuffers;
	uint32_t bufferMask;
	pthread_mutex_t lock;
	buffer_handle_t currentBuffer;
	int ion_client;
	mali_dpy_type dpy_type;

	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo finfo;
	float xdpi;
	float ydpi;
	float fps;
	int swapInterval;

#ifdef __cplusplus
	/* Never intended to be used from C code */
	enum
	{
		// flag to indicate we'll post this buffer
		PRIV_USAGE_LOCKED_FOR_POST = 0x80000000
	};
#endif

#ifdef __cplusplus
	/* default constructor */
	private_module_t();
#endif
};
typedef struct private_module_t mali_gralloc_module;

#endif /* MALI_GRALLOC_MODULE_H_ */
