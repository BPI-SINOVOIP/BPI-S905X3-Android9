/*
 * Copyright 2017 The Android Open Source Project
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

#include <cstdint>

#include <EGL/egl.h>
#include <EGL/eglext.h>

struct ANativeWindowBuffer;

namespace android {
namespace RE {

class Image {
public:
    virtual ~Image() = 0;
    virtual bool setNativeWindowBuffer(ANativeWindowBuffer* buffer, bool isProtected,
                                       int32_t cropWidth, int32_t cropHeight) = 0;
};

namespace impl {

class RenderEngine;

class Image : public RE::Image {
public:
    explicit Image(const RenderEngine& engine);
    ~Image() override;

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    bool setNativeWindowBuffer(ANativeWindowBuffer* buffer, bool isProtected, int32_t cropWidth,
                               int32_t cropHeight) override;

private:
    // methods internal to RenderEngine
    friend class RenderEngine;
    EGLSurface getEGLImage() const { return mEGLImage; }

    EGLDisplay mEGLDisplay;
    EGLImageKHR mEGLImage = EGL_NO_IMAGE_KHR;
};

} // namespace impl
} // namespace RE
} // namespace android
