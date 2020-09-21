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

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "Layer"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <algorithm>

#include <cutils/compiler.h>
#include <cutils/native_handle.h>
#include <cutils/properties.h>

#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/NativeHandle.h>
#include <utils/StopWatch.h>
#include <utils/Trace.h>

#include <ui/DebugUtils.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>

#include <gui/BufferItem.h>
#include <gui/LayerDebugInfo.h>
#include <gui/Surface.h>

#include "BufferLayer.h"
#include "Colorizer.h"
#include "DisplayDevice.h"
#include "Layer.h"
#include "LayerRejecter.h"
#include "MonitoredProducer.h"
#include "SurfaceFlinger.h"
#include "clz.h"

#include "DisplayHardware/HWComposer.h"

#include "RenderEngine/RenderEngine.h"

#include <mutex>
#include "LayerProtoHelper.h"

#define DEBUG_RESIZE 0

namespace android {

LayerBE::LayerBE()
      : mMesh(Mesh::TRIANGLE_FAN, 4, 2, 2) {
}


int32_t Layer::sSequence = 1;

Layer::Layer(SurfaceFlinger* flinger, const sp<Client>& client, const String8& name, uint32_t w,
             uint32_t h, uint32_t flags)
      : contentDirty(false),
        sequence(uint32_t(android_atomic_inc(&sSequence))),
        mFlinger(flinger),
        mPremultipliedAlpha(true),
        mName(name),
        mTransactionFlags(0),
        mPendingStateMutex(),
        mPendingStates(),
        mQueuedFrames(0),
        mSidebandStreamChanged(false),
        mActiveBufferSlot(BufferQueue::INVALID_BUFFER_SLOT),
        mCurrentTransform(0),
        mOverrideScalingMode(-1),
        mCurrentOpacity(true),
        mCurrentFrameNumber(0),
        mFrameLatencyNeeded(false),
        mFiltering(false),
        mNeedsFiltering(false),
        mProtectedByApp(false),
        mClientRef(client),
        mPotentialCursor(false),
        mQueueItemLock(),
        mQueueItemCondition(),
        mQueueItems(),
        mLastFrameNumberReceived(0),
        mAutoRefresh(false),
        mFreezeGeometryUpdates(false),
        mCurrentChildren(LayerVector::StateSet::Current),
        mDrawingChildren(LayerVector::StateSet::Drawing) {
    mCurrentCrop.makeInvalid();

    uint32_t layerFlags = 0;
    if (flags & ISurfaceComposerClient::eHidden) layerFlags |= layer_state_t::eLayerHidden;
    if (flags & ISurfaceComposerClient::eOpaque) layerFlags |= layer_state_t::eLayerOpaque;
    if (flags & ISurfaceComposerClient::eSecure) layerFlags |= layer_state_t::eLayerSecure;

    mName = name;
    mTransactionName = String8("TX - ") + mName;

    mCurrentState.active.w = w;
    mCurrentState.active.h = h;
    mCurrentState.flags = layerFlags;
    mCurrentState.active.transform.set(0, 0);
    mCurrentState.crop.makeInvalid();
    mCurrentState.finalCrop.makeInvalid();
    mCurrentState.requestedFinalCrop = mCurrentState.finalCrop;
    mCurrentState.requestedCrop = mCurrentState.crop;
    mCurrentState.z = 0;
    mCurrentState.color.a = 1.0f;
    mCurrentState.layerStack = 0;
    mCurrentState.sequence = 0;
    mCurrentState.requested = mCurrentState.active;
    mCurrentState.appId = 0;
    mCurrentState.type = 0;

    // drawing state & current state are identical
    mDrawingState = mCurrentState;

    const auto& hwc = flinger->getHwComposer();
    const auto& activeConfig = hwc.getActiveConfig(HWC_DISPLAY_PRIMARY);
    nsecs_t displayPeriod = activeConfig->getVsyncPeriod();
    mFrameTracker.setDisplayRefreshPeriod(displayPeriod);

    CompositorTiming compositorTiming;
    flinger->getCompositorTiming(&compositorTiming);
    mFrameEventHistory.initializeCompositorTiming(compositorTiming);
}

void Layer::onFirstRef() {}

Layer::~Layer() {
    sp<Client> c(mClientRef.promote());
    if (c != 0) {
        c->detachLayer(this);
    }

    for (auto& point : mRemoteSyncPoints) {
        point->setTransactionApplied();
    }
    for (auto& point : mLocalSyncPoints) {
        point->setFrameAvailable();
    }
    mFrameTracker.logAndResetStats(mName);
}

// ---------------------------------------------------------------------------
// callbacks
// ---------------------------------------------------------------------------

/*
 * onLayerDisplayed is only meaningful for BufferLayer, but, is called through
 * Layer.  So, the implementation is done in BufferLayer.  When called on a
 * ColorLayer object, it's essentially a NOP.
 */
void Layer::onLayerDisplayed(const sp<Fence>& /*releaseFence*/) {}

void Layer::onRemovedFromCurrentState() {
    // the layer is removed from SF mCurrentState to mLayersPendingRemoval

    mPendingRemoval = true;

    if (mCurrentState.zOrderRelativeOf != nullptr) {
        sp<Layer> strongRelative = mCurrentState.zOrderRelativeOf.promote();
        if (strongRelative != nullptr) {
            strongRelative->removeZOrderRelative(this);
            mFlinger->setTransactionFlags(eTraversalNeeded);
        }
        mCurrentState.zOrderRelativeOf = nullptr;
    }

    for (const auto& child : mCurrentChildren) {
        child->onRemovedFromCurrentState();
    }
}

void Layer::onRemoved() {
    // the layer is removed from SF mLayersPendingRemoval
    abandon();

    destroyAllHwcLayers();

    for (const auto& child : mCurrentChildren) {
        child->onRemoved();
    }
}

// ---------------------------------------------------------------------------
// set-up
// ---------------------------------------------------------------------------

const String8& Layer::getName() const {
    return mName;
}

bool Layer::getPremultipledAlpha() const {
    return mPremultipliedAlpha;
}

sp<IBinder> Layer::getHandle() {
    Mutex::Autolock _l(mLock);
    return new Handle(mFlinger, this);
}

// ---------------------------------------------------------------------------
// h/w composer set-up
// ---------------------------------------------------------------------------

bool Layer::createHwcLayer(HWComposer* hwc, int32_t hwcId) {
    LOG_ALWAYS_FATAL_IF(getBE().mHwcLayers.count(hwcId) != 0,
                        "Already have a layer for hwcId %d", hwcId);
    HWC2::Layer* layer = hwc->createLayer(hwcId);
    if (!layer) {
        return false;
    }
    LayerBE::HWCInfo& hwcInfo = getBE().mHwcLayers[hwcId];
    hwcInfo.hwc = hwc;
    hwcInfo.layer = layer;
    layer->setLayerDestroyedListener(
            [this, hwcId](HWC2::Layer* /*layer*/) { getBE().mHwcLayers.erase(hwcId); });
    return true;
}

bool Layer::destroyHwcLayer(int32_t hwcId) {
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        return false;
    }
    auto& hwcInfo = getBE().mHwcLayers[hwcId];
    LOG_ALWAYS_FATAL_IF(hwcInfo.layer == nullptr, "Attempt to destroy null layer");
    LOG_ALWAYS_FATAL_IF(hwcInfo.hwc == nullptr, "Missing HWComposer");
    hwcInfo.hwc->destroyLayer(hwcId, hwcInfo.layer);
    // The layer destroyed listener should have cleared the entry from
    // mHwcLayers. Verify that.
    LOG_ALWAYS_FATAL_IF(getBE().mHwcLayers.count(hwcId) != 0,
                        "Stale layer entry in getBE().mHwcLayers");
    return true;
}

void Layer::destroyAllHwcLayers() {
    size_t numLayers = getBE().mHwcLayers.size();
    for (size_t i = 0; i < numLayers; ++i) {
        LOG_ALWAYS_FATAL_IF(getBE().mHwcLayers.empty(), "destroyAllHwcLayers failed");
        destroyHwcLayer(getBE().mHwcLayers.begin()->first);
    }
    LOG_ALWAYS_FATAL_IF(!getBE().mHwcLayers.empty(),
                        "All hardware composer layers should have been destroyed");
}

Rect Layer::getContentCrop() const {
    // this is the crop rectangle that applies to the buffer
    // itself (as opposed to the window)
    Rect crop;
    if (!mCurrentCrop.isEmpty()) {
        // if the buffer crop is defined, we use that
        crop = mCurrentCrop;
    } else if (getBE().compositionInfo.mBuffer != nullptr) {
        // otherwise we use the whole buffer
        crop = getBE().compositionInfo.mBuffer->getBounds();
    } else {
        // if we don't have a buffer yet, we use an empty/invalid crop
        crop.makeInvalid();
    }
    return crop;
}

static Rect reduce(const Rect& win, const Region& exclude) {
    if (CC_LIKELY(exclude.isEmpty())) {
        return win;
    }
    if (exclude.isRect()) {
        return win.reduce(exclude.getBounds());
    }
    return Region(win).subtract(exclude).getBounds();
}

