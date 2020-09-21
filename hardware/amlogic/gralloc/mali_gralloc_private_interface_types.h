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

#ifndef MALI_GRALLOC_PRIVATE_INTERFACE_TYPES_H_
#define MALI_GRALLOC_PRIVATE_INTERFACE_TYPES_H_

enum
{
	/* CROP_RECT and YUV_TRANS are intended to be
	 * written by producers and read by consumers.
	 * A producer should write these parameters before
	 * it queues a buffer to the consumer.
	 */

	/* CROP RECT, defined as an int array of top, left, height, width. Origin in top-left corner */
	GRALLOC_ARM_BUFFER_ATTR_CROP_RECT = 1,

	/* Set if the AFBC format used a YUV transform before compressing */
	GRALLOC_ARM_BUFFER_ATTR_AFBC_YUV_TRANS = 2,

	/* Set if the AFBC format uses sparse allocation */
	GRALLOC_ARM_BUFFER_ATTR_AFBC_SPARSE_ALLOC = 3,

	/* Amlogic: set tunnel index for omx video */
        GRALLOC_ARM_BUFFER_ATTR_AM_OMX_TUNNEL = 4,

	GRALLOC_ARM_BUFFER_ATTR_LAST
};

typedef uint32_t buf_attr;

typedef enum
{
	MALI_YUV_NO_INFO,
	MALI_YUV_BT601_NARROW,
	MALI_YUV_BT601_WIDE,
	MALI_YUV_BT709_NARROW,
	MALI_YUV_BT709_WIDE
} mali_gralloc_yuv_info;

#endif /* MALI_GRALLOC_PRIVATE_INTERFACE_TYPES_H_ */
