/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ESE_SG_H_
#define ESE_SG_H_ 1

#include "../../../libese-sysdeps/include/ese/sysdeps.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Scatter-Gather Buffer */
struct EseSgBuffer {
  union {
     uint8_t *base;
     const uint8_t *c_base;
  };
  uint32_t len;
};

#define ese_sg_set(ese_sg_ptr, buf, len) \
  (ese_sg_ptr)->base = buf; \
  (ese_sg_ptr)->len = len

/* Computes the total length/size of the buffer on each call. */
uint32_t ese_sg_length(const struct EseSgBuffer *bufs, uint32_t cnt);
uint32_t ese_sg_to_buf(const struct EseSgBuffer *src, uint32_t src_cnt, uint32_t start_at, uint32_t length, uint8_t *dst);
uint32_t ese_sg_from_buf(struct EseSgBuffer *dst, uint32_t dst_cnt, uint32_t start_at, uint32_t length, const uint8_t *src);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ESE_SG_H_ */
