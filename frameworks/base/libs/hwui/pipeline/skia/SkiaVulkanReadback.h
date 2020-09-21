/*
 * Copyright (C) 2018 The Android Open Source Project
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

#pragma once

#include "Readback.h"

namespace android {
namespace uirenderer {
namespace skiapipeline {

class SkiaVulkanReadback : public Readback {
public:
    SkiaVulkanReadback(renderthread::RenderThread& thread) : Readback(thread) {}

    virtual CopyResult copySurfaceInto(Surface& surface, const Rect& srcRect,
            SkBitmap* bitmap) override {
        //TODO: implement Vulkan readback.
        return CopyResult::UnknownError;
    }

    virtual CopyResult copyGraphicBufferInto(GraphicBuffer* graphicBuffer,
            SkBitmap* bitmap) override {
        //TODO: implement Vulkan readback.
        return CopyResult::UnknownError;
    }
};

} /* namespace skiapipeline */
} /* namespace uirenderer */
} /* namespace android */