static FloatRect reduce(const FloatRect& win, const Region& exclude) {
    if (CC_LIKELY(exclude.isEmpty())) {
        return win;
    }
    // Convert through Rect (by rounding) for lack of FloatRegion
    return Region(Rect{win}).subtract(exclude).getBounds().toFloatRect();
}

Rect Layer::computeScreenBounds(bool reduceTransparentRegion) const {
    const Layer::State& s(getDrawingState());
    Rect win(s.active.w, s.active.h);

    if (!s.crop.isEmpty()) {
        win.intersect(s.crop, &win);
    }

    Transform t = getTransform();
    win = t.transform(win);

    if (!s.finalCrop.isEmpty()) {
        win.intersect(s.finalCrop, &win);
    }

    const sp<Layer>& p = mDrawingParent.promote();
    // Now we need to calculate the parent bounds, so we can clip ourselves to those.
    // When calculating the parent bounds for purposes of clipping,
    // we don't need to constrain the parent to its transparent region.
    // The transparent region is an optimization based on the
    // buffer contents of the layer, but does not affect the space allocated to
    // it by policy, and thus children should be allowed to extend into the
    // parent's transparent region. In fact one of the main uses, is to reduce
    // buffer allocation size in cases where a child window sits behind a main window
    // (by marking the hole in the parent window as a transparent region)
    if (p != nullptr) {
        Rect bounds = p->computeScreenBounds(false);
        bounds.intersect(win, &win);
    }

    if (reduceTransparentRegion) {
        auto const screenTransparentRegion = t.transform(s.activeTransparentRegion);
        win = reduce(win, screenTransparentRegion);
    }

    return win;
}

FloatRect Layer::computeBounds() const {
    const Layer::State& s(getDrawingState());
    return computeBounds(s.activeTransparentRegion);
}

FloatRect Layer::computeBounds(const Region& activeTransparentRegion) const {
    const Layer::State& s(getDrawingState());
    Rect win(s.active.w, s.active.h);

    if (!s.crop.isEmpty()) {
        win.intersect(s.crop, &win);
    }

    Rect bounds = win;
    const auto& p = mDrawingParent.promote();
    if (p != nullptr) {
        // Look in computeScreenBounds recursive call for explanation of
        // why we pass false here.
        bounds = p->computeScreenBounds(false /* reduceTransparentRegion */);
    }

    Transform t = getTransform();

    FloatRect floatWin = win.toFloatRect();
    if (p != nullptr) {
        floatWin = t.transform(floatWin);
        floatWin = floatWin.intersect(bounds.toFloatRect());
        floatWin = t.inverse().transform(floatWin);
    }

    // subtract the transparent region and snap to the bounds
    return reduce(floatWin, activeTransparentRegion);
}

Rect Layer::computeInitialCrop(const sp<const DisplayDevice>& hw) const {
    // the crop is the area of the window that gets cropped, but not
    // scaled in any ways.
    const State& s(getDrawingState());

    // apply the projection's clipping to the window crop in
    // layerstack space, and convert-back to layer space.
    // if there are no window scaling involved, this operation will map to full
    // pixels in the buffer.
    // FIXME: the 3 lines below can produce slightly incorrect clipping when we have
    // a viewport clipping and a window transform. we should use floating point to fix this.

    Rect activeCrop(s.active.w, s.active.h);
    if (!s.crop.isEmpty()) {
        activeCrop.intersect(s.crop, &activeCrop);
    }

    Transform t = getTransform();
    activeCrop = t.transform(activeCrop);
    if (!activeCrop.intersect(hw->getViewport(), &activeCrop)) {
        activeCrop.clear();
    }
    if (!s.finalCrop.isEmpty()) {
        if (!activeCrop.intersect(s.finalCrop, &activeCrop)) {
            activeCrop.clear();
        }
    }

    const auto& p = mDrawingParent.promote();
    if (p != nullptr) {
        auto parentCrop = p->computeInitialCrop(hw);
        activeCrop.intersect(parentCrop, &activeCrop);
    }

    return activeCrop;
}

FloatRect Layer::computeCrop(const sp<const DisplayDevice>& hw) const {
    // the content crop is the area of the content that gets scaled to the
    // layer's size. This is in buffer space.
    FloatRect crop = getContentCrop().toFloatRect();

    // In addition there is a WM-specified crop we pull from our drawing state.
    const State& s(getDrawingState());

    // Screen space to make reduction to parent crop clearer.
    Rect activeCrop = computeInitialCrop(hw);
    Transform t = getTransform();
    // Back to layer space to work with the content crop.
    activeCrop = t.inverse().transform(activeCrop);

    // This needs to be here as transform.transform(Rect) computes the
    // transformed rect and then takes the bounding box of the result before
    // returning. This means
    // transform.inverse().transform(transform.transform(Rect)) != Rect
    // in which case we need to make sure the final rect is clipped to the
    // display bounds.
    if (!activeCrop.intersect(Rect(s.active.w, s.active.h), &activeCrop)) {
        activeCrop.clear();
    }

    // subtract the transparent region and snap to the bounds
    activeCrop = reduce(activeCrop, s.activeTransparentRegion);

    // Transform the window crop to match the buffer coordinate system,
    // which means using the inverse of the current transform set on the
    // SurfaceFlingerConsumer.
    uint32_t invTransform = mCurrentTransform;
    if (getTransformToDisplayInverse()) {
        /*
         * the code below applies the primary display's inverse transform to the
         * buffer
         */
        uint32_t invTransformOrient = DisplayDevice::getPrimaryDisplayOrientationTransform();
        // calculate the inverse transform
        if (invTransformOrient & NATIVE_WINDOW_TRANSFORM_ROT_90) {
            invTransformOrient ^= NATIVE_WINDOW_TRANSFORM_FLIP_V | NATIVE_WINDOW_TRANSFORM_FLIP_H;
        }
        // and apply to the current transform
        invTransform = (Transform(invTransformOrient) * Transform(invTransform)).getOrientation();
    }

    int winWidth = s.active.w;
    int winHeight = s.active.h;
    if (invTransform & NATIVE_WINDOW_TRANSFORM_ROT_90) {
        // If the activeCrop has been rotate the ends are rotated but not
        // the space itself so when transforming ends back we can't rely on
        // a modification of the axes of rotation. To account for this we
        // need to reorient the inverse rotation in terms of the current
        // axes of rotation.
        bool is_h_flipped = (invTransform & NATIVE_WINDOW_TRANSFORM_FLIP_H) != 0;
        bool is_v_flipped = (invTransform & NATIVE_WINDOW_TRANSFORM_FLIP_V) != 0;
        if (is_h_flipped == is_v_flipped) {
            invTransform ^= NATIVE_WINDOW_TRANSFORM_FLIP_V | NATIVE_WINDOW_TRANSFORM_FLIP_H;
        }
        winWidth = s.active.h;
        winHeight = s.active.w;
    }
    const Rect winCrop = activeCrop.transform(invTransform, s.active.w, s.active.h);

    // below, crop is intersected with winCrop expressed in crop's coordinate space
    float xScale = crop.getWidth() / float(winWidth);
    float yScale = crop.getHeight() / float(winHeight);

    float insetL = winCrop.left * xScale;
    float insetT = winCrop.top * yScale;
    float insetR = (winWidth - winCrop.right) * xScale;
    float insetB = (winHeight - winCrop.bottom) * yScale;

    crop.left += insetL;
    crop.top += insetT;
    crop.right -= insetR;
    crop.bottom -= insetB;

    return crop;
}

