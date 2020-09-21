/*
 * Copyright (C) 2010 The Android Open Source Project
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

#undef LOG_TAG
#define LOG_TAG "BufferLayerConsumer"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
//#define LOG_NDEBUG 0

#include "BufferLayerConsumer.h"

#include "DispSync.h"
#include "Layer.h"
#include "RenderEngine/Image.h"
#include "RenderEngine/RenderEngine.h"

#include <inttypes.h>

#include <cutils/compiler.h>

#include <hardware/hardware.h>

#include <math/mat4.h>

#include <gui/BufferItem.h>
#include <gui/GLConsumer.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <private/gui/ComposerService.h>
#include <private/gui/SyncFeatures.h>

#include <utils/Log.h>
#include <utils/String8.h>
#include <utils/Trace.h>

namespace android {

// Macros for including the BufferLayerConsumer name in log messages
#define BLC_LOGV(x, ...) ALOGV("[%s] " x, mName.string(), ##__VA_ARGS__)
#define BLC_LOGD(x, ...) ALOGD("[%s] " x, mName.string(), ##__VA_ARGS__)
//#define BLC_LOGI(x, ...) ALOGI("[%s] " x, mName.string(), ##__VA_ARGS__)
#define BLC_LOGW(x, ...) ALOGW("[%s] " x, mName.string(), ##__VA_ARGS__)
#define BLC_LOGE(x, ...) ALOGE("[%s] " x, mName.string(), ##__VA_ARGS__)

static const mat4 mtxIdentity;

BufferLayerConsumer::BufferLayerConsumer(const sp<IGraphicBufferConsumer>& bq,
                                         RE::RenderEngine& engine, uint32_t tex, Layer* layer)
      : ConsumerBase(bq, false),
        mCurrentCrop(Rect::EMPTY_RECT),
        mCurrentTransform(0),
        mCurrentScalingMode(NATIVE_WINDOW_SCALING_MODE_FREEZE),
        mCurrentFence(Fence::NO_FENCE),
        mCurrentTimestamp(0),
        mCurrentDataSpace(ui::Dataspace::UNKNOWN),
        mCurrentFrameNumber(0),
        mCurrentTransformToDisplayInverse(false),
        mCurrentSurfaceDamage(),
        mCurrentApi(0),
        mDefaultWidth(1),
        mDefaultHeight(1),
        mFilteringEnabled(true),
        mRE(engine),
        mTexName(tex),
        mLayer(layer),
        mCurrentTexture(BufferQueue::INVALID_BUFFER_SLOT) {
    BLC_LOGV("BufferLayerConsumer");

    memcpy(mCurrentTransformMatrix, mtxIdentity.asArray(), sizeof(mCurrentTransformMatrix));

    mConsumer->setConsumerUsageBits(DEFAULT_USAGE_FLAGS);
}

status_t BufferLayerConsumer::setDefaultBufferSize(uint32_t w, uint32_t h) {
    Mutex::Autolock lock(mMutex);
    if (mAbandoned) {
        BLC_LOGE("setDefaultBufferSize: BufferLayerConsumer is abandoned!");
        return NO_INIT;
    }
    mDefaultWidth = w;
    mDefaultHeight = h;
    return mConsumer->setDefaultBufferSize(w, h);
}

void BufferLayerConsumer::setContentsChangedListener(const wp<ContentsChangedListener>& listener) {
    setFrameAvailableListener(listener);
    Mutex::Autolock lock(mMutex);
    mContentsChangedListener = listener;
}

// We need to determine the time when a buffer acquired now will be
// displayed.  This can be calculated:
//   time when previous buffer's actual-present fence was signaled
//    + current display refresh rate * HWC latency
//    + a little extra padding
//
// Buffer producers are expected to set their desired presentation time
// based on choreographer time stamps, which (coming from vsync events)
// will be slightly later then the actual-present timing.  If we get a
// desired-present time that is unintentionally a hair after the next
// vsync, we'll hold the frame when we really want to display it.  We
// need to take the offset between actual-present and reported-vsync
// into account.
//
// If the system is configured without a DispSync phase offset for the app,
// we also want to throw in a bit of padding to avoid edge cases where we
// just barely miss.  We want to do it here, not in every app.  A major
// source of trouble is the app's use of the display's ideal refresh time
// (via Display.getRefreshRate()), which could be off of the actual refresh
// by a few percent, with the error multiplied by the number of frames
// between now and when the buffer should be displayed.
//
// If the refresh reported to the app has a phase offset, we shouldn't need
// to tweak anything here.
nsecs_t BufferLayerConsumer::computeExpectedPresent(const DispSync& dispSync) {
    // The HWC doesn't currently have a way to report additional latency.
    // Assume that whatever we submit now will appear right after the flip.
    // For a smart panel this might be 1.  This is expressed in frames,
    // rather than time, because we expect to have a constant frame delay
    // regardless of the refresh rate.
    const uint32_t hwcLatency = 0;

    // Ask DispSync when the next refresh will be (CLOCK_MONOTONIC).
    const nsecs_t nextRefresh = dispSync.computeNextRefresh(hwcLatency);

    // The DispSync time is already adjusted for the difference between
    // vsync and reported-vsync (SurfaceFlinger::dispSyncPresentTimeOffset), so
    // we don't need to factor that in here.  Pad a little to avoid
    // weird effects if apps might be requesting times right on the edge.
    nsecs_t extraPadding = 0;
    if (SurfaceFlinger::vsyncPhaseOffsetNs == 0) {
        extraPadding = 1000000; // 1ms (6% of 60Hz)
    }

    return nextRefresh + extraPadding;
}

status_t BufferLayerConsumer::updateTexImage(BufferRejecter* rejecter, const DispSync& dispSync,
                                             bool* autoRefresh, bool* queuedBuffer,
                                             uint64_t maxFrameNumber) {
    ATRACE_CALL();
    BLC_LOGV("updateTexImage");
    Mutex::Autolock lock(mMutex);

    if (mAbandoned) {
        BLC_LOGE("updateTexImage: BufferLayerConsumer is abandoned!");
        return NO_INIT;
    }

    // Make sure RenderEngine is current
    if (!mRE.isCurrent()) {
        BLC_LOGE("updateTexImage: RenderEngine is not current");
        return INVALID_OPERATION;
    }

    BufferItem item;

    // Acquire the next buffer.
    // In asynchronous mode the list is guaranteed to be one buffer
    // deep, while in synchronous mode we use the oldest buffer.
    status_t err = acquireBufferLocked(&item, computeExpectedPresent(dispSync), maxFrameNumber);
    if (err != NO_ERROR) {
        if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
            err = NO_ERROR;
        } else if (err == BufferQueue::PRESENT_LATER) {
            // return the error, without logging
        } else {
            BLC_LOGE("updateTexImage: acquire failed: %s (%d)", strerror(-err), err);
        }
        return err;
    }

    if (autoRefresh) {
        *autoRefresh = item.mAutoRefresh;
    }

    if (queuedBuffer) {
        *queuedBuffer = item.mQueuedBuffer;
    }

    // We call the rejecter here, in case the caller has a reason to
    // not accept this buffer.  This is used by SurfaceFlinger to
    // reject buffers which have the wrong size
    int slot = item.mSlot;
    if (rejecter && rejecter->reject(mSlots[slot].mGraphicBuffer, item)) {
        releaseBufferLocked(slot, mSlots[slot].mGraphicBuffer);
        return BUFFER_REJECTED;
    }

    // Release the previous buffer.
    err = updateAndReleaseLocked(item, &mPendingRelease);
    if (err != NO_ERROR) {
        return err;
    }

    if (!SyncFeatures::getInstance().useNativeFenceSync()) {
        // Bind the new buffer to the GL texture.
        //
        // Older devices require the "implicit" synchronization provided
        // by glEGLImageTargetTexture2DOES, which this method calls.  Newer
        // devices will either call this in Layer::onDraw, or (if it's not
        // a GL-composited layer) not at all.
        err = bindTextureImageLocked();
    }

    return err;
}

status_t BufferLayerConsumer::bindTextureImage() {
    Mutex::Autolock lock(mMutex);
    return bindTextureImageLocked();
}

void BufferLayerConsumer::setReleaseFence(const sp<Fence>& fence) {
    if (!fence->isValid()) {
        return;
    }

    auto slot = mPendingRelease.isPending ? mPendingRelease.currentTexture : mCurrentTexture;
    if (slot == BufferQueue::INVALID_BUFFER_SLOT) {
        return;
    }

    auto buffer = mPendingRelease.isPending ? mPendingRelease.graphicBuffer
                                            : mCurrentTextureImage->graphicBuffer();
    auto err = addReleaseFence(slot, buffer, fence);
    if (err != OK) {
        BLC_LOGE("setReleaseFence: failed to add the fence: %s (%d)", strerror(-err), err);
    }
}

bool BufferLayerConsumer::releasePendingBuffer() {
    if (!mPendingRelease.isPending) {
        BLC_LOGV("Pending buffer already released");
        return false;
    }
    BLC_LOGV("Releasing pending buffer");
    Mutex::Autolock lock(mMutex);
    status_t result =
            releaseBufferLocked(mPendingRelease.currentTexture, mPendingRelease.graphicBuffer);
    if (result < NO_ERROR) {
        BLC_LOGE("releasePendingBuffer failed: %s (%d)", strerror(-result), result);
    }
    mPendingRelease = PendingRelease();
    return true;
}

sp<Fence> BufferLayerConsumer::getPrevFinalReleaseFence() const {
    Mutex::Autolock lock(mMutex);
    return ConsumerBase::mPrevFinalReleaseFence;
}

status_t BufferLayerConsumer::acquireBufferLocked(BufferItem* item, nsecs_t presentWhen,
                                                  uint64_t maxFrameNumber) {
    status_t err = ConsumerBase::acquireBufferLocked(item, presentWhen, maxFrameNumber);
    if (err != NO_ERROR) {
        return err;
    }

    // If item->mGraphicBuffer is not null, this buffer has not been acquired
    // before, so any prior EglImage created is using a stale buffer. This
    // replaces any old EglImage with a new one (using the new buffer).
    if (item->mGraphicBuffer != nullptr) {
        mImages[item->mSlot] = new Image(item->mGraphicBuffer, mRE);
    }

    return NO_ERROR;
}

bool BufferLayerConsumer::canUseImageCrop(const Rect& crop) const {
    // If the crop rect is not at the origin, we can't set the crop on the
    // EGLImage because that's not allowed by the EGL_ANDROID_image_crop
    // extension.  In the future we can add a layered extension that
    // removes this restriction if there is hardware that can support it.
    return mRE.supportsImageCrop() && crop.left == 0 && crop.top == 0;
}

status_t BufferLayerConsumer::updateAndReleaseLocked(const BufferItem& item,
                                                     PendingRelease* pendingRelease) {
    status_t err = NO_ERROR;

    int slot = item.mSlot;

    // Do whatever sync ops we need to do before releasing the old slot.
    if (slot != mCurrentTexture) {
        err = syncForReleaseLocked();
        if (err != NO_ERROR) {
            // Release the buffer we just acquired.  It's not safe to
            // release the old buffer, so instead we just drop the new frame.
            // As we are still under lock since acquireBuffer, it is safe to
            // release by slot.
            releaseBufferLocked(slot, mSlots[slot].mGraphicBuffer);
            return err;
        }
    }

    BLC_LOGV("updateAndRelease: (slot=%d buf=%p) -> (slot=%d buf=%p)", mCurrentTexture,
             mCurrentTextureImage != nullptr ? mCurrentTextureImage->graphicBufferHandle() : 0,
             slot, mSlots[slot].mGraphicBuffer->handle);

    // Hang onto the pointer so that it isn't freed in the call to
    // releaseBufferLocked() if we're in shared buffer mode and both buffers are
    // the same.
    sp<Image> nextTextureImage = mImages[slot];

    // release old buffer
    if (mCurrentTexture != BufferQueue::INVALID_BUFFER_SLOT) {
        if (pendingRelease == nullptr) {
            status_t status =
                    releaseBufferLocked(mCurrentTexture, mCurrentTextureImage->graphicBuffer());
            if (status < NO_ERROR) {
                BLC_LOGE("updateAndRelease: failed to release buffer: %s (%d)", strerror(-status),
                         status);
                err = status;
                // keep going, with error raised [?]
            }
        } else {
            pendingRelease->currentTexture = mCurrentTexture;
            pendingRelease->graphicBuffer = mCurrentTextureImage->graphicBuffer();
            pendingRelease->isPending = true;
        }
    }

    // Update the BufferLayerConsumer state.
    mCurrentTexture = slot;
    mCurrentTextureImage = nextTextureImage;
    mCurrentCrop = item.mCrop;
    mCurrentTransform = item.mTransform;
    mCurrentScalingMode = item.mScalingMode;
    mCurrentTimestamp = item.mTimestamp;
    mCurrentDataSpace = static_cast<ui::Dataspace>(item.mDataSpace);
    mCurrentHdrMetadata = item.mHdrMetadata;
    mCurrentFence = item.mFence;
    mCurrentFenceTime = item.mFenceTime;
    mCurrentFrameNumber = item.mFrameNumber;
    mCurrentTransformToDisplayInverse = item.mTransformToDisplayInverse;
    mCurrentSurfaceDamage = item.mSurfaceDamage;
    mCurrentApi = item.mApi;

    computeCurrentTransformMatrixLocked();

    return err;
}

status_t BufferLayerConsumer::bindTextureImageLocked() {
    ATRACE_CALL();
    mRE.checkErrors();

    if (mCurrentTexture == BufferQueue::INVALID_BUFFER_SLOT && mCurrentTextureImage == nullptr) {
        BLC_LOGE("bindTextureImage: no currently-bound texture");
        mRE.bindExternalTextureImage(mTexName, *mRE.createImage());
        return NO_INIT;
    }

    const Rect& imageCrop = canUseImageCrop(mCurrentCrop) ? mCurrentCrop : Rect::EMPTY_RECT;
    status_t err = mCurrentTextureImage->createIfNeeded(imageCrop);
    if (err != NO_ERROR) {
        BLC_LOGW("bindTextureImage: can't create image on slot=%d", mCurrentTexture);
        mRE.bindExternalTextureImage(mTexName, *mRE.createImage());
        return UNKNOWN_ERROR;
    }

    mRE.bindExternalTextureImage(mTexName, mCurrentTextureImage->image());

    // Wait for the new buffer to be ready.
    return doFenceWaitLocked();
}

status_t BufferLayerConsumer::syncForReleaseLocked() {
    BLC_LOGV("syncForReleaseLocked");

    if (mCurrentTexture != BufferQueue::INVALID_BUFFER_SLOT) {
        if (SyncFeatures::getInstance().useNativeFenceSync()) {
            base::unique_fd fenceFd = mRE.flush();
            if (fenceFd == -1) {
                BLC_LOGE("syncForReleaseLocked: failed to flush RenderEngine");
                return UNKNOWN_ERROR;
            }
            sp<Fence> fence(new Fence(std::move(fenceFd)));
            status_t err = addReleaseFenceLocked(mCurrentTexture,
                                                 mCurrentTextureImage->graphicBuffer(), fence);
            if (err != OK) {
                BLC_LOGE("syncForReleaseLocked: error adding release fence: "
                         "%s (%d)",
                         strerror(-err), err);
                return err;
            }
        }
    }

    return OK;
}

void BufferLayerConsumer::getTransformMatrix(float mtx[16]) {
    Mutex::Autolock lock(mMutex);
    memcpy(mtx, mCurrentTransformMatrix, sizeof(mCurrentTransformMatrix));
}

void BufferLayerConsumer::setFilteringEnabled(bool enabled) {
    Mutex::Autolock lock(mMutex);
    if (mAbandoned) {
        BLC_LOGE("setFilteringEnabled: BufferLayerConsumer is abandoned!");
        return;
    }
    bool needsRecompute = mFilteringEnabled != enabled;
    mFilteringEnabled = enabled;

    if (needsRecompute && mCurrentTextureImage == nullptr) {
        BLC_LOGD("setFilteringEnabled called with mCurrentTextureImage == nullptr");
    }

    if (needsRecompute && mCurrentTextureImage != nullptr) {
        computeCurrentTransformMatrixLocked();
    }
}

void BufferLayerConsumer::computeCurrentTransformMatrixLocked() {
    BLC_LOGV("computeCurrentTransformMatrixLocked");
    sp<GraphicBuffer> buf =
            (mCurrentTextureImage == nullptr) ? nullptr : mCurrentTextureImage->graphicBuffer();
    if (buf == nullptr) {
        BLC_LOGD("computeCurrentTransformMatrixLocked: "
                 "mCurrentTextureImage is nullptr");
    }
    const Rect& cropRect = canUseImageCrop(mCurrentCrop) ? Rect::EMPTY_RECT : mCurrentCrop;
    GLConsumer::computeTransformMatrix(mCurrentTransformMatrix, buf, cropRect, mCurrentTransform,
                                       mFilteringEnabled);
}

nsecs_t BufferLayerConsumer::getTimestamp() {
    BLC_LOGV("getTimestamp");
    Mutex::Autolock lock(mMutex);
    return mCurrentTimestamp;
}

ui::Dataspace BufferLayerConsumer::getCurrentDataSpace() {
    BLC_LOGV("getCurrentDataSpace");
    Mutex::Autolock lock(mMutex);
    return mCurrentDataSpace;
}

const HdrMetadata& BufferLayerConsumer::getCurrentHdrMetadata() const {
    BLC_LOGV("getCurrentHdrMetadata");
    Mutex::Autolock lock(mMutex);
    return mCurrentHdrMetadata;
}

uint64_t BufferLayerConsumer::getFrameNumber() {
    BLC_LOGV("getFrameNumber");
    Mutex::Autolock lock(mMutex);
    return mCurrentFrameNumber;
}

bool BufferLayerConsumer::getTransformToDisplayInverse() const {
    Mutex::Autolock lock(mMutex);
    return mCurrentTransformToDisplayInverse;
}

const Region& BufferLayerConsumer::getSurfaceDamage() const {
    return mCurrentSurfaceDamage;
}

int BufferLayerConsumer::getCurrentApi() const {
    Mutex::Autolock lock(mMutex);
    return mCurrentApi;
}

sp<GraphicBuffer> BufferLayerConsumer::getCurrentBuffer(int* outSlot) const {
    Mutex::Autolock lock(mMutex);

    if (outSlot != nullptr) {
        *outSlot = mCurrentTexture;
    }

    return (mCurrentTextureImage == nullptr) ? nullptr : mCurrentTextureImage->graphicBuffer();
}

Rect BufferLayerConsumer::getCurrentCrop() const {
    Mutex::Autolock lock(mMutex);
    return (mCurrentScalingMode == NATIVE_WINDOW_SCALING_MODE_SCALE_CROP)
            ? GLConsumer::scaleDownCrop(mCurrentCrop, mDefaultWidth, mDefaultHeight)
            : mCurrentCrop;
}

uint32_t BufferLayerConsumer::getCurrentTransform() const {
    Mutex::Autolock lock(mMutex);
    return mCurrentTransform;
}

uint32_t BufferLayerConsumer::getCurrentScalingMode() const {
    Mutex::Autolock lock(mMutex);
    return mCurrentScalingMode;
}

sp<Fence> BufferLayerConsumer::getCurrentFence() const {
    Mutex::Autolock lock(mMutex);
    return mCurrentFence;
}

std::shared_ptr<FenceTime> BufferLayerConsumer::getCurrentFenceTime() const {
    Mutex::Autolock lock(mMutex);
    return mCurrentFenceTime;
}

status_t BufferLayerConsumer::doFenceWaitLocked() const {
    if (!mRE.isCurrent()) {
        BLC_LOGE("doFenceWait: RenderEngine is not current");
        return INVALID_OPERATION;
    }

    if (mCurrentFence->isValid()) {
        if (SyncFeatures::getInstance().useWaitSync()) {
            base::unique_fd fenceFd(mCurrentFence->dup());
            if (fenceFd == -1) {
                BLC_LOGE("doFenceWait: error dup'ing fence fd: %d", errno);
                return -errno;
            }
            if (!mRE.waitFence(std::move(fenceFd))) {
                BLC_LOGE("doFenceWait: failed to wait on fence fd");
                return UNKNOWN_ERROR;
            }
        } else {
            status_t err = mCurrentFence->waitForever("BufferLayerConsumer::doFenceWaitLocked");
            if (err != NO_ERROR) {
                BLC_LOGE("doFenceWait: error waiting for fence: %d", err);
                return err;
            }
        }
    }

    return NO_ERROR;
}

void BufferLayerConsumer::freeBufferLocked(int slotIndex) {
    BLC_LOGV("freeBufferLocked: slotIndex=%d", slotIndex);
    if (slotIndex == mCurrentTexture) {
        mCurrentTexture = BufferQueue::INVALID_BUFFER_SLOT;
    }
    mImages[slotIndex].clear();
    ConsumerBase::freeBufferLocked(slotIndex);
}

void BufferLayerConsumer::onDisconnect() {
    sp<Layer> l = mLayer.promote();
    if (l.get()) {
        l->onDisconnect();
    }
}

void BufferLayerConsumer::onSidebandStreamChanged() {
    FrameAvailableListener* unsafeFrameAvailableListener = nullptr;
    {
        Mutex::Autolock lock(mFrameAvailableMutex);
        unsafeFrameAvailableListener = mFrameAvailableListener.unsafe_get();
    }
    sp<ContentsChangedListener> listener;
    { // scope for the lock
        Mutex::Autolock lock(mMutex);
        ALOG_ASSERT(unsafeFrameAvailableListener == mContentsChangedListener.unsafe_get());
        listener = mContentsChangedListener.promote();
    }

    if (listener != nullptr) {
        listener->onSidebandStreamChanged();
    }
}

void BufferLayerConsumer::addAndGetFrameTimestamps(const NewFrameEventsEntry* newTimestamps,
                                                   FrameEventHistoryDelta* outDelta) {
    sp<Layer> l = mLayer.promote();
    if (l.get()) {
        l->addAndGetFrameTimestamps(newTimestamps, outDelta);
    }
}

void BufferLayerConsumer::abandonLocked() {
    BLC_LOGV("abandonLocked");
    mCurrentTextureImage.clear();
    ConsumerBase::abandonLocked();
}

status_t BufferLayerConsumer::setConsumerUsageBits(uint64_t usage) {
    return ConsumerBase::setConsumerUsageBits(usage | DEFAULT_USAGE_FLAGS);
}

void BufferLayerConsumer::dumpLocked(String8& result, const char* prefix) const {
    result.appendFormat("%smTexName=%d mCurrentTexture=%d\n"
                        "%smCurrentCrop=[%d,%d,%d,%d] mCurrentTransform=%#x\n",
                        prefix, mTexName, mCurrentTexture, prefix, mCurrentCrop.left,
                        mCurrentCrop.top, mCurrentCrop.right, mCurrentCrop.bottom,
                        mCurrentTransform);

    ConsumerBase::dumpLocked(result, prefix);
}

BufferLayerConsumer::Image::Image(sp<GraphicBuffer> graphicBuffer, RE::RenderEngine& engine)
      : mGraphicBuffer(graphicBuffer),
        mImage{engine.createImage()},
        mCreated(false),
        mCropWidth(0),
        mCropHeight(0) {}

BufferLayerConsumer::Image::~Image() = default;

status_t BufferLayerConsumer::Image::createIfNeeded(const Rect& imageCrop) {
    const int32_t cropWidth = imageCrop.width();
    const int32_t cropHeight = imageCrop.height();
    if (mCreated && mCropWidth == cropWidth && mCropHeight == cropHeight) {
        return OK;
    }

    mCreated = mImage->setNativeWindowBuffer(mGraphicBuffer->getNativeBuffer(),
                                             mGraphicBuffer->getUsage() & GRALLOC_USAGE_PROTECTED,
                                             cropWidth, cropHeight);
    if (mCreated) {
        mCropWidth = cropWidth;
        mCropHeight = cropHeight;
    } else {
        mCropWidth = 0;
        mCropHeight = 0;

        const sp<GraphicBuffer>& buffer = mGraphicBuffer;
        ALOGE("Failed to create image. size=%ux%u st=%u usage=%#" PRIx64 " fmt=%d",
              buffer->getWidth(), buffer->getHeight(), buffer->getStride(), buffer->getUsage(),
              buffer->getPixelFormat());
    }

    return mCreated ? OK : UNKNOWN_ERROR;
}

}; // namespace android
