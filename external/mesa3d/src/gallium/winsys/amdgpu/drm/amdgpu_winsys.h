/*
 * Copyright © 2009 Corbin Simpson
 * Copyright © 2015 Advanced Micro Devices, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Marek Olšák <maraeo@gmail.com>
 */

#ifndef AMDGPU_WINSYS_H
#define AMDGPU_WINSYS_H

#include "pipebuffer/pb_cache.h"
#include "pipebuffer/pb_slab.h"
#include "gallium/drivers/radeon/radeon_winsys.h"
#include "addrlib/addrinterface.h"
#include "util/u_queue.h"
#include <amdgpu.h>

struct amdgpu_cs;

#define AMDGPU_SLAB_MIN_SIZE_LOG2 9
#define AMDGPU_SLAB_MAX_SIZE_LOG2 14

struct amdgpu_winsys {
   struct radeon_winsys base;
   struct pipe_reference reference;
   struct pb_cache bo_cache;
   struct pb_slabs bo_slabs;

   amdgpu_device_handle dev;

   pipe_mutex bo_fence_lock;

   int num_cs; /* The number of command streams created. */
   uint32_t next_bo_unique_id;
   uint64_t allocated_vram;
   uint64_t allocated_gtt;
   uint64_t mapped_vram;
   uint64_t mapped_gtt;
   uint64_t buffer_wait_time; /* time spent in buffer_wait in ns */
   uint64_t num_gfx_IBs;
   uint64_t num_sdma_IBs;

   struct radeon_info info;

   /* multithreaded IB submission */
   struct util_queue cs_queue;

   struct amdgpu_gpu_info amdinfo;
   ADDR_HANDLE addrlib;
   uint32_t rev_id;
   unsigned family;

   bool check_vm;

   /* List of all allocated buffers */
   pipe_mutex global_bo_list_lock;
   struct list_head global_bo_list;
   unsigned num_buffers;
};

static inline struct amdgpu_winsys *
amdgpu_winsys(struct radeon_winsys *base)
{
   return (struct amdgpu_winsys*)base;
}

void amdgpu_surface_init_functions(struct amdgpu_winsys *ws);
ADDR_HANDLE amdgpu_addr_create(struct amdgpu_winsys *ws);

#endif
