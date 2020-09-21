/*
 * Copyright 2013 The Android Open Source Project
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

#ifndef SF_RENDERENGINE_H_
#define SF_RENDERENGINE_H_

#include <memory>

#include <stdint.h>
#include <sys/types.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <Transform.h>
#include <android-base/unique_fd.h>
#include <gui/SurfaceControl.h>
#include <math/mat4.h>

#define EGL_NO_CONFIG ((EGLConfig)0)

struct ANativeWindowBuffer;

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

class String8;
class Rect;
class Region;
class Mesh;
class Texture;

namespace RE {

class Image;
class Surface;
class BindNativeBufferAsFramebuffer;

namespace impl {
class RenderEngine;
}

class RenderEngine {
public:
    enum FeatureFlag {
        WIDE_COLOR_SUPPORT = 1 << 0 // Platform has a wide color display
    };

    virtual ~RenderEngine() = 0;

    virtual std::unique_ptr<RE::Surface> createSurface() = 0;
    virtual std::unique_ptr<RE::Image> createImage() = 0;

    virtual void primeCache() const = 0;

    // dump the extension strings. always call the base class.
    virtual void dump(String8& result) = 0;

    virtual bool supportsImageCrop() const = 0;

    virtual bool isCurrent() const = 0;
    virtual bool setCurrentSurface(const RE::Surface& surface) = 0;
    virtual void resetCurrentSurface() = 0;

    // helpers
    // flush submits RenderEngine command stream for execution and returns a
    // native fence fd that is signaled when the execution has completed.  It
    // returns -1 on errors.
    virtual base::unique_fd flush() = 0;
    // finish waits until RenderEngine command stream has been executed.  It
    // returns false on errors.
    virtual bool finish() = 0;
    // waitFence inserts a wait on an external fence fd to RenderEngine
    // command stream.  It returns false on errors.
    virtual bool waitFence(base::unique_fd fenceFd) = 0;

    virtual void clearWithColor(float red, float green, float blue, float alpha) = 0;
    virtual void fillRegionWithColor(const Region& region, uint32_t height, float red, float green,
                                     float blue, float alpha) = 0;

    // common to all GL versions
    virtual void setScissor(uint32_t left, uint32_t bottom, uint32_t right, uint32_t top) = 0;
    virtual void disableScissor() = 0;
    virtual void genTextures(size_t count, uint32_t* names) = 0;
    virtual void deleteTextures(size_t count, uint32_t const* names) = 0;
    virtual void bindExternalTextureImage(uint32_t texName, const RE::Image& image) = 0;
    virtual void readPixels(size_t l, size_t b, size_t w, size_t h, uint32_t* pixels) = 0;
    virtual void bindNativeBufferAsFrameBuffer(ANativeWindowBuffer* buffer,
                                               RE::BindNativeBufferAsFramebuffer* bindHelper) = 0;
    virtual void unbindNativeBufferAsFrameBuffer(RE::BindNativeBufferAsFramebuffer* bindHelper) = 0;

    // set-up
    virtual void checkErrors() const;
    virtual void setViewportAndProjection(size_t vpw, size_t vph, Rect sourceCrop, size_t hwh,
                                          bool yswap, Transform::orientation_flags rotation) = 0;
    virtual void setupLayerBlending(bool premultipliedAlpha, bool opaque, bool disableTexture,
                                    const half4& color) = 0;
    virtual void setupLayerTexturing(const Texture& texture) = 0;
    virtual void setupLayerBlackedOut() = 0;
    virtual void setupFillWithColor(float r, float g, float b, float a) = 0;

    virtual void setupColorTransform(const mat4& /* colorTransform */) = 0;
    virtual void setSaturationMatrix(const mat4& /* saturationMatrix */) = 0;

    virtual void disableTexturing() = 0;
    virtual void disableBlending() = 0;

    // HDR and wide color gamut support
    virtual void setSourceY410BT2020(bool enable) = 0;
    virtual void setSourceDataSpace(ui::Dataspace source) = 0;
    virtual void setOutputDataSpace(ui::Dataspace dataspace) = 0;
    virtual void setDisplayMaxLuminance(const float maxLuminance) = 0;

    // drawing
    virtual void drawMesh(const Mesh& mesh) = 0;

    // queries
    virtual size_t getMaxTextureSize() const = 0;
    virtual size_t getMaxViewportDims() const = 0;
};

