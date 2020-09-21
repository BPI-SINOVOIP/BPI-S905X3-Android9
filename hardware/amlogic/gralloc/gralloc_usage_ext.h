/*
 *  gralloc_usage_ext.h
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2018 Amlogic, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#ifndef GRALLOC_USAGE_EXT_H
#define GRALLOC_USAGE_EXT_H

/* gralloc usage extensions */
enum {
	GRALLOC_USAGE_AML_SECURE            = 0x00400000,
	GRALLOC_USAGE_AML_VIDEO_OVERLAY     = 0x01000000,
	GRALLOC_USAGE_AML_DMA_BUFFER        = 0x02000000,
	GRALLOC_USAGE_AML_OMX_OVERLAY       = 0x04000000,
};

#endif /* GRALLOC_USAGE_EXT_H */
