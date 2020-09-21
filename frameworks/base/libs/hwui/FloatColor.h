/*
 * Copyright (C) 2015 The Android Open Source Project
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
#ifndef FLOATCOLOR_H
#define FLOATCOLOR_H

#include "utils/Color.h"
#include "utils/Macros.h"
#include "utils/MathUtils.h"

#include <stdint.h>

namespace android {
namespace uirenderer {

struct FloatColor {
    // "color" is a gamma-encoded sRGB color
    // After calling this method, the color is stored as a pre-multiplied linear color
    // if linear blending is enabled. Otherwise, the color is stored as a pre-multiplied
    // gamma-encoded sRGB color
    void set(uint32_t color) {
        a = ((color >> 24) & 0xff) / 255.0f;
        r = a * EOCF(((color >> 16) & 0xff) / 255.0f);
        g = a * EOCF(((color >> 8) & 0xff) / 255.0f);
        b = a * EOCF(((color)&0xff) / 255.0f);
    }

    // "color" is a gamma-encoded sRGB color
    // After calling this method, the color is stored as a un-premultiplied linear color
    // if linear blending is enabled. Otherwise, the color is stored as a un-premultiplied
    // gamma-encoded sRGB color
    void setUnPreMultiplied(uint32_t color) {
        a = ((color >> 24) & 0xff) / 255.0f;
        r = EOCF(((color >> 16) & 0xff) / 255.0f);
        g = EOCF(((color >> 8) & 0xff) / 255.0f);
        b = EOCF(((color)&0xff) / 255.0f);
    }

    bool isNotBlack() { return a < 1.0f || r > 0.0f || g > 0.0f || b > 0.0f; }

    bool operator==(const FloatColor& other) const {
        return MathUtils::areEqual(r, other.r) && MathUtils::areEqual(g, other.g) &&
               MathUtils::areEqual(b, other.b) && MathUtils::areEqual(a, other.a);
    }

    bool operator!=(const FloatColor& other) const { return !(*this == other); }

    float r;
    float g;
    float b;
    float a;
};

REQUIRE_COMPATIBLE_LAYOUT(FloatColor);

} /* namespace uirenderer */
} /* namespace android */

#endif /* FLOATCOLOR_H */
