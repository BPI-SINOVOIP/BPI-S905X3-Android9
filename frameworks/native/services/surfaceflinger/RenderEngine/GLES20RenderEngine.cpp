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

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "RenderEngine"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <ui/ColorSpace.h>
#include <ui/DebugUtils.h>
#include <ui/Rect.h>

#include <utils/String8.h>
#include <utils/Trace.h>

#include <cutils/compiler.h>
#include <gui/ISurfaceComposer.h>
#include <math.h>

#include "Description.h"
#include "GLES20RenderEngine.h"
#include "Mesh.h"
#include "Program.h"
#include "ProgramCache.h"
#include "Texture.h"

#include <fstream>
#include <sstream>

// ---------------------------------------------------------------------------
bool checkGlError(const char* op, int lineNumber) {
    bool errorFound = false;
    GLint error = glGetError();
    while (error != GL_NO_ERROR) {
        errorFound = true;
        error = glGetError();
        ALOGV("after %s() (line # %d) glError (0x%x)\n", op, lineNumber, error);
    }
    return errorFound;
}

static constexpr bool outputDebugPPMs = false;

void writePPM(const char* basename, GLuint width, GLuint height) {
    ALOGV("writePPM #%s: %d x %d", basename, width, height);

    std::vector<GLubyte> pixels(width * height * 4);
    std::vector<GLubyte> outBuffer(width * height * 3);

    // TODO(courtneygo): We can now have float formats, need
    // to remove this code or update to support.
    // Make returned pixels fit in uint32_t, one byte per component
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    if (checkGlError(__FUNCTION__, __LINE__)) {
        return;
    }

    std::string filename(basename);
    filename.append(".ppm");
    std::ofstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        ALOGE("Unable to open file: %s", filename.c_str());
        ALOGE("You may need to do: \"adb shell setenforce 0\" to enable "
              "surfaceflinger to write debug images");
        return;
    }

    file << "P6\n";
    file << width << "\n";
    file << height << "\n";
    file << 255 << "\n";

    auto ptr = reinterpret_cast<char*>(pixels.data());
    auto outPtr = reinterpret_cast<char*>(outBuffer.data());
    for (int y = height - 1; y >= 0; y--) {
        char* data = ptr + y * width * sizeof(uint32_t);

        for (GLuint x = 0; x < width; x++) {
            // Only copy R, G and B components
            outPtr[0] = data[0];
            outPtr[1] = data[1];
            outPtr[2] = data[2];
            data += sizeof(uint32_t);
            outPtr += 3;
        }
    }
    file.write(reinterpret_cast<char*>(outBuffer.data()), outBuffer.size());
}

// ---------------------------------------------------------------------------
namespace android {
namespace RE {
namespace impl {
// ---------------------------------------------------------------------------

using ui::Dataspace;

GLES20RenderEngine::GLES20RenderEngine(uint32_t featureFlags)
      : RenderEngine(featureFlags),
        mVpWidth(0),
        mVpHeight(0),
        mPlatformHasWideColor((featureFlags & WIDE_COLOR_SUPPORT) != 0) {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, mMaxViewportDims);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    const uint16_t protTexData[] = {0};
    glGenTextures(1, &mProtectedTexName);
    glBindTexture(GL_TEXTURE_2D, mProtectedTexName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, protTexData);

    // mColorBlindnessCorrection = M;

