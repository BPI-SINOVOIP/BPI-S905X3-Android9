/*
 * Copyright (C) 2014 The Android Open Source Project
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
#ifndef PAINT_UTILS_H
#define PAINT_UTILS_H

#include <utils/Blur.h>

#include <SkColorFilter.h>
#include <SkDrawLooper.h>
#include <SkShader.h>

namespace android {
namespace uirenderer {

/**
 * Utility methods for accessing data within SkPaint, and providing defaults
 * with optional SkPaint pointers.
 */
class PaintUtils {
public:
    static inline GLenum getFilter(const SkPaint* paint) {
        if (!paint || paint->getFilterQuality() != kNone_SkFilterQuality) {
            return GL_LINEAR;
        }
        return GL_NEAREST;
    }

    static bool isOpaquePaint(const SkPaint* paint) {
        if (!paint) return true;  // default (paintless) behavior is SrcOver, black

        if (paint->getAlpha() != 0xFF || PaintUtils::isBlendedShader(paint->getShader()) ||
            PaintUtils::isBlendedColorFilter(paint->getColorFilter())) {
            return false;
        }

        // Only let simple srcOver / src blending modes declare opaque, since behavior is clear.
        SkBlendMode mode = paint->getBlendMode();
        return mode == SkBlendMode::kSrcOver || mode == SkBlendMode::kSrc;
    }

    static bool isBlendedShader(const SkShader* shader) {
        if (shader == nullptr) {
            return false;
        }
        return !shader->isOpaque();
    }

    static bool isBlendedColorFilter(const SkColorFilter* filter) {
        if (filter == nullptr) {
            return false;
        }
        return (filter->getFlags() & SkColorFilter::kAlphaUnchanged_Flag) == 0;
    }

    struct TextShadow {
        SkScalar radius;
        float dx;
        float dy;
        SkColor color;
    };

    static inline bool getTextShadow(const SkPaint* paint, TextShadow* textShadow) {
        SkDrawLooper::BlurShadowRec blur;
        if (paint && paint->getLooper() && paint->getLooper()->asABlurShadow(&blur)) {
            if (textShadow) {
                textShadow->radius = Blur::convertSigmaToRadius(blur.fSigma);
                textShadow->dx = blur.fOffset.fX;
                textShadow->dy = blur.fOffset.fY;
                textShadow->color = blur.fColor;
            }
            return true;
        }
        return false;
    }

    static inline bool hasTextShadow(const SkPaint* paint) { return getTextShadow(paint, nullptr); }

    static inline SkBlendMode getBlendModeDirect(const SkPaint* paint) {
        return paint ? paint->getBlendMode() : SkBlendMode::kSrcOver;
    }

    static inline int getAlphaDirect(const SkPaint* paint) {
        return paint ? paint->getAlpha() : 255;
    }

};  // class PaintUtils

} /* namespace uirenderer */
} /* namespace android */

#endif /* PAINT_UTILS_H */
