/*
 * Copyright 2014 The Android Open Source Project
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

#include "common.rs"

// #define LOG_DEBUG 1

// Must be yuvx_444 (interleaved yuv)
rs_allocation mInput;

uint32_t width;
float inv_width; // must be set to 1/w

// Average 1D array -> Single element
// Input: mInput must be yuvx_444
yuvx_444 RS_KERNEL means_yuvx_444(void) {

    uint3 sum  = { 0, 0, 0 };

    LOGD("width", width);

    for (uint32_t i = 0; i < width; ++i) {
        yuvx_444 elem = rsGetElementAt_yuvx_444(mInput, i);

        LOGD("elem", elem);
        LOGD("i", i);

        sum.x += elem.x;
        sum.y += elem.y;
        sum.z += elem.z;
    }

    sum.x *= inv_width; // multiply by 1/w
    sum.y *= inv_width; // multiply by 1/w
    sum.z *= inv_width; // multiply by 1/w

    yuvx_444 avg = convert_yuvx_444(sum);

    return avg;
}