class BindNativeBufferAsFramebuffer {
public:
    BindNativeBufferAsFramebuffer(RenderEngine& engine, ANativeWindowBuffer* buffer)
          : mEngine(engine) {
        mEngine.bindNativeBufferAsFrameBuffer(buffer, this);
    }
    ~BindNativeBufferAsFramebuffer() { mEngine.unbindNativeBufferAsFrameBuffer(this); }
    status_t getStatus() const { return mStatus; }

protected:
    friend impl::RenderEngine;

    RenderEngine& mEngine;
    EGLImageKHR mImage;
    uint32_t mTexName, mFbName;
    status_t mStatus;
};

namespace impl {

class Image;
class Surface;

class RenderEngine : public RE::RenderEngine {
    enum GlesVersion {
        GLES_VERSION_1_0 = 0x10000,
        GLES_VERSION_1_1 = 0x10001,
        GLES_VERSION_2_0 = 0x20000,
        GLES_VERSION_3_0 = 0x30000,
    };
    static GlesVersion parseGlesVersion(const char* str);

    EGLDisplay mEGLDisplay;
    EGLConfig mEGLConfig;
    EGLContext mEGLContext;
    void setEGLHandles(EGLDisplay display, EGLConfig config, EGLContext ctxt);

    static bool overrideUseContextPriorityFromConfig(bool useContextPriority);

protected:
    RenderEngine(uint32_t featureFlags);

    const uint32_t mFeatureFlags;

public:
    virtual ~RenderEngine() = 0;

    static std::unique_ptr<RenderEngine> create(int hwcFormat, uint32_t featureFlags);

    static EGLConfig chooseEglConfig(EGLDisplay display, int format, bool logConfig);

    // RenderEngine interface implementation

    std::unique_ptr<RE::Surface> createSurface() override;
    std::unique_ptr<RE::Image> createImage() override;

    void primeCache() const override;

    // dump the extension strings. always call the base class.
    void dump(String8& result) override;

    bool supportsImageCrop() const override;

    bool isCurrent() const;
    bool setCurrentSurface(const RE::Surface& surface) override;
    void resetCurrentSurface() override;

    // synchronization

    // flush submits RenderEngine command stream for execution and returns a
    // native fence fd that is signaled when the execution has completed.  It
    // returns -1 on errors.
    base::unique_fd flush() override;
    // finish waits until RenderEngine command stream has been executed.  It
    // returns false on errors.
    bool finish() override;
    // waitFence inserts a wait on an external fence fd to RenderEngine
    // command stream.  It returns false on errors.
    bool waitFence(base::unique_fd fenceFd) override;

    // helpers
    void clearWithColor(float red, float green, float blue, float alpha) override;
    void fillRegionWithColor(const Region& region, uint32_t height, float red, float green,
                             float blue, float alpha) override;

    // common to all GL versions
    void setScissor(uint32_t left, uint32_t bottom, uint32_t right, uint32_t top) override;
    void disableScissor() override;
    void genTextures(size_t count, uint32_t* names) override;
    void deleteTextures(size_t count, uint32_t const* names) override;
    void bindExternalTextureImage(uint32_t texName, const RE::Image& image) override;
    void readPixels(size_t l, size_t b, size_t w, size_t h, uint32_t* pixels) override;

    void checkErrors() const override;

    void setupColorTransform(const mat4& /* colorTransform */) override {}
    void setSaturationMatrix(const mat4& /* saturationMatrix */) override {}

    // internal to RenderEngine
    EGLDisplay getEGLDisplay() const;
    EGLConfig getEGLConfig() const;

    // Common implementation
    bool setCurrentSurface(const RE::impl::Surface& surface);
    void bindExternalTextureImage(uint32_t texName, const RE::impl::Image& image);

    void bindNativeBufferAsFrameBuffer(ANativeWindowBuffer* buffer,
                                       RE::BindNativeBufferAsFramebuffer* bindHelper) override;
    void unbindNativeBufferAsFrameBuffer(RE::BindNativeBufferAsFramebuffer* bindHelper) override;

    // Overriden by each specialization
    virtual void bindImageAsFramebuffer(EGLImageKHR image, uint32_t* texName, uint32_t* fbName,
                                        uint32_t* status) = 0;
    virtual void unbindFramebuffer(uint32_t texName, uint32_t fbName) = 0;
};

} // namespace impl
} // namespace RE
} // namespace android

#endif /* SF_RENDERENGINE_H_ */