    if (mPlatformHasWideColor) {
        ColorSpace srgb(ColorSpace::sRGB());
        ColorSpace displayP3(ColorSpace::DisplayP3());
        ColorSpace bt2020(ColorSpace::BT2020());

        // Compute sRGB to Display P3 transform matrix.
        // NOTE: For now, we are limiting output wide color space support to
        // Display-P3 only.
        mSrgbToDisplayP3 = mat4(ColorSpaceConnector(srgb, displayP3).getTransform());

        // Compute Display P3 to sRGB transform matrix.
        mDisplayP3ToSrgb = mat4(ColorSpaceConnector(displayP3, srgb).getTransform());

        // no chromatic adaptation needed since all color spaces use D65 for their white points.
        mSrgbToXyz = srgb.getRGBtoXYZ();
        mDisplayP3ToXyz = displayP3.getRGBtoXYZ();
        mBt2020ToXyz = bt2020.getRGBtoXYZ();
        mXyzToSrgb = mat4(srgb.getXYZtoRGB());
        mXyzToDisplayP3 = mat4(displayP3.getXYZtoRGB());
        mXyzToBt2020 = mat4(bt2020.getXYZtoRGB());
    }
}

GLES20RenderEngine::~GLES20RenderEngine() {}

size_t GLES20RenderEngine::getMaxTextureSize() const {
    return mMaxTextureSize;
}

size_t GLES20RenderEngine::getMaxViewportDims() const {
    return mMaxViewportDims[0] < mMaxViewportDims[1] ? mMaxViewportDims[0] : mMaxViewportDims[1];
}

void GLES20RenderEngine::setViewportAndProjection(size_t vpw, size_t vph, Rect sourceCrop,
                                                  size_t hwh, bool yswap,
                                                  Transform::orientation_flags rotation) {
    int32_t l = sourceCrop.left;
    int32_t r = sourceCrop.right;

    // In GL, (0, 0) is the bottom-left corner, so flip y coordinates
    int32_t t = hwh - sourceCrop.top;
    int32_t b = hwh - sourceCrop.bottom;

    mat4 m;
    if (yswap) {
        m = mat4::ortho(l, r, t, b, 0, 1);
    } else {
        m = mat4::ortho(l, r, b, t, 0, 1);
    }

    // Apply custom rotation to the projection.
    float rot90InRadians = 2.0f * static_cast<float>(M_PI) / 4.0f;
    switch (rotation) {
        case Transform::ROT_0:
            break;
        case Transform::ROT_90:
            m = mat4::rotate(rot90InRadians, vec3(0, 0, 1)) * m;
            break;
        case Transform::ROT_180:
            m = mat4::rotate(rot90InRadians * 2.0f, vec3(0, 0, 1)) * m;
            break;
        case Transform::ROT_270:
            m = mat4::rotate(rot90InRadians * 3.0f, vec3(0, 0, 1)) * m;
            break;
        default:
            break;
    }

    glViewport(0, 0, vpw, vph);
    mState.setProjectionMatrix(m);
    mVpWidth = vpw;
    mVpHeight = vph;
}

void GLES20RenderEngine::setupLayerBlending(bool premultipliedAlpha, bool opaque,
                                            bool disableTexture, const half4& color) {
    mState.setPremultipliedAlpha(premultipliedAlpha);
    mState.setOpaque(opaque);
    mState.setColor(color);

    if (disableTexture) {
        mState.disableTexture();
    }

    if (color.a < 1.0f || !opaque) {
        glEnable(GL_BLEND);
        glBlendFunc(premultipliedAlpha ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

void GLES20RenderEngine::setSourceY410BT2020(bool enable) {
    mState.setY410BT2020(enable);
}

void GLES20RenderEngine::setSourceDataSpace(Dataspace source) {
    mDataSpace = source;
}

void GLES20RenderEngine::setOutputDataSpace(Dataspace dataspace) {
    mOutputDataSpace = dataspace;
}

void GLES20RenderEngine::setDisplayMaxLuminance(const float maxLuminance) {
    mState.setDisplayMaxLuminance(maxLuminance);
}

void GLES20RenderEngine::setupLayerTexturing(const Texture& texture) {
    GLuint target = texture.getTextureTarget();
    glBindTexture(target, texture.getTextureName());
    GLenum filter = GL_NEAREST;
    if (texture.getFiltering()) {
        filter = GL_LINEAR;
    }
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);

    mState.setTexture(texture);
}

void GLES20RenderEngine::setupLayerBlackedOut() {
    glBindTexture(GL_TEXTURE_2D, mProtectedTexName);
    Texture texture(Texture::TEXTURE_2D, mProtectedTexName);
    texture.setDimensions(1, 1); // FIXME: we should get that from somewhere
    mState.setTexture(texture);
}

void GLES20RenderEngine::setupColorTransform(const mat4& colorTransform) {
    mState.setColorMatrix(colorTransform);
}

void GLES20RenderEngine::setSaturationMatrix(const mat4& saturationMatrix) {
    mState.setSaturationMatrix(saturationMatrix);
}

void GLES20RenderEngine::disableTexturing() {
    mState.disableTexture();
}

void GLES20RenderEngine::disableBlending() {
    glDisable(GL_BLEND);
}

void GLES20RenderEngine::bindImageAsFramebuffer(EGLImageKHR image, uint32_t* texName,
                                                uint32_t* fbName, uint32_t* status) {
    GLuint tname, name;
    // turn our EGLImage into a texture
    glGenTextures(1, &tname);
    glBindTexture(GL_TEXTURE_2D, tname);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)image);

    // create a Framebuffer Object to render into
    glGenFramebuffers(1, &name);
    glBindFramebuffer(GL_FRAMEBUFFER, name);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tname, 0);