void Layer::setGeometry(const sp<const DisplayDevice>& displayDevice, uint32_t z)
{
    const auto hwcId = displayDevice->getHwcDisplayId();
    auto& hwcInfo = getBE().mHwcLayers[hwcId];

    // enable this layer
    hwcInfo.forceClientComposition = false;

    if (isSecure() && !displayDevice->isSecure()) {
        hwcInfo.forceClientComposition = true;
    }

    auto& hwcLayer = hwcInfo.layer;

    // this gives us only the "orientation" component of the transform
    const State& s(getDrawingState());
    auto blendMode = HWC2::BlendMode::None;
    if (!isOpaque(s) || getAlpha() != 1.0f) {
        blendMode =
                mPremultipliedAlpha ? HWC2::BlendMode::Premultiplied : HWC2::BlendMode::Coverage;
    }
    auto error = hwcLayer->setBlendMode(blendMode);
    ALOGE_IF(error != HWC2::Error::None,
             "[%s] Failed to set blend mode %s:"
             " %s (%d)",
             mName.string(), to_string(blendMode).c_str(), to_string(error).c_str(),
             static_cast<int32_t>(error));

    // apply the layer's transform, followed by the display's global transform
    // here we're guaranteed that the layer's transform preserves rects
    Region activeTransparentRegion(s.activeTransparentRegion);
    Transform t = getTransform();
    if (!s.crop.isEmpty()) {
        Rect activeCrop(s.crop);
        activeCrop = t.transform(activeCrop);
        if (!activeCrop.intersect(displayDevice->getViewport(), &activeCrop)) {
            activeCrop.clear();
        }
        activeCrop = t.inverse().transform(activeCrop, true);
        // This needs to be here as transform.transform(Rect) computes the
        // transformed rect and then takes the bounding box of the result before
        // returning. This means
        // transform.inverse().transform(transform.transform(Rect)) != Rect
        // in which case we need to make sure the final rect is clipped to the
        // display bounds.
        if (!activeCrop.intersect(Rect(s.active.w, s.active.h), &activeCrop)) {
            activeCrop.clear();
        }
        // mark regions outside the crop as transparent
        activeTransparentRegion.orSelf(Rect(0, 0, s.active.w, activeCrop.top));
        activeTransparentRegion.orSelf(Rect(0, activeCrop.bottom, s.active.w, s.active.h));
        activeTransparentRegion.orSelf(Rect(0, activeCrop.top, activeCrop.left, activeCrop.bottom));
        activeTransparentRegion.orSelf(
                Rect(activeCrop.right, activeCrop.top, s.active.w, activeCrop.bottom));
    }

    // computeBounds returns a FloatRect to provide more accuracy during the
    // transformation. We then round upon constructing 'frame'.
    Rect frame{t.transform(computeBounds(activeTransparentRegion))};
    if (!s.finalCrop.isEmpty()) {
        if (!frame.intersect(s.finalCrop, &frame)) {
            frame.clear();
        }
    }
    if (!frame.intersect(displayDevice->getViewport(), &frame)) {
        frame.clear();
    }
    const Transform& tr(displayDevice->getTransform());
    Rect transformedFrame = tr.transform(frame);
    error = hwcLayer->setDisplayFrame(transformedFrame);
    if (error != HWC2::Error::None) {
        ALOGE("[%s] Failed to set display frame [%d, %d, %d, %d]: %s (%d)", mName.string(),
              transformedFrame.left, transformedFrame.top, transformedFrame.right,
              transformedFrame.bottom, to_string(error).c_str(), static_cast<int32_t>(error));
    } else {
        hwcInfo.displayFrame = transformedFrame;
    }

    FloatRect sourceCrop = computeCrop(displayDevice);
    error = hwcLayer->setSourceCrop(sourceCrop);
    if (error != HWC2::Error::None) {
        ALOGE("[%s] Failed to set source crop [%.3f, %.3f, %.3f, %.3f]: "
              "%s (%d)",
              mName.string(), sourceCrop.left, sourceCrop.top, sourceCrop.right, sourceCrop.bottom,
              to_string(error).c_str(), static_cast<int32_t>(error));
    } else {
        hwcInfo.sourceCrop = sourceCrop;
    }

    float alpha = static_cast<float>(getAlpha());
    error = hwcLayer->setPlaneAlpha(alpha);
    ALOGE_IF(error != HWC2::Error::None,
             "[%s] Failed to set plane alpha %.3f: "
             "%s (%d)",
             mName.string(), alpha, to_string(error).c_str(), static_cast<int32_t>(error));

    error = hwcLayer->setZOrder(z);
    ALOGE_IF(error != HWC2::Error::None, "[%s] Failed to set Z %u: %s (%d)", mName.string(), z,
             to_string(error).c_str(), static_cast<int32_t>(error));

    int type = s.type;
    int appId = s.appId;
    sp<Layer> parent = mDrawingParent.promote();
    if (parent.get()) {
        auto& parentState = parent->getDrawingState();
        if (parentState.type >= 0 || parentState.appId >= 0) {
            type = parentState.type;
            appId = parentState.appId;
        }
    }

    error = hwcLayer->setInfo(type, appId);
    ALOGE_IF(error != HWC2::Error::None, "[%s] Failed to set info (%d)", mName.string(),
             static_cast<int32_t>(error));

    /*
     * Transformations are applied in this order:
     * 1) buffer orientation/flip/mirror
     * 2) state transformation (window manager)
     * 3) layer orientation (screen orientation)
     * (NOTE: the matrices are multiplied in reverse order)
     */

    const Transform bufferOrientation(mCurrentTransform);
    Transform transform(tr * t * bufferOrientation);

    if (getTransformToDisplayInverse()) {
        /*
         * the code below applies the primary display's inverse transform to the
         * buffer
         */
        uint32_t invTransform = DisplayDevice::getPrimaryDisplayOrientationTransform();
        // calculate the inverse transform
        if (invTransform & NATIVE_WINDOW_TRANSFORM_ROT_90) {
            invTransform ^= NATIVE_WINDOW_TRANSFORM_FLIP_V | NATIVE_WINDOW_TRANSFORM_FLIP_H;
        }

        /*
         * Here we cancel out the orientation component of the WM transform.
         * The scaling and translate components are already included in our bounds
         * computation so it's enough to just omit it in the composition.
         * See comment in onDraw with ref to b/36727915 for why.
         */
        transform = Transform(invTransform) * tr * bufferOrientation;
    }

    // this gives us only the "orientation" component of the transform
    const uint32_t orientation = transform.getOrientation();
    if (orientation & Transform::ROT_INVALID) {
        // we can only handle simple transformation
        hwcInfo.forceClientComposition = true;
    } else {
        auto transform = static_cast<HWC2::Transform>(orientation);
        hwcInfo.transform = transform;
        auto error = hwcLayer->setTransform(transform);
        ALOGE_IF(error != HWC2::Error::None,
                 "[%s] Failed to set transform %s: "
                 "%s (%d)",
                 mName.string(), to_string(transform).c_str(), to_string(error).c_str(),
                 static_cast<int32_t>(error));
    }
}

void Layer::forceClientComposition(int32_t hwcId) {
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        ALOGE("forceClientComposition: no HWC layer found (%d)", hwcId);
        return;
    }

    getBE().mHwcLayers[hwcId].forceClientComposition = true;
}

bool Layer::getForceClientComposition(int32_t hwcId) {
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        ALOGE("getForceClientComposition: no HWC layer found (%d)", hwcId);
        return false;
    }

    return getBE().mHwcLayers[hwcId].forceClientComposition;
}

void Layer::updateCursorPosition(const sp<const DisplayDevice>& displayDevice) {
    auto hwcId = displayDevice->getHwcDisplayId();
    if (getBE().mHwcLayers.count(hwcId) == 0 ||
        getCompositionType(hwcId) != HWC2::Composition::Cursor) {
        return;
    }

    // This gives us only the "orientation" component of the transform
    const State& s(getCurrentState());

    // Apply the layer's transform, followed by the display's global transform
    // Here we're guaranteed that the layer's transform preserves rects
    Rect win(s.active.w, s.active.h);
    if (!s.crop.isEmpty()) {
        win.intersect(s.crop, &win);
    }
    // Subtract the transparent region and snap to the bounds
    Rect bounds = reduce(win, s.activeTransparentRegion);
    Rect frame(getTransform().transform(bounds));
    frame.intersect(displayDevice->getViewport(), &frame);
    if (!s.finalCrop.isEmpty()) {
        frame.intersect(s.finalCrop, &frame);
    }
    auto& displayTransform(displayDevice->getTransform());
    auto position = displayTransform.transform(frame);

    auto error = getBE().mHwcLayers[hwcId].layer->setCursorPosition(position.left,
                                                                              position.top);
    ALOGE_IF(error != HWC2::Error::None,
             "[%s] Failed to set cursor position "
             "to (%d, %d): %s (%d)",
             mName.string(), position.left, position.top, to_string(error).c_str(),
             static_cast<int32_t>(error));
}

// ---------------------------------------------------------------------------
// drawing...
// ---------------------------------------------------------------------------

void Layer::draw(const RenderArea& renderArea, const Region& clip) const {
    onDraw(renderArea, clip, false);
}

void Layer::draw(const RenderArea& renderArea, bool useIdentityTransform) const {
    onDraw(renderArea, Region(renderArea.getBounds()), useIdentityTransform);
}

void Layer::draw(const RenderArea& renderArea) const {
    onDraw(renderArea, Region(renderArea.getBounds()), false);
}

void Layer::clearWithOpenGL(const RenderArea& renderArea, float red, float green, float blue,
                            float alpha) const {
    auto& engine(mFlinger->getRenderEngine());
    computeGeometry(renderArea, getBE().mMesh, false);
    engine.setupFillWithColor(red, green, blue, alpha);
    engine.drawMesh(getBE().mMesh);
}

void Layer::clearWithOpenGL(const RenderArea& renderArea) const {
    clearWithOpenGL(renderArea, 0, 0, 0, 0);
}

void Layer::setCompositionType(int32_t hwcId, HWC2::Composition type, bool callIntoHwc) {
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        ALOGE("setCompositionType called without a valid HWC layer");
        return;
    }
    auto& hwcInfo = getBE().mHwcLayers[hwcId];
    auto& hwcLayer = hwcInfo.layer;
    ALOGV("setCompositionType(%" PRIx64 ", %s, %d)", hwcLayer->getId(), to_string(type).c_str(),
          static_cast<int>(callIntoHwc));
    if (hwcInfo.compositionType != type) {
        ALOGV("    actually setting");
        hwcInfo.compositionType = type;
        if (callIntoHwc) {
            auto error = hwcLayer->setCompositionType(type);
            ALOGE_IF(error != HWC2::Error::None,
                     "[%s] Failed to set "
                     "composition type %s: %s (%d)",
                     mName.string(), to_string(type).c_str(), to_string(error).c_str(),
                     static_cast<int32_t>(error));
        }
    }
}

