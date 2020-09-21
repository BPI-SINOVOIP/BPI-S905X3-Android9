/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef MISC_H
#define MISC_H

#include <stdlib.h>
#include <cutils/native_handle.h>

#define PROP_VALUE_LEN_MAX  92

bool sys_get_bool_prop(const char* prop, bool defVal);
int32_t sys_get_string_prop(const char* prop, char * val);
int32_t sys_set_prop(const char *prop, const char *val);

int32_t sysfs_get_int(const char* path, int32_t def);
int32_t sysfs_get_string(const char* path, char *str, int32_t len);
int32_t sysfs_get_string_ex(const char* path, char *str, int32_t size, bool needOriginalData);
int32_t sysfs_set_string(const char *path, const char *val);

native_handle_t * gralloc_alloc_dma_buf(int w, int h, int format, bool bScanout, bool afbc = false);
int32_t gralloc_free_dma_buf(native_handle_t * hnd);

native_handle_t * gralloc_ref_dma_buf(const native_handle_t * hnd);
int32_t gralloc_unref_dma_buf(native_handle_t * hnd);

int32_t gralloc_lock_dma_buf(native_handle_t * handle, void** vaddr);
int32_t gralloc_unlock_dma_buf(native_handle_t * handle);


#endif/*MISC_H*/
