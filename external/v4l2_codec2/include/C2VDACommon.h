// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_C2_VDA_COMMON_H
#define ANDROID_C2_VDA_COMMON_H

#include <inttypes.h>

namespace android {
enum class HalPixelFormat : uint32_t {
    UNKNOWN = 0x0,
    // The pixel formats defined in Android but are used among C2VDAComponent.
    YCbCr_420_888 = 0x23,
    YV12 = 0x32315659,
    NV12 = 0x3231564e,
};
} // namespace android
#endif  // ANDROID_C2_VDA_COMMON_H