HWC2::Composition Layer::getCompositionType(int32_t hwcId) const {
    if (hwcId == DisplayDevice::DISPLAY_ID_INVALID) {
        // If we're querying the composition type for a display that does not
        // have a HWC counterpart, then it will always be Client
        return HWC2::Composition::Client;
    }
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        ALOGE("getCompositionType called with an invalid HWC layer");
        return HWC2::Composition::Invalid;
    }
    return getBE().mHwcLayers.at(hwcId).compositionType;
}

void Layer::setClearClientTarget(int32_t hwcId, bool clear) {
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        ALOGE("setClearClientTarget called without a valid HWC layer");
        return;
    }
    getBE().mHwcLayers[hwcId].clearClientTarget = clear;
}

bool Layer::getClearClientTarget(int32_t hwcId) const {
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        ALOGE("getClearClientTarget called without a valid HWC layer");
        return false;
    }
    return getBE().mHwcLayers.at(hwcId).clearClientTarget;
}

bool Layer::addSyncPoint(const std::shared_ptr<SyncPoint>& point) {
    if (point->getFrameNumber() <= mCurrentFrameNumber) {
        // Don't bother with a SyncPoint, since we've already latched the
        // relevant frame
        return false;
    }

    Mutex::Autolock lock(mLocalSyncPointMutex);
    mLocalSyncPoints.push_back(point);
    return true;
}

void Layer::setFiltering(bool filtering) {
    mFiltering = filtering;
}

bool Layer::getFiltering() const {
    return mFiltering;
}

// ----------------------------------------------------------------------------
// local state
// ----------------------------------------------------------------------------

static void boundPoint(vec2* point, const Rect& crop) {
    if (point->x < crop.left) {
        point->x = crop.left;
    }
    if (point->x > crop.right) {
        point->x = crop.right;
    }
    if (point->y < crop.top) {
        point->y = crop.top;
    }
    if (point->y > crop.bottom) {
        point->y = crop.bottom;
    }
}

void Layer::computeGeometry(const RenderArea& renderArea, Mesh& mesh,
                            bool useIdentityTransform) const {
    const Layer::State& s(getDrawingState());
    const Transform renderAreaTransform(renderArea.getTransform());
    const uint32_t height = renderArea.getHeight();
    FloatRect win = computeBounds();

    vec2 lt = vec2(win.left, win.top);
    vec2 lb = vec2(win.left, win.bottom);
    vec2 rb = vec2(win.right, win.bottom);
    vec2 rt = vec2(win.right, win.top);

    Transform layerTransform = getTransform();
    if (!useIdentityTransform) {
        lt = layerTransform.transform(lt);
        lb = layerTransform.transform(lb);
        rb = layerTransform.transform(rb);
        rt = layerTransform.transform(rt);
    }

    if (!s.finalCrop.isEmpty()) {
        boundPoint(&lt, s.finalCrop);
        boundPoint(&lb, s.finalCrop);
        boundPoint(&rb, s.finalCrop);
        boundPoint(&rt, s.finalCrop);
    }

    Mesh::VertexArray<vec2> position(mesh.getPositionArray<vec2>());
    position[0] = renderAreaTransform.transform(lt);
    position[1] = renderAreaTransform.transform(lb);
    position[2] = renderAreaTransform.transform(rb);
    position[3] = renderAreaTransform.transform(rt);
    for (size_t i = 0; i < 4; i++) {
        position[i].y = height - position[i].y;
    }
}

bool Layer::isSecure() const {
    const Layer::State& s(mDrawingState);
    return (s.flags & layer_state_t::eLayerSecure);
}

void Layer::setVisibleRegion(const Region& visibleRegion) {
    // always called from main thread
    this->visibleRegion = visibleRegion;
}

void Layer::setCoveredRegion(const Region& coveredRegion) {
    // always called from main thread
    this->coveredRegion = coveredRegion;
}

void Layer::setVisibleNonTransparentRegion(const Region& setVisibleNonTransparentRegion) {
    // always called from main thread
    this->visibleNonTransparentRegion = setVisibleNonTransparentRegion;
}

void Layer::clearVisibilityRegions() {
    visibleRegion.clear();
    visibleNonTransparentRegion.clear();
    coveredRegion.clear();
}

// ----------------------------------------------------------------------------
// transaction
// ----------------------------------------------------------------------------

void Layer::pushPendingState() {
    if (!mCurrentState.modified) {
        return;
    }

    // If this transaction is waiting on the receipt of a frame, generate a sync
    // point and send it to the remote layer.
    if (mCurrentState.barrierLayer != nullptr) {
        sp<Layer> barrierLayer = mCurrentState.barrierLayer.promote();
        if (barrierLayer == nullptr) {
            ALOGE("[%s] Unable to promote barrier Layer.", mName.string());
            // If we can't promote the layer we are intended to wait on,
            // then it is expired or otherwise invalid. Allow this transaction
            // to be applied as per normal (no synchronization).
            mCurrentState.barrierLayer = nullptr;
        } else {
            auto syncPoint = std::make_shared<SyncPoint>(mCurrentState.frameNumber);
            if (barrierLayer->addSyncPoint(syncPoint)) {
                mRemoteSyncPoints.push_back(std::move(syncPoint));
            } else {
                // We already missed the frame we're supposed to synchronize
                // on, so go ahead and apply the state update
                mCurrentState.barrierLayer = nullptr;
            }
        }

        // Wake us up to check if the frame has been received
        setTransactionFlags(eTransactionNeeded);
        mFlinger->setTransactionFlags(eTraversalNeeded);
    }
    mPendingStates.push_back(mCurrentState);
    ATRACE_INT(mTransactionName.string(), mPendingStates.size());
}

void Layer::popPendingState(State* stateToCommit) {
    *stateToCommit = mPendingStates[0];

    mPendingStates.removeAt(0);
    ATRACE_INT(mTransactionName.string(), mPendingStates.size());
}

bool Layer::applyPendingStates(State* stateToCommit) {
    bool stateUpdateAvailable = false;
    while (!mPendingStates.empty()) {
        if (mPendingStates[0].barrierLayer != nullptr) {
            if (mRemoteSyncPoints.empty()) {
                // If we don't have a sync point for this, apply it anyway. It
                // will be visually wrong, but it should keep us from getting
                // into too much trouble.
                ALOGE("[%s] No local sync point found", mName.string());
                popPendingState(stateToCommit);
                stateUpdateAvailable = true;
                continue;
            }

            if (mRemoteSyncPoints.front()->getFrameNumber() != mPendingStates[0].frameNumber) {
                ALOGE("[%s] Unexpected sync point frame number found", mName.string());

                // Signal our end of the sync point and then dispose of it
                mRemoteSyncPoints.front()->setTransactionApplied();
                mRemoteSyncPoints.pop_front();
                continue;
            }

            if (mRemoteSyncPoints.front()->frameIsAvailable()) {
                // Apply the state update
                popPendingState(stateToCommit);
                stateUpdateAvailable = true;

                // Signal our end of the sync point and then dispose of it
                mRemoteSyncPoints.front()->setTransactionApplied();
                mRemoteSyncPoints.pop_front();
            } else {
                break;
            }
        } else {
            popPendingState(stateToCommit);
            stateUpdateAvailable = true;
        }
    }

    // If we still have pending updates, wake SurfaceFlinger back up and point
    // it at this layer so we can process them
    if (!mPendingStates.empty()) {
        setTransactionFlags(eTransactionNeeded);
        mFlinger->setTransactionFlags(eTraversalNeeded);
    }

    mCurrentState.modified = false;
    return stateUpdateAvailable;
}

