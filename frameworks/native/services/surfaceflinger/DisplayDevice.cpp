/*
 * Copyright (C) 2007 The Android Open Source Project
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

// #define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "DisplayDevice"

#include <array>
#include <unordered_set>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <cutils/properties.h>

#include <utils/RefBase.h>
#include <utils/Log.h>

#include <ui/DebugUtils.h>
#include <ui/DisplayInfo.h>
#include <ui/PixelFormat.h>

#include <gui/Surface.h>

#include <hardware/gralloc.h>

#include "DisplayHardware/DisplaySurface.h"
#include "DisplayHardware/HWComposer.h"
#include "DisplayHardware/HWC2.h"
#include "RenderEngine/RenderEngine.h"

#include "clz.h"
#include "DisplayDevice.h"
#include "SurfaceFlinger.h"
#include "Layer.h"

#include <android/hardware/configstore/1.0/ISurfaceFlingerConfigs.h>
#include <configstore/Utils.h>

namespace android {

// retrieve triple buffer setting from configstore
using namespace android::hardware::configstore;
using namespace android::hardware::configstore::V1_0;
using android::ui::ColorMode;
using android::ui::Dataspace;
using android::ui::Hdr;
using android::ui::RenderIntent;

/*
 * Initialize the display to the specified values.
 *
 */

uint32_t DisplayDevice::sPrimaryDisplayOrientation = 0;

namespace {

// ordered list of known SDR color modes
const std::array<ColorMode, 2> sSdrColorModes = {
        ColorMode::DISPLAY_P3,
        ColorMode::SRGB,
};

// ordered list of known HDR color modes
const std::array<ColorMode, 2> sHdrColorModes = {
        ColorMode::BT2100_PQ,
        ColorMode::BT2100_HLG,
};

// ordered list of known SDR render intents
const std::array<RenderIntent, 2> sSdrRenderIntents = {
        RenderIntent::ENHANCE,
        RenderIntent::COLORIMETRIC,
};

// ordered list of known HDR render intents
const std::array<RenderIntent, 2> sHdrRenderIntents = {
        RenderIntent::TONE_MAP_ENHANCE,
        RenderIntent::TONE_MAP_COLORIMETRIC,
};

// map known color mode to dataspace
Dataspace colorModeToDataspace(ColorMode mode) {
    switch (mode) {
        case ColorMode::SRGB:
            return Dataspace::SRGB;
        case ColorMode::DISPLAY_P3:
            return Dataspace::DISPLAY_P3;
        case ColorMode::BT2100_HLG:
            return Dataspace::BT2020_HLG;
        case ColorMode::BT2100_PQ:
            return Dataspace::BT2020_PQ;
        default:
            return Dataspace::UNKNOWN;
    }
}

// Return a list of candidate color modes.
std::vector<ColorMode> getColorModeCandidates(ColorMode mode) {
    std::vector<ColorMode> candidates;

    // add mode itself
    candidates.push_back(mode);

    // check if mode is HDR
    bool isHdr = false;
    for (auto hdrMode : sHdrColorModes) {
        if (hdrMode == mode) {
            isHdr = true;
            break;
        }
    }

    // add other HDR candidates when mode is HDR
    if (isHdr) {
        for (auto hdrMode : sHdrColorModes) {
            if (hdrMode != mode) {
                candidates.push_back(hdrMode);
            }
        }
    }

    // add other SDR candidates
    for (auto sdrMode : sSdrColorModes) {
        if (sdrMode != mode) {
            candidates.push_back(sdrMode);
        }
    }

    return candidates;
}

// Return a list of candidate render intents.
std::vector<RenderIntent> getRenderIntentCandidates(RenderIntent intent) {
    std::vector<RenderIntent> candidates;

    // add intent itself
    candidates.push_back(intent);

    // check if intent is HDR
    bool isHdr = false;
    for (auto hdrIntent : sHdrRenderIntents) {
        if (hdrIntent == intent) {
            isHdr = true;
            break;
        }
    }

    if (isHdr) {
        // add other HDR candidates when intent is HDR
        for (auto hdrIntent : sHdrRenderIntents) {
            if (hdrIntent != intent) {
                candidates.push_back(hdrIntent);
            }
        }
    } else {
        // add other SDR candidates when intent is SDR
        for (auto sdrIntent : sSdrRenderIntents) {
            if (sdrIntent != intent) {
                candidates.push_back(sdrIntent);
            }
        }
    }

    return candidates;
}

// Return the best color mode supported by HWC.
ColorMode getHwcColorMode(
        const std::unordered_map<ColorMode, std::vector<RenderIntent>>& hwcColorModes,
        ColorMode mode) {
    std::vector<ColorMode> candidates = getColorModeCandidates(mode);
    for (auto candidate : candidates) {
        auto iter = hwcColorModes.find(candidate);
        if (iter != hwcColorModes.end()) {
            return candidate;
        }
    }

    return ColorMode::NATIVE;
}

// Return the best render intent supported by HWC.
RenderIntent getHwcRenderIntent(const std::vector<RenderIntent>& hwcIntents, RenderIntent intent) {
    std::vector<RenderIntent> candidates = getRenderIntentCandidates(intent);
    for (auto candidate : candidates) {
        for (auto hwcIntent : hwcIntents) {
            if (candidate == hwcIntent) {
                return candidate;
            }
        }
    }

    return RenderIntent::COLORIMETRIC;
}

} // anonymous namespace

