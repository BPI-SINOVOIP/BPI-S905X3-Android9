/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *
 * Description:
 */

#ifndef __UTIL_H
#define __UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

void yuyv422_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height);
void nv21_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height);
void nv21_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height);
void yv12_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
