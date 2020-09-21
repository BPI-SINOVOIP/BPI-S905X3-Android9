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

#ifndef AML_CAMERA_HARDWARE_INCLUDE_
#define AML_CAMERA_HARDWARE_INCLUDE_

#ifndef ALIGN
#define ALIGN(b,w) (((b)+((w)-1))/(w)*(w))
#endif

void convert_rgb24_to_rgb16(uint8_t *rgb888, uint8_t *rgb565, int width, int height);
void yuyv422_to_rgb16(unsigned char *from, unsigned char *to, int width,int height);
void yuyv422_to_rgb16(unsigned char *from, unsigned char *to, int size);
void yuyv422_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height);
void yuyv422_to_nv21(unsigned char *bufsrc, unsigned char *bufdest, int width, int height);
void yv12_adjust_memcpy(unsigned char *dst, unsigned char *src, int width, int height);
void yuyv_to_yv12(unsigned char *src, unsigned char *dst, int width, int height);
void rgb24_memcpy(unsigned char *dst, unsigned char *src, int width, int height);
void nv21_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height);
void yv12_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height);

void nv21_memcpy_canvas1080(unsigned char *dst, unsigned char *src, int width, int height);
void yv12_memcpy_canvas1080(unsigned char *dst, unsigned char *src, int width, int height);
#endif /* AML_CAMERA_HARDWARE_INCLUDE_*/