// clang-format off
DisplayDevice::DisplayDevice(
        const sp<SurfaceFlinger>& flinger,
        DisplayType type,
        int32_t hwcId,
        bool isSecure,
        const wp<IBinder>& displayToken,
        const sp<ANativeWindow>& nativeWindow,
        const sp<DisplaySurface>& displaySurface,
        std::unique_ptr<RE::Surface> renderSurface,
        int displayWidth,
        int displayHeight,
        bool hasWideColorGamut,
        const HdrCapabilities& hdrCapabilities,
        const int32_t supportedPerFrameMetadata,
        const std::unordered_map<ColorMode, std::vector<RenderIntent>>& hwcColorModes,
        int initialPowerMode)
    : lastCompositionHadVisibleLayers(false),
      mFlinger(flinger),
      mType(type),
      mHwcDisplayId(hwcId),
      mDisplayToken(displayToken),
      mNativeWindow(nativeWindow),
      mDisplaySurface(displaySurface),
      mSurface{std::move(renderSurface)},
      mDisplayWidth(displayWidth),
      mDisplayHeight(displayHeight),
      mPageFlipCount(0),
      mIsSecure(isSecure),
      mLayerStack(NO_LAYER_STACK),
      mOrientation(),
      mViewport(Rect::INVALID_RECT),
      mFrame(Rect::INVALID_RECT),
      mPowerMode(initialPowerMode),
      mActiveConfig(0),
      mColorTransform(HAL_COLOR_TRANSFORM_IDENTITY),
      mHasWideColorGamut(hasWideColorGamut),
      mHasHdr10(false),
      mHasHLG(false),
      mHasDolbyVision(false),
      mSupportedPerFrameMetadata(supportedPerFrameMetadata)
{
    // clang-format on
    populateColorModes(hwcColorModes);

    std::vector<Hdr> types = hdrCapabilities.getSupportedHdrTypes();
    for (Hdr hdrType : types) {
        switch (hdrType) {
            case Hdr::HDR10:
                mHasHdr10 = true;
                break;
            case Hdr::HLG:
                mHasHLG = true;
                break;
            case Hdr::DOLBY_VISION:
                mHasDolbyVision = true;
                break;
            default:
                ALOGE("UNKNOWN HDR capability: %d", static_cast<int32_t>(hdrType));
        }
    }

    float minLuminance = hdrCapabilities.getDesiredMinLuminance();
    float maxLuminance = hdrCapabilities.getDesiredMaxLuminance();
    float maxAverageLuminance = hdrCapabilities.getDesiredMaxAverageLuminance();

    minLuminance = minLuminance <= 0.0 ? sDefaultMinLumiance : minLuminance;
    maxLuminance = maxLuminance <= 0.0 ? sDefaultMaxLumiance : maxLuminance;
    maxAverageLuminance = maxAverageLuminance <= 0.0 ? sDefaultMaxLumiance : maxAverageLuminance;
    if (this->hasWideColorGamut()) {
        // insert HDR10/HLG as we will force client composition for HDR10/HLG
        // layers
        if (!hasHDR10Support()) {
          types.push_back(Hdr::HDR10);
        }

        if (!hasHLGSupport()) {
          types.push_back(Hdr::HLG);
        }
    }
    mHdrCapabilities = HdrCapabilities(types, maxLuminance, maxAverageLuminance, minLuminance);

    // initialize the display orientation transform.
    setProjection(DisplayState::eOrientationDefault, mViewport, mFrame);
}

