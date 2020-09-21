/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef AMDRM_UTILS_H
#define AMDRM_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif

struct tvp_region
{
	uint64_t start;
	uint64_t end;
	int mem_flags;
};

#define TVP_MM_ENABLE_FLAGS_FOR_4K 0x02

extern int tvp_mm_enable(int flags);
extern int tvp_mm_disable(int flags);
extern int tvp_mm_get_mem_region(struct tvp_region* region, int region_size);
extern int tvp_mm_get_enable();
extern int free_cma_buffer(void);


#ifdef  __cplusplus
}
#endif

#endif