    *status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    *texName = tname;
    *fbName = name;
}

void GLES20RenderEngine::unbindFramebuffer(uint32_t texName, uint32_t fbName) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbName);
    glDeleteTextures(1, &texName);
}

void GLES20RenderEngine::setupFillWithColor(float r, float g, float b, float a) {
    mState.setPremultipliedAlpha(true);
    mState.setOpaque(false);
    mState.setColor(half4(r, g, b, a));
    mState.disableTexture();
    glDisable(GL_BLEND);
}

void GLES20RenderEngine::drawMesh(const Mesh& mesh) {
    ATRACE_CALL();
    if (mesh.getTexCoordsSize()) {
        glEnableVertexAttribArray(Program::texCoords);
        glVertexAttribPointer(Program::texCoords, mesh.getTexCoordsSize(), GL_FLOAT, GL_FALSE,
                              mesh.getByteStride(), mesh.getTexCoords());
    }

    glVertexAttribPointer(Program::position, mesh.getVertexSize(), GL_FLOAT, GL_FALSE,
                          mesh.getByteStride(), mesh.getPositions());

    // By default, DISPLAY_P3 is the only supported wide color output. However,
    // when HDR content is present, hardware composer may be able to handle
    // BT2020 data space, in that case, the output data space is set to be
    // BT2020_HLG or BT2020_PQ respectively. In GPU fall back we need
    // to respect this and convert non-HDR content to HDR format.
    if (mPlatformHasWideColor) {
        Description wideColorState = mState;
        Dataspace inputStandard = static_cast<Dataspace>(mDataSpace & Dataspace::STANDARD_MASK);
        Dataspace inputTransfer = static_cast<Dataspace>(mDataSpace & Dataspace::TRANSFER_MASK);
        Dataspace outputStandard = static_cast<Dataspace>(mOutputDataSpace &
                                                          Dataspace::STANDARD_MASK);
        Dataspace outputTransfer = static_cast<Dataspace>(mOutputDataSpace &
                                                          Dataspace::TRANSFER_MASK);
        bool needsXYZConversion = needsXYZTransformMatrix();

        if (needsXYZConversion) {
            // The supported input color spaces are standard RGB, Display P3 and BT2020.
            switch (inputStandard) {
                case Dataspace::STANDARD_DCI_P3:
                    wideColorState.setInputTransformMatrix(mDisplayP3ToXyz);
                    break;
                case Dataspace::STANDARD_BT2020:
                    wideColorState.setInputTransformMatrix(mBt2020ToXyz);
                    break;
                default:
                    wideColorState.setInputTransformMatrix(mSrgbToXyz);
                    break;
            }

            // The supported output color spaces are BT2020, Display P3 and standard RGB.
            switch (outputStandard) {
                case Dataspace::STANDARD_BT2020:
                    wideColorState.setOutputTransformMatrix(mXyzToBt2020);
                    break;
                case Dataspace::STANDARD_DCI_P3:
                    wideColorState.setOutputTransformMatrix(mXyzToDisplayP3);
                    break;
                default:
                    wideColorState.setOutputTransformMatrix(mXyzToSrgb);
                    break;
            }
        } else if (inputStandard != outputStandard) {
            // At this point, the input data space and output data space could be both
            // HDR data spaces, but they match each other, we do nothing in this case.
            // In addition to the case above, the input data space could be
            // - scRGB linear
            // - scRGB non-linear
            // - sRGB
            // - Display P3
            // The output data spaces could be
            // - sRGB
            // - Display P3
            if (outputStandard == Dataspace::STANDARD_BT709) {
                wideColorState.setOutputTransformMatrix(mDisplayP3ToSrgb);
            } else if (outputStandard == Dataspace::STANDARD_DCI_P3) {
                wideColorState.setOutputTransformMatrix(mSrgbToDisplayP3);
            }
        }

        // we need to convert the RGB value to linear space and convert it back when:
        // - there is a color matrix that is not an identity matrix, or
        // - there is a saturation matrix that is not an identity matrix, or
        // - there is an output transform matrix that is not an identity matrix, or
        // - the input transfer function doesn't match the output transfer function.
        if (wideColorState.hasColorMatrix() || wideColorState.hasSaturationMatrix() ||
            wideColorState.hasOutputTransformMatrix() || inputTransfer != outputTransfer) {
            switch (inputTransfer) {
                case Dataspace::TRANSFER_ST2084:
                    wideColorState.setInputTransferFunction(Description::TransferFunction::ST2084);
                    break;
                case Dataspace::TRANSFER_HLG:
                    wideColorState.setInputTransferFunction(Description::TransferFunction::HLG);
                    break;
                case Dataspace::TRANSFER_LINEAR:
                    wideColorState.setInputTransferFunction(Description::TransferFunction::LINEAR);
                    break;
                default:
                    wideColorState.setInputTransferFunction(Description::TransferFunction::SRGB);
                    break;
            }

            switch (outputTransfer) {
                case Dataspace::TRANSFER_ST2084:
                    wideColorState.setOutputTransferFunction(Description::TransferFunction::ST2084);
                    break;
                case Dataspace::TRANSFER_HLG:
                    wideColorState.setOutputTransferFunction(Description::TransferFunction::HLG);
                    break;
                default:
                    wideColorState.setOutputTransferFunction(Description::TransferFunction::SRGB);
                    break;
            }
        }

        ProgramCache::getInstance().useProgram(wideColorState);

        glDrawArrays(mesh.getPrimitive(), 0, mesh.getVertexCount());

        if (outputDebugPPMs) {
            static uint64_t wideColorFrameCount = 0;
            std::ostringstream out;
            out << "/data/texture_out" << wideColorFrameCount++;
            writePPM(out.str().c_str(), mVpWidth, mVpHeight);
        }
    } else {
        ProgramCache::getInstance().useProgram(mState);

        glDrawArrays(mesh.getPrimitive(), 0, mesh.getVertexCount());
    }

    if (mesh.getTexCoordsSize()) {
        glDisableVertexAttribArray(Program::texCoords);
    }
}