uint32_t Layer::doTransaction(uint32_t flags) {
    ATRACE_CALL();

    pushPendingState();
    Layer::State c = getCurrentState();
    if (!applyPendingStates(&c)) {
        return 0;
    }

    const Layer::State& s(getDrawingState());

    const bool sizeChanged = (c.requested.w != s.requested.w) || (c.requested.h != s.requested.h);

    if (sizeChanged) {
        // the size changed, we need to ask our client to request a new buffer
        ALOGD_IF(DEBUG_RESIZE,
                 "doTransaction: geometry (layer=%p '%s'), tr=%02x, scalingMode=%d\n"
                 "  current={ active   ={ wh={%4u,%4u} crop={%4d,%4d,%4d,%4d} (%4d,%4d) }\n"
                 "            requested={ wh={%4u,%4u} }}\n"
                 "  drawing={ active   ={ wh={%4u,%4u} crop={%4d,%4d,%4d,%4d} (%4d,%4d) }\n"
                 "            requested={ wh={%4u,%4u} }}\n",
                 this, getName().string(), mCurrentTransform,
                 getEffectiveScalingMode(), c.active.w, c.active.h, c.crop.left, c.crop.top,
                 c.crop.right, c.crop.bottom, c.crop.getWidth(), c.crop.getHeight(), c.requested.w,
                 c.requested.h, s.active.w, s.active.h, s.crop.left, s.crop.top, s.crop.right,
                 s.crop.bottom, s.crop.getWidth(), s.crop.getHeight(), s.requested.w,
                 s.requested.h);

        // record the new size, form this point on, when the client request
        // a buffer, it'll get the new size.
        setDefaultBufferSize(c.requested.w, c.requested.h);
    }

    // Don't let Layer::doTransaction update the drawing state
    // if we have a pending resize, unless we are in fixed-size mode.
    // the drawing state will be updated only once we receive a buffer
    // with the correct size.
    //
    // In particular, we want to make sure the clip (which is part
    // of the geometry state) is latched together with the size but is
    // latched immediately when no resizing is involved.
    //
    // If a sideband stream is attached, however, we want to skip this
    // optimization so that transactions aren't missed when a buffer
    // never arrives
    //
    // In the case that we don't have a buffer we ignore other factors
    // and avoid entering the resizePending state. At a high level the
    // resizePending state is to avoid applying the state of the new buffer
    // to the old buffer. However in the state where we don't have an old buffer
    // there is no such concern but we may still be being used as a parent layer.
    const bool resizePending = ((c.requested.w != c.active.w) || (c.requested.h != c.active.h)) &&
            (getBE().compositionInfo.mBuffer != nullptr);
    if (!isFixedSize()) {
        if (resizePending && getBE().compositionInfo.hwc.sidebandStream == nullptr) {
            flags |= eDontUpdateGeometryState;
        }
    }

    // Here we apply various requested geometry states, depending on our
    // latching configuration. See Layer.h for a detailed discussion of
    // how geometry latching is controlled.
    if (!(flags & eDontUpdateGeometryState)) {
        Layer::State& editCurrentState(getCurrentState());

        // If mFreezeGeometryUpdates is true we are in the setGeometryAppliesWithResize
        // mode, which causes attributes which normally latch regardless of scaling mode,
        // to be delayed. We copy the requested state to the active state making sure
        // to respect these rules (again see Layer.h for a detailed discussion).
        //
        // There is an awkward asymmetry in the handling of the crop states in the position
        // states, as can be seen below. Largely this arises from position and transform
        // being stored in the same data structure while having different latching rules.
        // b/38182305
        //
        // Careful that "c" and editCurrentState may not begin as equivalent due to
        // applyPendingStates in the presence of deferred transactions.
        if (mFreezeGeometryUpdates) {
            float tx = c.active.transform.tx();
            float ty = c.active.transform.ty();
            c.active = c.requested;
            c.active.transform.set(tx, ty);
            editCurrentState.active = c.active;
        } else {
            editCurrentState.active = editCurrentState.requested;
            c.active = c.requested;
        }
    }

    if (s.active != c.active) {
        // invalidate and recompute the visible regions if needed
        flags |= Layer::eVisibleRegion;
    }

    if (c.sequence != s.sequence) {
        // invalidate and recompute the visible regions if needed
        flags |= eVisibleRegion;
        this->contentDirty = true;

        // we may use linear filtering, if the matrix scales us
        const uint8_t type = c.active.transform.getType();
        mNeedsFiltering = (!c.active.transform.preserveRects() || (type >= Transform::SCALE));
    }

    // If the layer is hidden, signal and clear out all local sync points so
    // that transactions for layers depending on this layer's frames becoming
    // visible are not blocked
    if (c.flags & layer_state_t::eLayerHidden) {
        clearSyncPoints();
    }

    // Commit the transaction
    commitTransaction(c);
    return flags;
}

void Layer::commitTransaction(const State& stateToCommit) {
    mDrawingState = stateToCommit;
}

uint32_t Layer::getTransactionFlags(uint32_t flags) {
    return android_atomic_and(~flags, &mTransactionFlags) & flags;
}

uint32_t Layer::setTransactionFlags(uint32_t flags) {
    return android_atomic_or(flags, &mTransactionFlags);
}

