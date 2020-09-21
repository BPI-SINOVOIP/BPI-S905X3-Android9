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

#include <utils/Log.h>
#include <string.h>

#include "util.h"

#ifndef ALIGN
#define ALIGN(b,w) (((b)+((w)-1))/(w)*(w))
#endif

static inline void yuv_to_rgb24(unsigned char y,unsigned char u,unsigned char v,unsigned char *rgb)
{
    register int r,g,b;
    int rgb24;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;

    rgb24 = (int)((r << 16) | (g  << 8)| b);

    *rgb = (unsigned char)r;
    rgb++;
    *rgb = (unsigned char)g;
    rgb++;
    *rgb = (unsigned char)b;
}

void yuyv422_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height)
{
    int y,z=0;
    int blocks;

    blocks = (width * height) * 2;

    for (y = 0,z = 0; y < blocks; y += 4,z += 6) {
        unsigned char Y1, Y2, U, V;

        Y1 = buf[y + 0];
        U = buf[y + 1];
        Y2 = buf[y + 2];
        V = buf[y + 3];

        yuv_to_rgb24(Y1, U, V, &rgb[z]);
        yuv_to_rgb24(Y2, U, V, &rgb[z + 3]);
    }
}

void nv21_to_rgb24(unsigned char *buf, unsigned char *rgb, int width, int height)
{
    int y,z = 0;
    int h;
    int blocks;
    unsigned char Y1, Y2, U, V;

    blocks = (width * height) * 2;

    for (h = 0, z = 0; h < height; h += 2) {
        for (y = 0; y < width * 2; y += 2) {

            Y1 = buf[ h * width + y + 0];
            V = buf[ blocks/2 + h * width/2 + y % width + 0 ];
            Y2 = buf[ h * width + y + 1];
            U = buf[ blocks/2 + h * width/2 + y % width + 1 ];

            yuv_to_rgb24(Y1, U, V, &rgb[z]);
            yuv_to_rgb24(Y2, U, V, &rgb[z + 3]);
            z += 6;
        }
    }
}

void nv21_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height)
{
        int stride = (width + 31) & ( ~31);
        int h;
        for (h = 0; h < height* 3/2; h++)
        {
                memcpy( dst, src, width);
                dst += width;
                src += stride;
        }
}

void yv12_memcpy_align32(unsigned char *dst, unsigned char *src, int width, int height)
{
        int new_width = (width + 63) & ( ~63);
        int stride;
        int h;
        for (h = 0; h < height; h++)
        {
                memcpy( dst, src, width);
                dst += width;
                src += new_width;
        }
        stride = ALIGN( width/2, 16);
        for (h = 0; h < height; h++)
        {
                memcpy( dst, src, width/2);
                dst += stride;
                src += new_width/2;
        }
}

