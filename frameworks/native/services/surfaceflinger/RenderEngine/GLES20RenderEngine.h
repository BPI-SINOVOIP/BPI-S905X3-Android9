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

#ifndef SF_GLES20RENDERENGINE_H_
#define SF_GLES20RENDERENGINE_H_

#include <stdint.h>
#include <sys/types.h>

#include <GLES2/gl2.h>
#include <Transform.h>

#include "Description.h"
#include "ProgramCache.h"
#include "RenderEngine.h"

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

class String8;
class Mesh;
class Texture;

namespace RE {
namespace impl {

class GLES20RenderEngine : public RenderEngine {
    GLuint mProtectedTexName;
    GLint mMaxViewportDims[2];
    GLint mMaxTextureSize;
    GLuint mVpWidth;
    GLuint mVpHeight;

    struct Group {
        GLuint texture;
        GLuint fbo;
        GLuint width;
        GLuint height;
        mat4 colorTransform;
    };

    Description mState;
    Vector<Group> mGroupStack;

    virtual void bindImageAsFramebuffer(EGLImageKHR image, uint32_t* texName, uint32_t* fbName,
                                        uint32_t* status);
    virtual void unbindFramebuffer(uint32_t texName, uint32_t fbName);

public:
    GLES20RenderEngine(uint32_t featureFlags); // See RenderEngine::FeatureFlag
    virtual ~GLES20RenderEngine();

protected:
    virtual void dump(String8& result);
    virtual void setViewportAndProjection(size_t vpw, size_t vph, Rect sourceCrop, size_t hwh,
                                          bool yswap, Transform::orientation_flags rotation);
    virtual void setupLayerBlending(bool premultipliedAlpha, bool opaque, bool disableTexture,
                                    const half4& color) override;

    // Color management related functions and state
    void setSourceY410BT2020(bool enable) override;
    void setSourceDataSpace(ui::Dataspace source) override;
    void setOutputDataSpace(ui::Dataspace dataspace) override;
    void setDisplayMaxLuminance(const float maxLuminance) override;

    virtual void setupLayerTexturing(const Texture& texture);
    virtual void setupLayerBlackedOut();
    virtual void setupFillWithColor(float r, float g, float b, float a);
    virtual void setupColorTransform(const mat4& colorTransform);
    virtual void setSaturationMatrix(const mat4& saturationMatrix);
    virtual void disableTexturing();
    virtual void disableBlending();

    virtual void drawMesh(const Mesh& mesh);

    virtual size_t getMaxTextureSize() const;
    virtual size_t getMaxViewportDims() const;

    // Current dataspace of layer being rendered
    ui::Dataspace mDataSpace = ui::Dataspace::UNKNOWN;

    // Current output dataspace of the render engine
    ui::Dataspace mOutputDataSpace = ui::Dataspace::UNKNOWN;

    // Currently only supporting sRGB, BT2020 and DisplayP3 color spaces
    const bool mPlatformHasWideColor = false;
    mat4 mSrgbToDisplayP3;
    mat4 mDisplayP3ToSrgb;
    mat3 mSrgbToXyz;
    mat3 mBt2020ToXyz;
    mat3 mDisplayP3ToXyz;
    mat4 mXyzToSrgb;
    mat4 mXyzToDisplayP3;
    mat4 mXyzToBt2020;

private:
    // A data space is considered HDR data space if it has BT2020 color space
    // with PQ or HLG transfer function.
    bool isHdrDataSpace(const ui::Dataspace dataSpace) const;
    bool needsXYZTransformMatrix() const;
};

// ---------------------------------------------------------------------------
} // namespace impl
} // namespace RE
} // namespace android
// ---------------------------------------------------------------------------

#endif /* SF_GLES20RENDERENGINE_H_ */
