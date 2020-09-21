/*
 * Copyright (C) 2016 ARM Limited. All rights reserved.
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

#ifndef MALI_GRALLOC_ION_H_
#define MALI_GRALLOC_ION_H_

#include "mali_gralloc_module.h"
#include "mali_gralloc_bufferdescriptor.h"

int mali_gralloc_ion_allocate(mali_gralloc_module *m, const gralloc_buffer_descriptor_t *descriptors,
                              uint32_t numDescriptors, buffer_handle_t *pHandle, bool *alloc_from_backing_store);
void mali_gralloc_ion_free(private_handle_t const *hnd);
void mali_gralloc_ion_sync(const mali_gralloc_module *m, private_handle_t *hnd);
int mali_gralloc_ion_map(private_handle_t *hnd);
void mali_gralloc_ion_unmap(private_handle_t *hnd);
int mali_gralloc_ion_device_close(struct hw_device_t *device);

#endif /* MALI_GRALLOC_ION_H_ */
