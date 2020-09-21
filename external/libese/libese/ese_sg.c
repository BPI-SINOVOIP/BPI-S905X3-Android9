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

#include "include/ese/ese.h"

static inline uint32_t min_u32(uint32_t a, uint32_t b) { return a < b ? a : b; }

ESE_API uint32_t ese_sg_length(const struct EseSgBuffer *bufs, uint32_t cnt) {
  const struct EseSgBuffer *buf = bufs;
  uint32_t len = 0;
  if (!bufs || cnt == 0) {
    return 0;
  }
  while (buf < bufs + cnt) {
    /* In reality, this is probably corruption or a mistake. */
    if (ESE_UINT32_MAX - len < buf->len) {
      return ESE_UINT32_MAX;
    }
    len += buf->len;
    buf++;
  }
  return len;
}

ESE_API uint32_t ese_sg_to_buf(const struct EseSgBuffer *src, uint32_t src_cnt,
                               uint32_t start_at, uint32_t length,
                               uint8_t *dst) {
  const struct EseSgBuffer *buf = src;
  uint32_t remaining = length;
  if (!src || src_cnt == 0) {
    return 0;
  }
  while (remaining && buf < src + src_cnt) {
    if (start_at > buf->len) {
      start_at -= buf->len;
      buf++;
      continue;
    }
    uint32_t copy_len = min_u32(remaining, buf->len - start_at);
    ese_memcpy(dst, buf->c_base + start_at, copy_len);
    dst += copy_len;
    remaining -= copy_len;
    start_at = 0;
    buf++;
  }
  return length - remaining;
}

ESE_API uint32_t ese_sg_from_buf(struct EseSgBuffer *dst, uint32_t dst_cnt,
                                 uint32_t start_at, uint32_t length,
                                 const uint8_t *src) {
  const struct EseSgBuffer *buf = dst;
  uint32_t remaining = length;
  if (!dst || dst_cnt == 0) {
    return 0;
  }

  while (remaining && buf < dst + dst_cnt) {
    if (start_at >= buf->len) {
      start_at -= buf->len;
      buf++;
      continue;
    }
    uint32_t copy_len = min_u32(remaining, buf->len - start_at);
    ese_memcpy(buf->base + start_at, src, copy_len);
    src += copy_len;
    remaining -= copy_len;
    start_at = 0;
    buf++;
  }
  return length - remaining;
}