void GLES20RenderEngine::dump(String8& result) {
    RenderEngine::dump(result);
    result.appendFormat("RenderEngine last dataspace conversion: (%s) to (%s)\n",
                        dataspaceDetails(static_cast<android_dataspace>(mDataSpace)).c_str(),
                        dataspaceDetails(static_cast<android_dataspace>(mOutputDataSpace)).c_str());
}

bool GLES20RenderEngine::isHdrDataSpace(const Dataspace dataSpace) const {
    const Dataspace standard = static_cast<Dataspace>(dataSpace & Dataspace::STANDARD_MASK);
    const Dataspace transfer = static_cast<Dataspace>(dataSpace & Dataspace::TRANSFER_MASK);
    return standard == Dataspace::STANDARD_BT2020 &&
        (transfer == Dataspace::TRANSFER_ST2084 || transfer == Dataspace::TRANSFER_HLG);
}

// For convenience, we want to convert the input color space to XYZ color space first,
// and then convert from XYZ color space to output color space when
// - SDR and HDR contents are mixed, either SDR content will be converted to HDR or
//   HDR content will be tone-mapped to SDR; Or,
// - there are HDR PQ and HLG contents presented at the same time, where we want to convert
//   HLG content to PQ content.
// In either case above, we need to operate the Y value in XYZ color space. Thus, when either
// input data space or output data space is HDR data space, and the input transfer function
// doesn't match the output transfer function, we would enable an intermediate transfrom to
// XYZ color space.
bool GLES20RenderEngine::needsXYZTransformMatrix() const {
    const bool isInputHdrDataSpace = isHdrDataSpace(mDataSpace);
    const bool isOutputHdrDataSpace = isHdrDataSpace(mOutputDataSpace);
    const Dataspace inputTransfer = static_cast<Dataspace>(mDataSpace & Dataspace::TRANSFER_MASK);
    const Dataspace outputTransfer = static_cast<Dataspace>(mOutputDataSpace &
                                                            Dataspace::TRANSFER_MASK);

    return (isInputHdrDataSpace || isOutputHdrDataSpace) && inputTransfer != outputTransfer;
}

// ---------------------------------------------------------------------------
} // namespace impl
} // namespace RE
} // namespace android
// ---------------------------------------------------------------------------

#if defined(__gl_h_)
#error "don't include gl/gl.h in this file"
#endif
