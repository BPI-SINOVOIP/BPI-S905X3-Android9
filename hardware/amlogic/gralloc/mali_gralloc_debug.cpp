/*
 * Copyright (C) 2016 ARM Limited. All rights reserved.
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

#include <inttypes.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

#if GRALLOC_USE_GRALLOC1_API == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif
#include <hardware/hardware.h>

#include "mali_gralloc_module.h"
#include "gralloc_priv.h"
#include "mali_gralloc_debug.h"

static pthread_mutex_t dump_lock = PTHREAD_MUTEX_INITIALIZER;
static std::vector<private_handle_t *> dump_buffers;
static android::String8 dumpStrings;

void mali_gralloc_dump_buffer_add(private_handle_t *handle)
{
	if (NULL == handle)
	{
		ALOGE("Invalid handle %p and return", handle);
		return;
	}

	pthread_mutex_lock(&dump_lock);
	dump_buffers.push_back(handle);
	pthread_mutex_unlock(&dump_lock);
}

void mali_gralloc_dump_buffer_erase(private_handle_t *handle)
{
	if (NULL == handle)
	{
		ALOGE("Invalid handle %p and return", handle);
		return;
	}

	pthread_mutex_lock(&dump_lock);
	dump_buffers.erase(std::remove(dump_buffers.begin(), dump_buffers.end(), handle));
	pthread_mutex_unlock(&dump_lock);
}

void mali_gralloc_dump_string(android::String8 &buf, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	buf.appendFormatV(fmt, args);
	va_end(args);
}

void mali_gralloc_dump_buffers(android::String8 &dumpStrings, uint32_t *outSize)
{
	if (NULL == outSize)
	{
		ALOGE("Invalid pointer to dump buffer size and return");
		return;
	}

	dumpStrings.clear();
	mali_gralloc_dump_string(dumpStrings,
	                         "-------------------------Start to dump Gralloc buffers info------------------------\n");
	private_handle_t *hnd;
	size_t num;

	mali_gralloc_dump_string(dumpStrings, "    handle  | width | height | stride |   req format   |internal "
	                                      "format|consumer usage|producer usage| shared fd | AFBC "
	                                      "|\n");
	mali_gralloc_dump_string(dumpStrings, "------------+-------+--------+--------+----------------+---------------+----"
	                                      "----------+--------------+-----------+------+\n");
	pthread_mutex_lock(&dump_lock);

	for (num = 0; num < dump_buffers.size(); num++)
	{
		hnd = dump_buffers[num];
		mali_gralloc_dump_string(dumpStrings, " %08" PRIxPTR " | %5d |  %5d |  %5d |    %08x    |    %09" PRIx64
		                                      "  |   %09" PRIx64 "  |   %09" PRIx64 "  |  %08x | %4d |\n",
		                         hnd, hnd->width, hnd->height, hnd->stride, hnd->req_format, hnd->internal_format,
		                         hnd->consumer_usage, hnd->producer_usage, hnd->share_fd,
		                         (hnd->internal_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK) ? true : false);
	}

	pthread_mutex_unlock(&dump_lock);
	mali_gralloc_dump_string(
	    dumpStrings, "---------------------End dump Gralloc buffers info with num %zu----------------------\n", num);

	*outSize = dumpStrings.size();
}

void mali_gralloc_dump_internal(uint32_t *outSize, char *outBuffer)
{
	uint32_t dumpSize;

	if (NULL == outSize)
	{
		ALOGE("Invalid pointer to dump buffer size and return");
		return;
	}

	if (NULL == outBuffer)
	{
		if (!dumpStrings.isEmpty())
		{
			dumpStrings.clear();
		}

		mali_gralloc_dump_buffers(dumpStrings, outSize);
	}
	else
	{
		if (dumpStrings.isEmpty())
		{
			*outSize = 0;
		}
		else
		{
			dumpSize = dumpStrings.size();
			*outSize = (dumpSize < *outSize) ? dumpSize : *outSize;
			memcpy(outBuffer, dumpStrings.string(), *outSize);
		}
	}
}
