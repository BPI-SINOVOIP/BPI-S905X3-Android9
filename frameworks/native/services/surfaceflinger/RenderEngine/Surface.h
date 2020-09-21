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

struct ANativeWindow;

namespace android {
namespace RE {

class Surface {
public:
    virtual ~Surface() = 0;

    virtual void setCritical(bool enable) = 0;
    virtual void setAsync(bool enable) = 0;

    virtual void setNativeWindow(ANativeWindow* window) = 0;
    virtual void swapBuffers() const = 0;

    virtual int32_t queryRedSize() const = 0;
    virtual int32_t queryGreenSize() const = 0;
    virtual int32_t queryBlueSize() const = 0;
    virtual int32_t queryAlphaSize() const = 0;

    virtual int32_t queryWidth() const = 0;
    virtual int32_t queryHeight() const = 0;
};

namespace impl {

class RenderEngine;

class Surface final : public RE::Surface {
public:
    Surface(const RenderEngine& engine);
    ~Surface();

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    // RE::Surface implementation
    void setCritical(bool enable) override { mCritical = enable; }
    void setAsync(bool enable) override { mAsync = enable; }

    void setNativeWindow(ANativeWindow* window) override;
    void swapBuffers() const override;

    int32_t queryRedSize() const override;
    int32_t queryGreenSize() const override;
    int32_t queryBlueSize() const override;
    int32_t queryAlphaSize() const override;

    int32_t queryWidth() const override;
    int32_t queryHeight() const override;

private:
    EGLint queryConfig(EGLint attrib) const;
    EGLint querySurface(EGLint attrib) const;

    // methods internal to RenderEngine
    friend class RenderEngine;
    bool getAsync() const { return mAsync; }
    EGLSurface getEGLSurface() const { return mEGLSurface; }

    EGLDisplay mEGLDisplay;
    EGLConfig mEGLConfig;

    bool mCritical = false;
    bool mAsync = false;

    ANativeWindow* mWindow = nullptr;
    EGLSurface mEGLSurface = EGL_NO_SURFACE;
};

} // namespace impl
} // namespace RE
} // namespace android
