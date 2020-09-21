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

#include <GLES2/gl2.h>
#include "Texture.h"

#ifndef SF_RENDER_ENGINE_DESCRIPTION_H_
#define SF_RENDER_ENGINE_DESCRIPTION_H_

namespace android {

class Program;

/*
 * This holds the state of the rendering engine. This class is used
 * to generate a corresponding GLSL program and set the appropriate
 * uniform.
 *
 * Program and ProgramCache are friends and access the state directly
 */
class Description {
public:
    Description() = default;
    ~Description() = default;

    void setPremultipliedAlpha(bool premultipliedAlpha);
    void setOpaque(bool opaque);
    void setTexture(const Texture& texture);
    void disableTexture();
    void setColor(const half4& color);
    void setProjectionMatrix(const mat4& mtx);
    void setSaturationMatrix(const mat4& mtx);
    void setColorMatrix(const mat4& mtx);
    void setInputTransformMatrix(const mat3& matrix);
    void setOutputTransformMatrix(const mat4& matrix);
    bool hasInputTransformMatrix() const;
    bool hasOutputTransformMatrix() const;
    bool hasColorMatrix() const;
    bool hasSaturationMatrix() const;
    const mat4& getColorMatrix() const;

    void setY410BT2020(bool enable);

    enum class TransferFunction : int {
        LINEAR,
        SRGB,
        ST2084,
        HLG,  // Hybrid Log-Gamma for HDR.
    };
    void setInputTransferFunction(TransferFunction transferFunction);
    void setOutputTransferFunction(TransferFunction transferFunction);
    void setDisplayMaxLuminance(const float maxLuminance);

private:
    friend class Program;
    friend class ProgramCache;

    // whether textures are premultiplied
    bool mPremultipliedAlpha = false;
    // whether this layer is marked as opaque
    bool mOpaque = true;

    // Texture this layer uses
    Texture mTexture;
    bool mTextureEnabled = false;

    // color used when texturing is disabled or when setting alpha.
    half4 mColor;

    // true if the sampled pixel values are in Y410/BT2020 rather than RGBA
    bool mY410BT2020 = false;

    // transfer functions for the input/output
    TransferFunction mInputTransferFunction = TransferFunction::LINEAR;
    TransferFunction mOutputTransferFunction = TransferFunction::LINEAR;

    float mDisplayMaxLuminance;

    // projection matrix
    mat4 mProjectionMatrix;
    mat4 mColorMatrix;
    mat4 mSaturationMatrix;
    mat3 mInputTransformMatrix;
    mat4 mOutputTransformMatrix;
};

} /* namespace android */

#endif /* SF_RENDER_ENGINE_DESCRIPTION_H_ */
