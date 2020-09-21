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

#include "Surface.h"

#include "RenderEngine.h"

#include <log/log.h>

namespace android {
namespace RE {

Surface::~Surface() = default;

namespace impl {

Surface::Surface(const RenderEngine& engine)
      : mEGLDisplay(engine.getEGLDisplay()), mEGLConfig(engine.getEGLConfig()) {
    // RE does not assume any config when EGL_KHR_no_config_context is supported
    if (mEGLConfig == EGL_NO_CONFIG_KHR) {
        mEGLConfig = RenderEngine::chooseEglConfig(mEGLDisplay, PIXEL_FORMAT_RGBA_8888, false);
    }
}

Surface::~Surface() {
    setNativeWindow(nullptr);
}

void Surface::setNativeWindow(ANativeWindow* window) {
    if (mEGLSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mEGLDisplay, mEGLSurface);
        mEGLSurface = EGL_NO_SURFACE;
    }

    mWindow = window;
    if (mWindow) {
        mEGLSurface = eglCreateWindowSurface(mEGLDisplay, mEGLConfig, mWindow, nullptr);
    }
}

void Surface::swapBuffers() const {
    if (!eglSwapBuffers(mEGLDisplay, mEGLSurface)) {
        EGLint error = eglGetError();

        const char format[] = "eglSwapBuffers(%p, %p) failed with 0x%08x";
        if (mCritical || error == EGL_CONTEXT_LOST) {
            LOG_ALWAYS_FATAL(format, mEGLDisplay, mEGLSurface, error);
        } else {
            ALOGE(format, mEGLDisplay, mEGLSurface, error);
        }
    }
}

EGLint Surface::queryConfig(EGLint attrib) const {
    EGLint value;
    if (!eglGetConfigAttrib(mEGLDisplay, mEGLConfig, attrib, &value)) {
        value = 0;
    }

    return value;
}

EGLint Surface::querySurface(EGLint attrib) const {
    EGLint value;
    if (!eglQuerySurface(mEGLDisplay, mEGLSurface, attrib, &value)) {
        value = 0;
    }

    return value;
}

int32_t Surface::queryRedSize() const {
    return queryConfig(EGL_RED_SIZE);
}

int32_t Surface::queryGreenSize() const {
    return queryConfig(EGL_GREEN_SIZE);
}

int32_t Surface::queryBlueSize() const {
    return queryConfig(EGL_BLUE_SIZE);
}

int32_t Surface::queryAlphaSize() const {
    return queryConfig(EGL_ALPHA_SIZE);
}

int32_t Surface::queryWidth() const {
    return querySurface(EGL_WIDTH);
}

int32_t Surface::queryHeight() const {
    return querySurface(EGL_HEIGHT);
}

} // namespace impl
} // namespace RE
} // namespace android
