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

#ifndef GRALLOC_HELPER_H_
#define GRALLOC_HELPER_H_

#include <sys/mman.h>
#include <android/log.h>

#ifndef AWAR
#define AWAR(fmt, args...) \
	__android_log_print(ANDROID_LOG_WARN, "gralloc", "%s:%d " fmt, __func__, __LINE__, ##args)
#endif
#ifndef AINF
#define AINF(fmt, args...) __android_log_print(ANDROID_LOG_INFO, "gralloc", fmt, ##args)
#endif
#ifndef AERR
#define AERR(fmt, args...) \
	__android_log_print(ANDROID_LOG_ERROR, "gralloc", "%s:%d " fmt, __func__, __LINE__, ##args)
#endif
#ifndef AERR_IF
#define AERR_IF(eq, fmt, args...) \
	if ((eq))                     \
	AERR(fmt, args)
#endif

#define GRALLOC_ALIGN(value, base) (((value) + ((base)-1)) & ~((base)-1))

#define GRALLOC_UNUSED(x) ((void)x)

static inline size_t round_up_to_page_size(size_t x)
{
	return (x + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}

#endif /* GRALLOC_HELPER_H_ */
