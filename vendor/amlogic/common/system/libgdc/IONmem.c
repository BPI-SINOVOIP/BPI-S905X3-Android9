/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <IONmem.h>
#include <linux/ion.h>
#include <ion/ion.h>
#include <gdc_api.h>

static int ion_mem_fd = -2;
static int ref_count = 0;

static int validate_init()
{
    switch (ion_mem_fd) {
      case -3:
        E_GDC("ion_mem_exit() already called, check stderr output for earlier "
            "ion_mem failure messages (possibly version mismatch).\n");

        return 0;

      case -2:
        E_GDC("ion_mem_init() not called, you must initialize ion_mem before "
            "making API calls.\n");

        return 0;

      case -1:
        E_GDC("ion_mem file descriptor -1 (failed 'open()')\n");

        return 0;

      default:
        return 1;
    }
}

int ion_mem_init(void)
{
    int flags;

    if (ion_mem_fd >= 0) {
        ref_count++;
        D_GDC("init: /dev/ion already opened, incremented ref_count %d\n",
            ref_count);
        return 0;
    }

    ion_mem_fd = ion_open();

    if (ion_mem_fd < 0) {
        E_GDC("init: Failed to open /dev/ion: '%s'\n", strerror(errno));
        return -1;
    }

    ref_count++;
    D_GDC("init: entered - ref_count %d, ion_mem_fd %d\n", ref_count, ion_mem_fd);
    D_GDC("init: successfully opened /dev/ion...\n");
    D_GDC("init: exiting, returning success\n");

    return 0;
}


unsigned long ion_mem_alloc(size_t size, IONMEM_AllocParams *params, bool cache_flag)
{
    int ret = -1;
    unsigned long PhyAdrr = 0;
    struct ion_custom_data custom_data;

    if (!validate_init()) {
        return 0;
    }

    unsigned flag = 0;
    if (cache_flag) {
        flag = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
    }
    D_GDC("ion_mem_alloc, cache=%d, bytes=%d\n", cache_flag, size);
    if (!cache_flag) {
        ret = ion_alloc(ion_mem_fd, size, 0, ION_HEAP_TYPE_DMA_MASK, flag, &params->mIonHnd);
        if (ret < 0) {
            ret = ion_alloc(ion_mem_fd, size, 0, ION_HEAP_CARVEOUT_MASK, flag, &params->mIonHnd);
        }
    }
    if (ret < 0) {
        ret = ion_alloc(ion_mem_fd, size, 0, 1 << ION_HEAP_TYPE_CUSTOM, flag, &params->mIonHnd);
        if (ret < 0) {
            ion_close(ion_mem_fd);
            E_GDC("ion_alloc failed, errno=%d\n", errno);
            ion_mem_fd = -1;
            return -ENOMEM;
        }
    }
    ret = ion_share(ion_mem_fd, params->mIonHnd, &params->mImageFd);
    if (ret < 0) {
        E_GDC("ion_share failed, errno=%d\n", errno);
        ion_free(ion_mem_fd, params->mIonHnd);
        ion_close(ion_mem_fd);
        ion_mem_fd = -1;
        return -EINVAL;
    }
    ion_free(ion_mem_fd, params->mIonHnd);
    return ret;
}

#if 0
int ion_mem_invalid_cache(int shared_fd)
{
    if (ion_mem_fd !=-1 && shared_fd != -1) {
        if (ion_cache_invalid(ion_mem_fd, shared_fd)) {
            E_GDC("ion_mem_invalid_cache err!\n");
            return -1;
        }
    } else {
        E_GDC("ion_mem_invalid_cache err!\n");
        return -1;
    }
    return 0;
}
#endif

int ion_mem_free(IONMEM_AllocParams *params)
{
    if (!validate_init()) {
        return -1;
    }
    D_GDC("ion_mem_free,mIonHnd=%x free\n", params->mIonHnd);

    ion_free(ion_mem_fd, params->mIonHnd);

    return 0;
}


int ion_mem_exit(void)
{
    int result = 0;

    D_GDC("exit: entered - ref_count %d, ion_mem_fd %d\n", ref_count, ion_mem_fd);

    if (!validate_init()) {
        return -1;
    }

    D_GDC("exit: decrementing ref_count\n");

    ref_count--;
    if (ref_count == 0) {
        result = ion_close(ion_mem_fd);

        D_GDC("exit: ref_count == 0, closed /dev/ion (%s)\n",
            result == -1 ? strerror(errno) : "succeeded");

        /* setting -3 allows to distinguish ion_mem exit from ion_mem failed */
        ion_mem_fd = -3;
    }

    D_GDC("exit: exiting, returning %d\n", result);

    return result;
}