DisplayDevice::~DisplayDevice() = default;

void DisplayDevice::disconnect(HWComposer& hwc) {
    if (mHwcDisplayId >= 0) {
        hwc.disconnectDisplay(mHwcDisplayId);
        mHwcDisplayId = -1;
    }
}

bool DisplayDevice::isValid() const {
    return mFlinger != nullptr;
}

int DisplayDevice::getWidth() const {
    return mDisplayWidth;
}

int DisplayDevice::getHeight() const {
    return mDisplayHeight;
}

void DisplayDevice::setDisplayName(const String8& displayName) {
    if (!displayName.isEmpty()) {
        // never override the name with an empty name
        mDisplayName = displayName;
    }
}

uint32_t DisplayDevice::getPageFlipCount() const {
    return mPageFlipCount;
}

void DisplayDevice::flip() const
{
    mFlinger->getRenderEngine().checkErrors();
    mPageFlipCount++;
}

status_t DisplayDevice::beginFrame(bool mustRecompose) const {
    return mDisplaySurface->beginFrame(mustRecompose);
}

status_t DisplayDevice::prepareFrame(HWComposer& hwc) {
    status_t error = hwc.prepare(*this);
    if (error != NO_ERROR) {
        return error;
    }

    DisplaySurface::CompositionType compositionType;
    bool hasClient = hwc.hasClientComposition(mHwcDisplayId);
    bool hasDevice = hwc.hasDeviceComposition(mHwcDisplayId);
    if (hasClient && hasDevice) {
        compositionType = DisplaySurface::COMPOSITION_MIXED;
    } else if (hasClient) {
        compositionType = DisplaySurface::COMPOSITION_GLES;
    } else if (hasDevice) {
        compositionType = DisplaySurface::COMPOSITION_HWC;
    } else {
        // Nothing to do -- when turning the screen off we get a frame like
        // this. Call it a HWC frame since we won't be doing any GLES work but
        // will do a prepare/set cycle.
        compositionType = DisplaySurface::COMPOSITION_HWC;
    }
    return mDisplaySurface->prepareFrame(compositionType);
}

void DisplayDevice::swapBuffers(HWComposer& hwc) const {
    if (hwc.hasClientComposition(mHwcDisplayId) || hwc.hasFlipClientTargetRequest(mHwcDisplayId)) {
        mSurface->swapBuffers();
    }

    status_t result = mDisplaySurface->advanceFrame();
    if (result != NO_ERROR) {
        ALOGE("[%s] failed pushing new frame to HWC: %d",
                mDisplayName.string(), result);
    }
}

void DisplayDevice::onSwapBuffersCompleted() const {
    mDisplaySurface->onFrameCommitted();
}

bool DisplayDevice::makeCurrent() const {
    bool success = mFlinger->getRenderEngine().setCurrentSurface(*mSurface);
    setViewportAndProjection();
    return success;
}

