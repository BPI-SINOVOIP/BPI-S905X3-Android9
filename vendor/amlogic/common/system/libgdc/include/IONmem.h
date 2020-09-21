/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef IONMEM_H
#define IONMEM_H
#include <linux/ion.h>
#include <stdbool.h>
#if defined (__cplusplus)
extern "C" {
#endif

typedef struct IONMEM_AllocParams {
    ion_user_handle_t   mIonHnd;
    int                 mImageFd;
    size_t size;
    unsigned char *usr_ptr;
} IONMEM_AllocParams;

int ion_mem_init(void);
unsigned long ion_mem_alloc(size_t size, IONMEM_AllocParams *params, bool cache_flag);
int ion_mem_invalid_cache(int shared_fd);
int ion_mem_free(IONMEM_AllocParams *params);
int ion_mem_exit(void);

#if defined (__cplusplus)
}
#endif

#endif