bool Layer::setPosition(float x, float y, bool immediate) {
    if (mCurrentState.requested.transform.tx() == x && mCurrentState.requested.transform.ty() == y)
        return false;
    mCurrentState.sequence++;

    // We update the requested and active position simultaneously because
    // we want to apply the position portion of the transform matrix immediately,
    // but still delay scaling when resizing a SCALING_MODE_FREEZE layer.
    mCurrentState.requested.transform.set(x, y);
    if (immediate && !mFreezeGeometryUpdates) {
        // Here we directly update the active state
        // unlike other setters, because we store it within
        // the transform, but use different latching rules.
        // b/38182305
        mCurrentState.active.transform.set(x, y);
    }
    mFreezeGeometryUpdates = mFreezeGeometryUpdates || !immediate;

    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

bool Layer::setChildLayer(const sp<Layer>& childLayer, int32_t z) {
    ssize_t idx = mCurrentChildren.indexOf(childLayer);
    if (idx < 0) {
        return false;
    }
    if (childLayer->setLayer(z)) {
        mCurrentChildren.removeAt(idx);
        mCurrentChildren.add(childLayer);
        return true;
    }
    return false;
}

bool Layer::setChildRelativeLayer(const sp<Layer>& childLayer,
        const sp<IBinder>& relativeToHandle, int32_t relativeZ) {
    ssize_t idx = mCurrentChildren.indexOf(childLayer);
    if (idx < 0) {
        return false;
    }
    if (childLayer->setRelativeLayer(relativeToHandle, relativeZ)) {
        mCurrentChildren.removeAt(idx);
        mCurrentChildren.add(childLayer);
        return true;
    }
    return false;
}

bool Layer::setLayer(int32_t z) {
    if (mCurrentState.z == z && !usingRelativeZ(LayerVector::StateSet::Current)) return false;
    mCurrentState.sequence++;
    mCurrentState.z = z;
    mCurrentState.modified = true;

    // Discard all relative layering.
    if (mCurrentState.zOrderRelativeOf != nullptr) {
        sp<Layer> strongRelative = mCurrentState.zOrderRelativeOf.promote();
        if (strongRelative != nullptr) {
            strongRelative->removeZOrderRelative(this);
        }
        mCurrentState.zOrderRelativeOf = nullptr;
    }
    setTransactionFlags(eTransactionNeeded);
    return true;
}

void Layer::removeZOrderRelative(const wp<Layer>& relative) {
    mCurrentState.zOrderRelatives.remove(relative);
    mCurrentState.sequence++;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
}

void Layer::addZOrderRelative(const wp<Layer>& relative) {
    mCurrentState.zOrderRelatives.add(relative);
    mCurrentState.modified = true;
    mCurrentState.sequence++;
    setTransactionFlags(eTransactionNeeded);
}

bool Layer::setRelativeLayer(const sp<IBinder>& relativeToHandle, int32_t relativeZ) {
    sp<Handle> handle = static_cast<Handle*>(relativeToHandle.get());
    if (handle == nullptr) {
        return false;
    }
    sp<Layer> relative = handle->owner.promote();
    if (relative == nullptr) {
        return false;
    }

    if (mCurrentState.z == relativeZ && usingRelativeZ(LayerVector::StateSet::Current) &&
            mCurrentState.zOrderRelativeOf == relative) {
        return false;
    }

    mCurrentState.sequence++;
    mCurrentState.modified = true;
    mCurrentState.z = relativeZ;

    auto oldZOrderRelativeOf = mCurrentState.zOrderRelativeOf.promote();
    if (oldZOrderRelativeOf != nullptr) {
        oldZOrderRelativeOf->removeZOrderRelative(this);
    }
    mCurrentState.zOrderRelativeOf = relative;
    relative->addZOrderRelative(this);

    setTransactionFlags(eTransactionNeeded);

    return true;
}

bool Layer::setSize(uint32_t w, uint32_t h) {
    if (mCurrentState.requested.w == w && mCurrentState.requested.h == h) return false;
    mCurrentState.requested.w = w;
    mCurrentState.requested.h = h;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}
bool Layer::setAlpha(float alpha) {
    if (mCurrentState.color.a == alpha) return false;
    mCurrentState.sequence++;
    mCurrentState.color.a = alpha;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

bool Layer::setColor(const half3& color) {
    if (color.r == mCurrentState.color.r && color.g == mCurrentState.color.g &&
        color.b == mCurrentState.color.b)
        return false;

    mCurrentState.sequence++;
    mCurrentState.color.r = color.r;
    mCurrentState.color.g = color.g;
    mCurrentState.color.b = color.b;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

bool Layer::setMatrix(const layer_state_t::matrix22_t& matrix) {
    mCurrentState.sequence++;
    mCurrentState.requested.transform.set(matrix.dsdx, matrix.dtdy, matrix.dtdx, matrix.dsdy);
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}
bool Layer::setTransparentRegionHint(const Region& transparent) {
    mCurrentState.requestedTransparentRegion = transparent;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}
bool Layer::setFlags(uint8_t flags, uint8_t mask) {
    const uint32_t newFlags = (mCurrentState.flags & ~mask) | (flags & mask);
    if (mCurrentState.flags == newFlags) return false;
    mCurrentState.sequence++;
    mCurrentState.flags = newFlags;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

bool Layer::setCrop(const Rect& crop, bool immediate) {
    if (mCurrentState.requestedCrop == crop) return false;
    mCurrentState.sequence++;
    mCurrentState.requestedCrop = crop;
    if (immediate && !mFreezeGeometryUpdates) {
        mCurrentState.crop = crop;
    }
    mFreezeGeometryUpdates = mFreezeGeometryUpdates || !immediate;

    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

bool Layer::setFinalCrop(const Rect& crop, bool immediate) {
    if (mCurrentState.requestedFinalCrop == crop) return false;
    mCurrentState.sequence++;
    mCurrentState.requestedFinalCrop = crop;
    if (immediate && !mFreezeGeometryUpdates) {
        mCurrentState.finalCrop = crop;
    }
    mFreezeGeometryUpdates = mFreezeGeometryUpdates || !immediate;

    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

bool Layer::setOverrideScalingMode(int32_t scalingMode) {
    if (scalingMode == mOverrideScalingMode) return false;
    mOverrideScalingMode = scalingMode;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

void Layer::setInfo(int32_t type, int32_t appId) {
    mCurrentState.appId = appId;
    mCurrentState.type = type;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
}

bool Layer::setLayerStack(uint32_t layerStack) {
    if (mCurrentState.layerStack == layerStack) return false;
    mCurrentState.sequence++;
    mCurrentState.layerStack = layerStack;
    mCurrentState.modified = true;
    setTransactionFlags(eTransactionNeeded);
    return true;
}

uint32_t Layer::getLayerStack() const {
    auto p = mDrawingParent.promote();
    if (p == nullptr) {
        return getDrawingState().layerStack;
    }
    return p->getLayerStack();
}

void Layer::deferTransactionUntil(const sp<Layer>& barrierLayer, uint64_t frameNumber) {
    mCurrentState.barrierLayer = barrierLayer;
    mCurrentState.frameNumber = frameNumber;
    // We don't set eTransactionNeeded, because just receiving a deferral
    // request without any other state updates shouldn't actually induce a delay
    mCurrentState.modified = true;
    pushPendingState();
    mCurrentState.barrierLayer = nullptr;
    mCurrentState.frameNumber = 0;
    mCurrentState.modified = false;
}

void Layer::deferTransactionUntil(const sp<IBinder>& barrierHandle, uint64_t frameNumber) {
    sp<Handle> handle = static_cast<Handle*>(barrierHandle.get());
    deferTransactionUntil(handle->owner.promote(), frameNumber);
}


// ----------------------------------------------------------------------------
// pageflip handling...
// ----------------------------------------------------------------------------

bool Layer::isHiddenByPolicy() const {
    const Layer::State& s(mDrawingState);
    const auto& parent = mDrawingParent.promote();
    if (parent != nullptr && parent->isHiddenByPolicy()) {
        return true;
    }
    return s.flags & layer_state_t::eLayerHidden;
}

uint32_t Layer::getEffectiveUsage(uint32_t usage) const {
    // TODO: should we do something special if mSecure is set?
    if (mProtectedByApp) {
        // need a hardware-protected path to external video sink
        usage |= GraphicBuffer::USAGE_PROTECTED;
    }
    if (mPotentialCursor) {
        usage |= GraphicBuffer::USAGE_CURSOR;
    }
    usage |= GraphicBuffer::USAGE_HW_COMPOSER;
    return usage;
}

void Layer::updateTransformHint(const sp<const DisplayDevice>& hw) const {
    uint32_t orientation = 0;
    if (!mFlinger->mDebugDisableTransformHint) {
        // The transform hint is used to improve performance, but we can
        // only have a single transform hint, it cannot
        // apply to all displays.
        const Transform& planeTransform(hw->getTransform());
        orientation = planeTransform.getOrientation();
        if (orientation & Transform::ROT_INVALID) {
            orientation = 0;
        }
    }
    setTransformHint(orientation);
}

// ----------------------------------------------------------------------------
// debugging
// ----------------------------------------------------------------------------

LayerDebugInfo Layer::getLayerDebugInfo() const {
    LayerDebugInfo info;
    const Layer::State& ds = getDrawingState();
    info.mName = getName();
    sp<Layer> parent = getParent();
    info.mParentName = (parent == nullptr ? std::string("none") : parent->getName().string());
    info.mType = String8(getTypeId());
    info.mTransparentRegion = ds.activeTransparentRegion;
    info.mVisibleRegion = visibleRegion;
    info.mSurfaceDamageRegion = surfaceDamageRegion;
    info.mLayerStack = getLayerStack();
    info.mX = ds.active.transform.tx();
    info.mY = ds.active.transform.ty();
    info.mZ = ds.z;
    info.mWidth = ds.active.w;
    info.mHeight = ds.active.h;
    info.mCrop = ds.crop;
    info.mFinalCrop = ds.finalCrop;
    info.mColor = ds.color;
    info.mFlags = ds.flags;
    info.mPixelFormat = getPixelFormat();
    info.mDataSpace = static_cast<android_dataspace>(mCurrentDataSpace);
    info.mMatrix[0][0] = ds.active.transform[0][0];
    info.mMatrix[0][1] = ds.active.transform[0][1];
    info.mMatrix[1][0] = ds.active.transform[1][0];
    info.mMatrix[1][1] = ds.active.transform[1][1];
    {
        sp<const GraphicBuffer> buffer = getBE().compositionInfo.mBuffer;
        if (buffer != 0) {
            info.mActiveBufferWidth = buffer->getWidth();
            info.mActiveBufferHeight = buffer->getHeight();
            info.mActiveBufferStride = buffer->getStride();
            info.mActiveBufferFormat = buffer->format;
        } else {
            info.mActiveBufferWidth = 0;
            info.mActiveBufferHeight = 0;
            info.mActiveBufferStride = 0;
            info.mActiveBufferFormat = 0;
        }
    }
    info.mNumQueuedFrames = getQueuedFrameCount();
    info.mRefreshPending = isBufferLatched();
    info.mIsOpaque = isOpaque(ds);
    info.mContentDirty = contentDirty;
    return info;
}

void Layer::miniDumpHeader(String8& result) {
    result.append("----------------------------------------");
    result.append("---------------------------------------\n");
    result.append(" Layer name\n");
    result.append("           Z | ");
    result.append(" Comp Type | ");
    result.append("  Disp Frame (LTRB) | ");
    result.append("         Source Crop (LTRB)\n");
    result.append("----------------------------------------");
    result.append("---------------------------------------\n");
}

void Layer::miniDump(String8& result, int32_t hwcId) const {
    if (getBE().mHwcLayers.count(hwcId) == 0) {
        return;
    }

    String8 name;
    if (mName.length() > 77) {
        std::string shortened;
        shortened.append(mName.string(), 36);
        shortened.append("[...]");
        shortened.append(mName.string() + (mName.length() - 36), 36);
        name = shortened.c_str();
    } else {
        name = mName;
    }

    result.appendFormat(" %s\n", name.string());

    const Layer::State& layerState(getDrawingState());
    const LayerBE::HWCInfo& hwcInfo = getBE().mHwcLayers.at(hwcId);
    if (layerState.zOrderRelativeOf != nullptr || mDrawingParent != nullptr) {
        result.appendFormat("  rel %6d | ", layerState.z);
    } else {
        result.appendFormat("  %10d | ", layerState.z);
    }
    result.appendFormat("%10s | ", to_string(getCompositionType(hwcId)).c_str());
    const Rect& frame = hwcInfo.displayFrame;
    result.appendFormat("%4d %4d %4d %4d | ", frame.left, frame.top, frame.right, frame.bottom);
    const FloatRect& crop = hwcInfo.sourceCrop;
    result.appendFormat("%6.1f %6.1f %6.1f %6.1f\n", crop.left, crop.top, crop.right, crop.bottom);

    result.append("- - - - - - - - - - - - - - - - - - - - ");
    result.append("- - - - - - - - - - - - - - - - - - - -\n");
}

void Layer::dumpFrameStats(String8& result) const {
    mFrameTracker.dumpStats(result);
}

void Layer::clearFrameStats() {
    mFrameTracker.clearStats();
}

void Layer::logFrameStats() {
    mFrameTracker.logAndResetStats(mName);
}

void Layer::getFrameStats(FrameStats* outStats) const {
    mFrameTracker.getStats(outStats);
}

void Layer::dumpFrameEvents(String8& result) {
    result.appendFormat("- Layer %s (%s, %p)\n", getName().string(), getTypeId(), this);
    Mutex::Autolock lock(mFrameEventHistoryMutex);
    mFrameEventHistory.checkFencesForCompletion();
    mFrameEventHistory.dump(result);
}

void Layer::onDisconnect() {
    Mutex::Autolock lock(mFrameEventHistoryMutex);
    mFrameEventHistory.onDisconnect();
    mTimeStats.onDisconnect(getName().c_str());
}

void Layer::addAndGetFrameTimestamps(const NewFrameEventsEntry* newTimestamps,
                                     FrameEventHistoryDelta* outDelta) {
    if (newTimestamps) {
        mTimeStats.setPostTime(getName().c_str(), newTimestamps->frameNumber,
                               newTimestamps->postedTime);
    }

    Mutex::Autolock lock(mFrameEventHistoryMutex);
    if (newTimestamps) {
        // If there are any unsignaled fences in the aquire timeline at this
        // point, the previously queued frame hasn't been latched yet. Go ahead
        // and try to get the signal time here so the syscall is taken out of
        // the main thread's critical path.
        mAcquireTimeline.updateSignalTimes();
        // Push the new fence after updating since it's likely still pending.
        mAcquireTimeline.push(newTimestamps->acquireFence);
        mFrameEventHistory.addQueue(*newTimestamps);
    }

    if (outDelta) {
        mFrameEventHistory.getAndResetDelta(outDelta);
    }
}

size_t Layer::getChildrenCount() const {
    size_t count = 0;
    for (const sp<Layer>& child : mCurrentChildren) {
        count += 1 + child->getChildrenCount();
    }
    return count;
}

void Layer::addChild(const sp<Layer>& layer) {
    mCurrentChildren.add(layer);
    layer->setParent(this);
}

ssize_t Layer::removeChild(const sp<Layer>& layer) {
    layer->setParent(nullptr);
    return mCurrentChildren.remove(layer);
}

bool Layer::reparentChildren(const sp<IBinder>& newParentHandle) {
    sp<Handle> handle = nullptr;
    sp<Layer> newParent = nullptr;
    if (newParentHandle == nullptr) {
        return false;
    }
    handle = static_cast<Handle*>(newParentHandle.get());
    newParent = handle->owner.promote();
    if (newParent == nullptr) {
        ALOGE("Unable to promote Layer handle");
        return false;
    }

    for (const sp<Layer>& child : mCurrentChildren) {
        newParent->addChild(child);

        sp<Client> client(child->mClientRef.promote());
        if (client != nullptr) {
            client->updateParent(newParent);
        }
    }
    mCurrentChildren.clear();

    return true;
}

void Layer::setChildrenDrawingParent(const sp<Layer>& newParent) {
    for (const sp<Layer>& child : mDrawingChildren) {
        child->mDrawingParent = newParent;
    }
}

bool Layer::reparent(const sp<IBinder>& newParentHandle) {
    if (newParentHandle == nullptr) {
        return false;
    }

    auto handle = static_cast<Handle*>(newParentHandle.get());
    sp<Layer> newParent = handle->owner.promote();
    if (newParent == nullptr) {
        ALOGE("Unable to promote Layer handle");
        return false;
    }

    sp<Layer> parent = getParent();
    if (parent != nullptr) {
        parent->removeChild(this);
    }
    newParent->addChild(this);

    sp<Client> client(mClientRef.promote());
    sp<Client> newParentClient(newParent->mClientRef.promote());

    if (client != newParentClient) {
        client->updateParent(newParent);
    }

    return true;
}

bool Layer::detachChildren() {
    for (const sp<Layer>& child : mCurrentChildren) {
        sp<Client> parentClient = mClientRef.promote();
        sp<Client> client(child->mClientRef.promote());
        if (client != nullptr && parentClient != client) {
            client->detachLayer(child.get());
            child->detachChildren();
        }
    }

    return true;
}

bool Layer::isLegacyDataSpace() const {
    // return true when no higher bits are set
    return !(mCurrentDataSpace & (ui::Dataspace::STANDARD_MASK |
                ui::Dataspace::TRANSFER_MASK | ui::Dataspace::RANGE_MASK));
}

void Layer::setParent(const sp<Layer>& layer) {
    mCurrentParent = layer;
}

void Layer::clearSyncPoints() {
    for (const auto& child : mCurrentChildren) {
        child->clearSyncPoints();
    }

    Mutex::Autolock lock(mLocalSyncPointMutex);
    for (auto& point : mLocalSyncPoints) {
        point->setFrameAvailable();
    }
    mLocalSyncPoints.clear();
}

int32_t Layer::getZ() const {
    return mDrawingState.z;
}

bool Layer::usingRelativeZ(LayerVector::StateSet stateSet) {
    const bool useDrawing = stateSet == LayerVector::StateSet::Drawing;
    const State& state = useDrawing ? mDrawingState : mCurrentState;
    return state.zOrderRelativeOf != nullptr;
}

__attribute__((no_sanitize("unsigned-integer-overflow"))) LayerVector Layer::makeTraversalList(
        LayerVector::StateSet stateSet, bool* outSkipRelativeZUsers) {
    LOG_ALWAYS_FATAL_IF(stateSet == LayerVector::StateSet::Invalid,
                        "makeTraversalList received invalid stateSet");
    const bool useDrawing = stateSet == LayerVector::StateSet::Drawing;
    const LayerVector& children = useDrawing ? mDrawingChildren : mCurrentChildren;
    const State& state = useDrawing ? mDrawingState : mCurrentState;

    if (state.zOrderRelatives.size() == 0) {
        *outSkipRelativeZUsers = true;
        return children;
    }

    LayerVector traverse(stateSet);
    for (const wp<Layer>& weakRelative : state.zOrderRelatives) {
        sp<Layer> strongRelative = weakRelative.promote();
        if (strongRelative != nullptr) {
            traverse.add(strongRelative);
        }
    }

    for (const sp<Layer>& child : children) {
        const State& childState = useDrawing ? child->mDrawingState : child->mCurrentState;
        if (childState.zOrderRelativeOf != nullptr) {
            continue;
        }
        traverse.add(child);
    }

    return traverse;
}

/**
 * Negatively signed relatives are before 'this' in Z-order.
 */
void Layer::traverseInZOrder(LayerVector::StateSet stateSet, const LayerVector::Visitor& visitor) {
    // In the case we have other layers who are using a relative Z to us, makeTraversalList will
    // produce a new list for traversing, including our relatives, and not including our children
    // who are relatives of another surface. In the case that there are no relative Z,
    // makeTraversalList returns our children directly to avoid significant overhead.
    // However in this case we need to take the responsibility for filtering children which
    // are relatives of another surface here.
    bool skipRelativeZUsers = false;
    const LayerVector list = makeTraversalList(stateSet, &skipRelativeZUsers);

    size_t i = 0;
    for (; i < list.size(); i++) {
        const auto& relative = list[i];
        if (skipRelativeZUsers && relative->usingRelativeZ(stateSet)) {
            continue;
        }

        if (relative->getZ() >= 0) {
            break;
        }
        relative->traverseInZOrder(stateSet, visitor);
    }

    visitor(this);
    for (; i < list.size(); i++) {
        const auto& relative = list[i];

        if (skipRelativeZUsers && relative->usingRelativeZ(stateSet)) {
            continue;
        }
        relative->traverseInZOrder(stateSet, visitor);
    }
}

/**
 * Positively signed relatives are before 'this' in reverse Z-order.
 */
void Layer::traverseInReverseZOrder(LayerVector::StateSet stateSet,
                                    const LayerVector::Visitor& visitor) {
    // See traverseInZOrder for documentation.
    bool skipRelativeZUsers = false;
    LayerVector list = makeTraversalList(stateSet, &skipRelativeZUsers);

    int32_t i = 0;
    for (i = int32_t(list.size()) - 1; i >= 0; i--) {
        const auto& relative = list[i];

        if (skipRelativeZUsers && relative->usingRelativeZ(stateSet)) {
            continue;
        }

        if (relative->getZ() < 0) {
            break;
        }
        relative->traverseInReverseZOrder(stateSet, visitor);
    }
    visitor(this);
    for (; i >= 0; i--) {
        const auto& relative = list[i];

        if (skipRelativeZUsers && relative->usingRelativeZ(stateSet)) {
            continue;
        }

        relative->traverseInReverseZOrder(stateSet, visitor);
    }
}

LayerVector Layer::makeChildrenTraversalList(LayerVector::StateSet stateSet,
                                             const std::vector<Layer*>& layersInTree) {
    LOG_ALWAYS_FATAL_IF(stateSet == LayerVector::StateSet::Invalid,
                        "makeTraversalList received invalid stateSet");
    const bool useDrawing = stateSet == LayerVector::StateSet::Drawing;
    const LayerVector& children = useDrawing ? mDrawingChildren : mCurrentChildren;
    const State& state = useDrawing ? mDrawingState : mCurrentState;

    LayerVector traverse(stateSet);
    for (const wp<Layer>& weakRelative : state.zOrderRelatives) {
        sp<Layer> strongRelative = weakRelative.promote();
        // Only add relative layers that are also descendents of the top most parent of the tree.
        // If a relative layer is not a descendent, then it should be ignored.
        if (std::binary_search(layersInTree.begin(), layersInTree.end(), strongRelative.get())) {
            traverse.add(strongRelative);
        }
    }

    for (const sp<Layer>& child : children) {
        const State& childState = useDrawing ? child->mDrawingState : child->mCurrentState;
        // If a layer has a relativeOf layer, only ignore if the layer it's relative to is a
        // descendent of the top most parent of the tree. If it's not a descendent, then just add
        // the child here since it won't be added later as a relative.
        if (std::binary_search(layersInTree.begin(), layersInTree.end(),
                               childState.zOrderRelativeOf.promote().get())) {
            continue;
        }
        traverse.add(child);
    }

    return traverse;
}

void Layer::traverseChildrenInZOrderInner(const std::vector<Layer*>& layersInTree,
                                          LayerVector::StateSet stateSet,
                                          const LayerVector::Visitor& visitor) {
    const LayerVector list = makeChildrenTraversalList(stateSet, layersInTree);

    size_t i = 0;
    for (; i < list.size(); i++) {
        const auto& relative = list[i];
        if (relative->getZ() >= 0) {
            break;
        }
        relative->traverseChildrenInZOrderInner(layersInTree, stateSet, visitor);
    }

    visitor(this);
    for (; i < list.size(); i++) {
        const auto& relative = list[i];
        relative->traverseChildrenInZOrderInner(layersInTree, stateSet, visitor);
    }
}

std::vector<Layer*> Layer::getLayersInTree(LayerVector::StateSet stateSet) {
    const bool useDrawing = stateSet == LayerVector::StateSet::Drawing;
    const LayerVector& children = useDrawing ? mDrawingChildren : mCurrentChildren;

    std::vector<Layer*> layersInTree = {this};
    for (size_t i = 0; i < children.size(); i++) {
        const auto& child = children[i];
        std::vector<Layer*> childLayers = child->getLayersInTree(stateSet);
        layersInTree.insert(layersInTree.end(), childLayers.cbegin(), childLayers.cend());
    }

    return layersInTree;
}

void Layer::traverseChildrenInZOrder(LayerVector::StateSet stateSet,
                                     const LayerVector::Visitor& visitor) {
    std::vector<Layer*> layersInTree = getLayersInTree(stateSet);
    std::sort(layersInTree.begin(), layersInTree.end());
    traverseChildrenInZOrderInner(layersInTree, stateSet, visitor);
}

Transform Layer::getTransform() const {
    Transform t;
    const auto& p = mDrawingParent.promote();
    if (p != nullptr) {
        t = p->getTransform();

        // If the parent is not using NATIVE_WINDOW_SCALING_MODE_FREEZE (e.g.
        // it isFixedSize) then there may be additional scaling not accounted
        // for in the transform. We need to mirror this scaling in child surfaces
        // or we will break the contract where WM can treat child surfaces as
        // pixels in the parent surface.
        if (p->isFixedSize() && p->getBE().compositionInfo.mBuffer != nullptr) {
            int bufferWidth;
            int bufferHeight;
            if ((p->mCurrentTransform & NATIVE_WINDOW_TRANSFORM_ROT_90) == 0) {
                bufferWidth = p->getBE().compositionInfo.mBuffer->getWidth();
                bufferHeight = p->getBE().compositionInfo.mBuffer->getHeight();
            } else {
                bufferHeight = p->getBE().compositionInfo.mBuffer->getWidth();
                bufferWidth = p->getBE().compositionInfo.mBuffer->getHeight();
            }
            float sx = p->getDrawingState().active.w / static_cast<float>(bufferWidth);
            float sy = p->getDrawingState().active.h / static_cast<float>(bufferHeight);
            Transform extraParentScaling;
            extraParentScaling.set(sx, 0, 0, sy);
            t = t * extraParentScaling;
        }
    }
    return t * getDrawingState().active.transform;
}

half Layer::getAlpha() const {
    const auto& p = mDrawingParent.promote();

    half parentAlpha = (p != nullptr) ? p->getAlpha() : 1.0_hf;
    return parentAlpha * getDrawingState().color.a;
}

half4 Layer::getColor() const {
    const half4 color(getDrawingState().color);
    return half4(color.r, color.g, color.b, getAlpha());
}

void Layer::commitChildList() {
    for (size_t i = 0; i < mCurrentChildren.size(); i++) {
        const auto& child = mCurrentChildren[i];
        child->commitChildList();
    }
    mDrawingChildren = mCurrentChildren;
    mDrawingParent = mCurrentParent;
}

void Layer::writeToProto(LayerProto* layerInfo, LayerVector::StateSet stateSet) {
    const bool useDrawing = stateSet == LayerVector::StateSet::Drawing;
    const LayerVector& children = useDrawing ? mDrawingChildren : mCurrentChildren;
    const State& state = useDrawing ? mDrawingState : mCurrentState;

    Transform requestedTransform = state.active.transform;
    Transform transform = getTransform();

    layerInfo->set_id(sequence);
    layerInfo->set_name(getName().c_str());
    layerInfo->set_type(String8(getTypeId()));

    for (const auto& child : children) {
        layerInfo->add_children(child->sequence);
    }

    for (const wp<Layer>& weakRelative : state.zOrderRelatives) {
        sp<Layer> strongRelative = weakRelative.promote();
        if (strongRelative != nullptr) {
            layerInfo->add_relatives(strongRelative->sequence);
        }
    }

    LayerProtoHelper::writeToProto(state.activeTransparentRegion,
                                   layerInfo->mutable_transparent_region());
    LayerProtoHelper::writeToProto(visibleRegion, layerInfo->mutable_visible_region());
    LayerProtoHelper::writeToProto(surfaceDamageRegion, layerInfo->mutable_damage_region());

    layerInfo->set_layer_stack(getLayerStack());
    layerInfo->set_z(state.z);

    PositionProto* position = layerInfo->mutable_position();
    position->set_x(transform.tx());
    position->set_y(transform.ty());

    PositionProto* requestedPosition = layerInfo->mutable_requested_position();
    requestedPosition->set_x(requestedTransform.tx());
    requestedPosition->set_y(requestedTransform.ty());

    SizeProto* size = layerInfo->mutable_size();
    size->set_w(state.active.w);
    size->set_h(state.active.h);

    LayerProtoHelper::writeToProto(state.crop, layerInfo->mutable_crop());
    LayerProtoHelper::writeToProto(state.finalCrop, layerInfo->mutable_final_crop());

    layerInfo->set_is_opaque(isOpaque(state));
    layerInfo->set_invalidate(contentDirty);

    // XXX (b/79210409) mCurrentDataSpace is not protected
    layerInfo->set_dataspace(dataspaceDetails(static_cast<android_dataspace>(mCurrentDataSpace)));

    layerInfo->set_pixel_format(decodePixelFormat(getPixelFormat()));
    LayerProtoHelper::writeToProto(getColor(), layerInfo->mutable_color());
    LayerProtoHelper::writeToProto(state.color, layerInfo->mutable_requested_color());
    layerInfo->set_flags(state.flags);

    LayerProtoHelper::writeToProto(transform, layerInfo->mutable_transform());
    LayerProtoHelper::writeToProto(requestedTransform, layerInfo->mutable_requested_transform());

    auto parent = useDrawing ? mDrawingParent.promote() : mCurrentParent.promote();
    if (parent != nullptr) {
        layerInfo->set_parent(parent->sequence);
    }

    auto zOrderRelativeOf = state.zOrderRelativeOf.promote();
    if (zOrderRelativeOf != nullptr) {
        layerInfo->set_z_order_relative_of(zOrderRelativeOf->sequence);
    }

    // XXX getBE().compositionInfo.mBuffer is not protected
    auto buffer = getBE().compositionInfo.mBuffer;
    if (buffer != nullptr) {
        LayerProtoHelper::writeToProto(buffer, layerInfo->mutable_active_buffer());
    }

    layerInfo->set_queued_frames(getQueuedFrameCount());
    layerInfo->set_refresh_pending(isBufferLatched());
    layerInfo->set_window_type(state.type);
    layerInfo->set_app_id(state.appId);
}

void Layer::writeToProto(LayerProto* layerInfo, int32_t hwcId) {
    writeToProto(layerInfo, LayerVector::StateSet::Drawing);

    const auto& hwcInfo = getBE().mHwcLayers.at(hwcId);

    const Rect& frame = hwcInfo.displayFrame;
    LayerProtoHelper::writeToProto(frame, layerInfo->mutable_hwc_frame());

    const FloatRect& crop = hwcInfo.sourceCrop;
    LayerProtoHelper::writeToProto(crop, layerInfo->mutable_hwc_crop());

    const int32_t transform = static_cast<int32_t>(hwcInfo.transform);
    layerInfo->set_hwc_transform(transform);

    const int32_t compositionType = static_cast<int32_t>(hwcInfo.compositionType);
    layerInfo->set_hwc_composition_type(compositionType);

    if (std::strcmp(getTypeId(), "BufferLayer") == 0 &&
        static_cast<BufferLayer*>(this)->isProtected()) {
        layerInfo->set_is_protected(true);
    } else {
        layerInfo->set_is_protected(false);
    }
}

// ---------------------------------------------------------------------------

}; // namespace android

#if defined(__gl_h_)
#error "don't include gl/gl.h in this file"
#endif

#if defined(__gl2_h_)
#error "don't include gl2/gl2.h in this file"
#endif