void DisplayDevice::setViewportAndProjection() const {
    size_t w = mDisplayWidth;
    size_t h = mDisplayHeight;
    Rect sourceCrop(0, 0, w, h);
    mFlinger->getRenderEngine().setViewportAndProjection(w, h, sourceCrop, h,
        false, Transform::ROT_0);
}

const sp<Fence>& DisplayDevice::getClientTargetAcquireFence() const {
    return mDisplaySurface->getClientTargetAcquireFence();
}

// ----------------------------------------------------------------------------

void DisplayDevice::setVisibleLayersSortedByZ(const Vector< sp<Layer> >& layers) {
    mVisibleLayersSortedByZ = layers;
}

const Vector< sp<Layer> >& DisplayDevice::getVisibleLayersSortedByZ() const {
    return mVisibleLayersSortedByZ;
}

void DisplayDevice::setLayersNeedingFences(const Vector< sp<Layer> >& layers) {
    mLayersNeedingFences = layers;
}

const Vector< sp<Layer> >& DisplayDevice::getLayersNeedingFences() const {
    return mLayersNeedingFences;
}

Region DisplayDevice::getDirtyRegion(bool repaintEverything) const {
    Region dirty;
    if (repaintEverything) {
        dirty.set(getBounds());
    } else {
        const Transform& planeTransform(mGlobalTransform);
        dirty = planeTransform.transform(this->dirtyRegion);
        dirty.andSelf(getBounds());
    }
    return dirty;
}

// ----------------------------------------------------------------------------
void DisplayDevice::setPowerMode(int mode) {
    mPowerMode = mode;
}

int DisplayDevice::getPowerMode()  const {
    return mPowerMode;
}

bool DisplayDevice::isDisplayOn() const {
    return (mPowerMode != HWC_POWER_MODE_OFF);
}

// ----------------------------------------------------------------------------
void DisplayDevice::setActiveConfig(int mode) {
    mActiveConfig = mode;
}

int DisplayDevice::getActiveConfig()  const {
    return mActiveConfig;
}

// ----------------------------------------------------------------------------
void DisplayDevice::setActiveColorMode(ColorMode mode) {
    mActiveColorMode = mode;
}

ColorMode DisplayDevice::getActiveColorMode() const {
    return mActiveColorMode;
}

RenderIntent DisplayDevice::getActiveRenderIntent() const {
    return mActiveRenderIntent;
}

void DisplayDevice::setActiveRenderIntent(RenderIntent renderIntent) {
    mActiveRenderIntent = renderIntent;
}

void DisplayDevice::setColorTransform(const mat4& transform) {
    const bool isIdentity = (transform == mat4());
    mColorTransform =
            isIdentity ? HAL_COLOR_TRANSFORM_IDENTITY : HAL_COLOR_TRANSFORM_ARBITRARY_MATRIX;
}

android_color_transform_t DisplayDevice::getColorTransform() const {
    return mColorTransform;
}

void DisplayDevice::setCompositionDataSpace(ui::Dataspace dataspace) {
    mCompositionDataSpace = dataspace;
    ANativeWindow* const window = mNativeWindow.get();
    native_window_set_buffers_data_space(window, static_cast<android_dataspace>(dataspace));
}

ui::Dataspace DisplayDevice::getCompositionDataSpace() const {
    return mCompositionDataSpace;
}

// ----------------------------------------------------------------------------

void DisplayDevice::setLayerStack(uint32_t stack) {
    mLayerStack = stack;
    dirtyRegion.set(bounds());
}

// ----------------------------------------------------------------------------

uint32_t DisplayDevice::getOrientationTransform() const {
    uint32_t transform = 0;
    switch (mOrientation) {
        case DisplayState::eOrientationDefault:
            transform = Transform::ROT_0;
            break;
        case DisplayState::eOrientation90:
            transform = Transform::ROT_90;
            break;
        case DisplayState::eOrientation180:
            transform = Transform::ROT_180;
            break;
        case DisplayState::eOrientation270:
            transform = Transform::ROT_270;
            break;
    }
    return transform;
}

