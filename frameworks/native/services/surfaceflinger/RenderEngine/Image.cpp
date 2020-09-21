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

#include "Image.h"

#include <vector>

#include <log/log.h>

#include "GLExtensions.h"
#include "RenderEngine.h"

namespace android {
namespace RE {

Image::~Image() = default;

namespace impl {

Image::Image(const RenderEngine& engine) : mEGLDisplay(engine.getEGLDisplay()) {}

Image::~Image() {
    setNativeWindowBuffer(nullptr, false, 0, 0);
}

static std::vector<EGLint> buildAttributeList(bool isProtected, int32_t cropWidth,
                                              int32_t cropHeight) {
    std::vector<EGLint> attrs;
    attrs.reserve(16);

    attrs.push_back(EGL_IMAGE_PRESERVED_KHR);
    attrs.push_back(EGL_TRUE);

    if (isProtected && GLExtensions::getInstance().hasProtectedContent()) {
        attrs.push_back(EGL_PROTECTED_CONTENT_EXT);
        attrs.push_back(EGL_TRUE);
    }

    if (cropWidth > 0 && cropHeight > 0) {
        attrs.push_back(EGL_IMAGE_CROP_LEFT_ANDROID);
        attrs.push_back(0);
        attrs.push_back(EGL_IMAGE_CROP_TOP_ANDROID);
        attrs.push_back(0);
        attrs.push_back(EGL_IMAGE_CROP_RIGHT_ANDROID);
        attrs.push_back(cropWidth);
        attrs.push_back(EGL_IMAGE_CROP_BOTTOM_ANDROID);
        attrs.push_back(cropHeight);
    }

    attrs.push_back(EGL_NONE);

    return attrs;
}

bool Image::setNativeWindowBuffer(ANativeWindowBuffer* buffer, bool isProtected, int32_t cropWidth,
                                  int32_t cropHeight) {
    if (mEGLImage != EGL_NO_IMAGE_KHR) {
        if (!eglDestroyImageKHR(mEGLDisplay, mEGLImage)) {
            ALOGE("failed to destroy image: %#x", eglGetError());
        }
        mEGLImage = EGL_NO_IMAGE_KHR;
    }

    if (buffer) {
        std::vector<EGLint> attrs = buildAttributeList(isProtected, cropWidth, cropHeight);
        mEGLImage = eglCreateImageKHR(mEGLDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                      static_cast<EGLClientBuffer>(buffer), attrs.data());
        if (mEGLImage == EGL_NO_IMAGE_KHR) {
            ALOGE("failed to create EGLImage: %#x", eglGetError());
            return false;
        }
    }

    return true;
}

} // namespace impl
} // namespace RE
} // namespace android
