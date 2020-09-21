/*
 * (C) COPYRIGHT 2017 ARM Limited. All rights reserved.
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

#ifndef MALI_GRALLOC_USAGES_H_
#define MALI_GRALLOC_USAGES_H_

/*
 * Below usage types overlap, this is intentional.
 * The reason is that for Gralloc 0.3 there are very
 * few usage flags we have at our disposal.
 *
 * The overlapping is handled by processing the definitions
 * in a specific order.
 *
 * MALI_GRALLOC_USAGE_PRIVATE_FORMAT and MALI_GRALLOC_USAGE_NO_AFBC
 * don't overlap and are processed first.
 *
 * MALI_GRALLOC_USAGE_YUV_CONF are only for YUV formats and clients
 * using MALI_GRALLOC_USAGE_NO_AFBC must never allocate YUV formats.
 * The latter is strictly enforced and allocations will fail.
 *
 * MALI_GRALLOC_USAGE_AFBC_PADDING is only valid if MALI_GRALLOC_USAGE_NO_AFBC
 * is not present.
 */

/*
 * Gralloc private usage 0-3 are the same in 0.3 and 1.0.
 * We defined based our usages based on what is available.
 */
#if defined(GRALLOC_MODULE_API_VERSION_1_0)
typedef enum
{
	/* The client has specified a private format in the format parameter */
	MALI_GRALLOC_USAGE_PRIVATE_FORMAT = (int)GRALLOC1_PRODUCER_USAGE_PRIVATE_3,

	/* Buffer won't be allocated as AFBC */
	MALI_GRALLOC_USAGE_NO_AFBC = (int)(GRALLOC1_PRODUCER_USAGE_PRIVATE_1 | GRALLOC1_PRODUCER_USAGE_PRIVATE_2),

	/* Valid only for YUV allocations */
	MALI_GRALLOC_USAGE_YUV_CONF_0 = 0,
	MALI_GRALLOC_USAGE_YUV_CONF_1 = (int)GRALLOC1_PRODUCER_USAGE_PRIVATE_1,
	MALI_GRALLOC_USAGE_YUV_CONF_2 = (int)GRALLOC1_PRODUCER_USAGE_PRIVATE_0,
	MALI_GRALLOC_USAGE_YUV_CONF_3 = (int)(GRALLOC1_PRODUCER_USAGE_PRIVATE_0 | GRALLOC1_PRODUCER_USAGE_PRIVATE_1),
	MALI_GRALLOC_USAGE_YUV_CONF_MASK = MALI_GRALLOC_USAGE_YUV_CONF_3,

	/* A very specific alignment is requested on some buffers */
	MALI_GRALLOC_USAGE_AFBC_PADDING = (int)GRALLOC1_PRODUCER_USAGE_PRIVATE_2,

} mali_gralloc_usage_type;
#elif defined(GRALLOC_MODULE_API_VERSION_0_3)
typedef enum
{
	/* The client has specified a private format in the format parameter */
	MALI_GRALLOC_USAGE_PRIVATE_FORMAT = (int)GRALLOC_USAGE_PRIVATE_3,

	/* Buffer won't be allocated as AFBC */
	MALI_GRALLOC_USAGE_NO_AFBC = (int)(GRALLOC_USAGE_PRIVATE_1 | GRALLOC_USAGE_PRIVATE_2),

	/* Valid only for YUV allocations */
	MALI_GRALLOC_USAGE_YUV_CONF_0 = 0,
	MALI_GRALLOC_USAGE_YUV_CONF_1 = (int)GRALLOC_USAGE_PRIVATE_1,
	MALI_GRALLOC_USAGE_YUV_CONF_2 = (int)GRALLOC_USAGE_PRIVATE_0,
	MALI_GRALLOC_USAGE_YUV_CONF_3 = (int)(GRALLOC_USAGE_PRIVATE_0 | GRALLOC_USAGE_PRIVATE_1),
	MALI_GRALLOC_USAGE_YUV_CONF_MASK = MALI_GRALLOC_USAGE_YUV_CONF_3,

	/* A very specific alignment is requested on some buffers */
	MALI_GRALLOC_USAGE_AFBC_PADDING = GRALLOC_USAGE_PRIVATE_2,

} mali_gralloc_usage_type;
#else
#if defined(GRALLOC_LIBRARY_BUILD)
#error "Please include mali_gralloc_module.h before including other gralloc headers when building gralloc itself"
#else
#error "Please include either gralloc.h or gralloc1.h header before including gralloc_priv.h"
#endif
#endif

#endif /*MALI_GRALLOC_USAGES_H_*/