status_t DisplayDevice::orientationToTransfrom(
        int orientation, int w, int h, Transform* tr)
{
    uint32_t flags = 0;
    switch (orientation) {
    case DisplayState::eOrientationDefault:
        flags = Transform::ROT_0;
        break;
    case DisplayState::eOrientation90:
        flags = Transform::ROT_90;
        break;
    case DisplayState::eOrientation180:
        flags = Transform::ROT_180;
        break;
    case DisplayState::eOrientation270:
        flags = Transform::ROT_270;
        break;
    default:
        return BAD_VALUE;
    }
    tr->set(flags, w, h);
    return NO_ERROR;
}

void DisplayDevice::setDisplaySize(const int newWidth, const int newHeight) {
    dirtyRegion.set(getBounds());

    mSurface->setNativeWindow(nullptr);

    mDisplaySurface->resizeBuffers(newWidth, newHeight);

    ANativeWindow* const window = mNativeWindow.get();
    mSurface->setNativeWindow(window);
    mDisplayWidth = mSurface->queryWidth();
    mDisplayHeight = mSurface->queryHeight();

    LOG_FATAL_IF(mDisplayWidth != newWidth,
                "Unable to set new width to %d", newWidth);
    LOG_FATAL_IF(mDisplayHeight != newHeight,
                "Unable to set new height to %d", newHeight);
}

void DisplayDevice::setProjection(int orientation,
        const Rect& newViewport, const Rect& newFrame) {
    Rect viewport(newViewport);
    Rect frame(newFrame);

    const int w = mDisplayWidth;
    const int h = mDisplayHeight;

    Transform R;
    DisplayDevice::orientationToTransfrom(orientation, w, h, &R);

    if (!frame.isValid()) {
        // the destination frame can be invalid if it has never been set,
        // in that case we assume the whole display frame.
        frame = Rect(w, h);
    }

    if (viewport.isEmpty()) {
        // viewport can be invalid if it has never been set, in that case
        // we assume the whole display size.
        // it's also invalid to have an empty viewport, so we handle that
        // case in the same way.
        viewport = Rect(w, h);
        if (R.getOrientation() & Transform::ROT_90) {
            // viewport is always specified in the logical orientation
            // of the display (ie: post-rotation).
            swap(viewport.right, viewport.bottom);
        }
    }

    dirtyRegion.set(getBounds());

    Transform TL, TP, S;
    float src_width  = viewport.width();
    float src_height = viewport.height();
    float dst_width  = frame.width();
    float dst_height = frame.height();
    if (src_width != dst_width || src_height != dst_height) {
        float sx = dst_width  / src_width;
        float sy = dst_height / src_height;
        S.set(sx, 0, 0, sy);
    }

    float src_x = viewport.left;
    float src_y = viewport.top;
    float dst_x = frame.left;
    float dst_y = frame.top;
    TL.set(-src_x, -src_y);
    TP.set(dst_x, dst_y);

    // need to take care of primary display rotation for mGlobalTransform
    // for case if the panel is not installed aligned with device orientation
    if (mType == DisplayType::DISPLAY_PRIMARY) {
        int primaryDisplayOrientation = mFlinger->getPrimaryDisplayOrientation();
        DisplayDevice::orientationToTransfrom(
                (orientation + primaryDisplayOrientation) % (DisplayState::eOrientation270 + 1),
                w, h, &R);
    }

    // The viewport and frame are both in the logical orientation.
    // Apply the logical translation, scale to physical size, apply the
    // physical translation and finally rotate to the physical orientation.
    mGlobalTransform = R * TP * S * TL;

    const uint8_t type = mGlobalTransform.getType();
    mNeedsFiltering = (!mGlobalTransform.preserveRects() ||
            (type >= Transform::SCALE));

    mScissor = mGlobalTransform.transform(viewport);
    if (mScissor.isEmpty()) {
        mScissor = getBounds();
    }

    mOrientation = orientation;
    if (mType == DisplayType::DISPLAY_PRIMARY) {
        uint32_t transform = 0;
        switch (mOrientation) {
            case DisplayState::eOrientationDefault:
                transform = Transform::ROT_0;
                break;
            case DisplayState::eOrientation90:
                transform = Transform::ROT_90;
                break;
            case DisplayState::eOrientation180:
                transform = Transform::ROT_180;
                break;
            case DisplayState::eOrientation270:
                transform = Transform::ROT_270;
                break;
        }
        sPrimaryDisplayOrientation = transform;
    }
    mViewport = viewport;
    mFrame = frame;
}

uint32_t DisplayDevice::getPrimaryDisplayOrientationTransform() {
    return sPrimaryDisplayOrientation;
}

void DisplayDevice::dump(String8& result) const {
    const Transform& tr(mGlobalTransform);
    ANativeWindow* const window = mNativeWindow.get();
    result.appendFormat("+ DisplayDevice: %s\n", mDisplayName.string());
    result.appendFormat("   type=%x, hwcId=%d, layerStack=%u, (%4dx%4d), ANativeWindow=%p "
                        "(%d:%d:%d:%d), orient=%2d (type=%08x), "
                        "flips=%u, isSecure=%d, powerMode=%d, activeConfig=%d, numLayers=%zu\n",
                        mType, mHwcDisplayId, mLayerStack, mDisplayWidth, mDisplayHeight, window,
                        mSurface->queryRedSize(), mSurface->queryGreenSize(),
                        mSurface->queryBlueSize(), mSurface->queryAlphaSize(), mOrientation,
                        tr.getType(), getPageFlipCount(), mIsSecure, mPowerMode, mActiveConfig,
                        mVisibleLayersSortedByZ.size());
    result.appendFormat("   v:[%d,%d,%d,%d], f:[%d,%d,%d,%d], s:[%d,%d,%d,%d],"
                        "transform:[[%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f][%0.3f,%0.3f,%0.3f]]\n",
                        mViewport.left, mViewport.top, mViewport.right, mViewport.bottom,
                        mFrame.left, mFrame.top, mFrame.right, mFrame.bottom, mScissor.left,
                        mScissor.top, mScissor.right, mScissor.bottom, tr[0][0], tr[1][0], tr[2][0],
                        tr[0][1], tr[1][1], tr[2][1], tr[0][2], tr[1][2], tr[2][2]);
    auto const surface = static_cast<Surface*>(window);
    ui::Dataspace dataspace = surface->getBuffersDataSpace();
    result.appendFormat("   wideColorGamut=%d, hdr10=%d, colorMode=%s, dataspace: %s (%d)\n",
                        mHasWideColorGamut, mHasHdr10,
                        decodeColorMode(mActiveColorMode).c_str(),
                        dataspaceDetails(static_cast<android_dataspace>(dataspace)).c_str(), dataspace);

    String8 surfaceDump;
    mDisplaySurface->dumpAsString(surfaceDump);
    result.append(surfaceDump);
}

// Map dataspace/intent to the best matched dataspace/colorMode/renderIntent
// supported by HWC.
void DisplayDevice::addColorMode(
        const std::unordered_map<ColorMode, std::vector<RenderIntent>>& hwcColorModes,
        const ColorMode mode, const RenderIntent intent) {
    // find the best color mode
    const ColorMode hwcColorMode = getHwcColorMode(hwcColorModes, mode);

    // find the best render intent
    auto iter = hwcColorModes.find(hwcColorMode);
    const auto& hwcIntents =
            iter != hwcColorModes.end() ? iter->second : std::vector<RenderIntent>();
    const RenderIntent hwcIntent = getHwcRenderIntent(hwcIntents, intent);

    const Dataspace dataspace = colorModeToDataspace(mode);
    const Dataspace hwcDataspace = colorModeToDataspace(hwcColorMode);

    ALOGV("DisplayDevice %d/%d: map (%s, %s) to (%s, %s, %s)", mType, mHwcDisplayId,
          dataspaceDetails(static_cast<android_dataspace_t>(dataspace)).c_str(),
          decodeRenderIntent(intent).c_str(),
          dataspaceDetails(static_cast<android_dataspace_t>(hwcDataspace)).c_str(),
          decodeColorMode(hwcColorMode).c_str(), decodeRenderIntent(hwcIntent).c_str());

    mColorModes[getColorModeKey(dataspace, intent)] = {hwcDataspace, hwcColorMode, hwcIntent};
}

void DisplayDevice::populateColorModes(
        const std::unordered_map<ColorMode, std::vector<RenderIntent>>& hwcColorModes) {
    if (!hasWideColorGamut()) {
        return;
    }

    // collect all known SDR render intents
    std::unordered_set<RenderIntent> sdrRenderIntents(sSdrRenderIntents.begin(),
                                                      sSdrRenderIntents.end());
    auto iter = hwcColorModes.find(ColorMode::SRGB);
    if (iter != hwcColorModes.end()) {
        for (auto intent : iter->second) {
            sdrRenderIntents.insert(intent);
        }
    }

    // add all known SDR combinations
    for (auto intent : sdrRenderIntents) {
        for (auto mode : sSdrColorModes) {
            addColorMode(hwcColorModes, mode, intent);
        }
    }

    // collect all known HDR render intents
    std::unordered_set<RenderIntent> hdrRenderIntents(sHdrRenderIntents.begin(),
                                                      sHdrRenderIntents.end());
    iter = hwcColorModes.find(ColorMode::BT2100_PQ);
    if (iter != hwcColorModes.end()) {
        for (auto intent : iter->second) {
            hdrRenderIntents.insert(intent);
        }
    }

    // add all known HDR combinations
    for (auto intent : sHdrRenderIntents) {
        for (auto mode : sHdrColorModes) {
            addColorMode(hwcColorModes, mode, intent);
        }
    }
}

bool DisplayDevice::hasRenderIntent(RenderIntent intent) const {
    // assume a render intent is supported when SRGB supports it; we should
    // get rid of that assumption.
    auto iter = mColorModes.find(getColorModeKey(Dataspace::SRGB, intent));
    return iter != mColorModes.end() && iter->second.renderIntent == intent;
}

bool DisplayDevice::hasLegacyHdrSupport(Dataspace dataspace) const {
    if ((dataspace == Dataspace::BT2020_PQ && hasHDR10Support()) ||
        (dataspace == Dataspace::BT2020_HLG && hasHLGSupport())) {
        auto iter =
                mColorModes.find(getColorModeKey(dataspace, RenderIntent::TONE_MAP_COLORIMETRIC));
        return iter == mColorModes.end() || iter->second.dataspace != dataspace;
    }

    return false;
}

void DisplayDevice::getBestColorMode(Dataspace dataspace, RenderIntent intent,
                                     Dataspace* outDataspace, ColorMode* outMode,
                                     RenderIntent* outIntent) const {
    auto iter = mColorModes.find(getColorModeKey(dataspace, intent));
    if (iter != mColorModes.end()) {
        *outDataspace = iter->second.dataspace;
        *outMode = iter->second.colorMode;
        *outIntent = iter->second.renderIntent;
    } else {
        ALOGE("map unknown (%s)/(%s) to default color mode",
              dataspaceDetails(static_cast<android_dataspace_t>(dataspace)).c_str(),
              decodeRenderIntent(intent).c_str());

        *outDataspace = Dataspace::UNKNOWN;
        *outMode = ColorMode::NATIVE;
        *outIntent = RenderIntent::COLORIMETRIC;
    }
}

std::atomic<int32_t> DisplayDeviceState::nextDisplayId(1);

DisplayDeviceState::DisplayDeviceState(DisplayDevice::DisplayType type, bool isSecure)
    : type(type),
      layerStack(DisplayDevice::NO_LAYER_STACK),
      orientation(0),
      width(0),
      height(0),
      isSecure(isSecure)
{
    viewport.makeInvalid();
    frame.makeInvalid();
}

}  // namespace android
