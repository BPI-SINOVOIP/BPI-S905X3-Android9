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

#ifndef ANDROID_LAYER_H
#define ANDROID_LAYER_H

#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Timers.h>

#include <ui/FloatRect.h>
#include <ui/FrameStats.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <ui/Region.h>

#include <gui/ISurfaceComposerClient.h>
#include <gui/LayerState.h>
#include <gui/BufferQueue.h>

#include <list>
#include <cstdint>

#include "Client.h"
#include "FrameTracker.h"
#include "LayerVector.h"
#include "MonitoredProducer.h"
#include "SurfaceFlinger.h"
#include "TimeStats/TimeStats.h"
#include "Transform.h"

#include <layerproto/LayerProtoHeader.h>
#include "DisplayHardware/HWComposer.h"
#include "DisplayHardware/HWComposerBufferCache.h"
#include "RenderArea.h"
#include "RenderEngine/Mesh.h"
#include "RenderEngine/Texture.h"

#include <math/vec4.h>
#include <vector>

using namespace android::surfaceflinger;

namespace android {

// ---------------------------------------------------------------------------

class Client;
class Colorizer;
class DisplayDevice;
class GraphicBuffer;
class SurfaceFlinger;
class LayerDebugInfo;
class LayerBE;

namespace impl {
class SurfaceInterceptor;
}

// ---------------------------------------------------------------------------

struct CompositionInfo {
    HWC2::Composition compositionType;
    sp<GraphicBuffer> mBuffer = nullptr;
    int mBufferSlot = BufferQueue::INVALID_BUFFER_SLOT;
    struct {
        HWComposer* hwc;
        sp<Fence> fence;
        HWC2::BlendMode blendMode;
        Rect displayFrame;
        float alpha;
        FloatRect sourceCrop;
        HWC2::Transform transform;
        int z;
        int type;
        int appId;
        Region visibleRegion;
        Region surfaceDamage;
        sp<NativeHandle> sidebandStream;
        android_dataspace dataspace;
        hwc_color_t color;
    } hwc;
    struct {
        RE::RenderEngine* renderEngine;
        Mesh* mesh;
    } renderEngine;
};

class LayerBE {
public:
    LayerBE();

    // The mesh used to draw the layer in GLES composition mode
    Mesh mMesh;

    // HWC items, accessed from the main thread
    struct HWCInfo {
        HWCInfo()
              : hwc(nullptr),
                layer(nullptr),
                forceClientComposition(false),
                compositionType(HWC2::Composition::Invalid),
                clearClientTarget(false),
                transform(HWC2::Transform::None) {}

        HWComposer* hwc;
        HWC2::Layer* layer;
        bool forceClientComposition;
        HWC2::Composition compositionType;
        bool clearClientTarget;
        Rect displayFrame;
        FloatRect sourceCrop;
        HWComposerBufferCache bufferCache;
        HWC2::Transform transform;
    };

    // A layer can be attached to multiple displays when operating in mirror mode
    // (a.k.a: when several displays are attached with equal layerStack). In this
    // case we need to keep track. In non-mirror mode, a layer will have only one
    // HWCInfo. This map key is a display layerStack.
    std::unordered_map<int32_t, HWCInfo> mHwcLayers;

    CompositionInfo compositionInfo;
};

class Layer : public virtual RefBase {
    static int32_t sSequence;

public:
    LayerBE& getBE() { return mBE; }
    LayerBE& getBE() const { return mBE; }
    mutable bool contentDirty;
    // regions below are in window-manager space
    Region visibleRegion;
    Region coveredRegion;
    Region visibleNonTransparentRegion;
    Region surfaceDamageRegion;

    // Layer serial number.  This gives layers an explicit ordering, so we
    // have a stable sort order when their layer stack and Z-order are
    // the same.
    int32_t sequence;

    enum { // flags for doTransaction()
        eDontUpdateGeometryState = 0x00000001,
        eVisibleRegion = 0x00000002,
    };

    struct Geometry {
        uint32_t w;
        uint32_t h;
        Transform transform;

        inline bool operator==(const Geometry& rhs) const {
            return (w == rhs.w && h == rhs.h) && (transform.tx() == rhs.transform.tx()) &&
                    (transform.ty() == rhs.transform.ty());
        }
        inline bool operator!=(const Geometry& rhs) const { return !operator==(rhs); }
    };

    struct State {
        Geometry active;
        Geometry requested;
        int32_t z;

        // The identifier of the layer stack this layer belongs to. A layer can
        // only be associated to a single layer stack. A layer stack is a
        // z-ordered group of layers which can be associated to one or more
        // displays. Using the same layer stack on different displays is a way
        // to achieve mirroring.
        uint32_t layerStack;

        uint8_t flags;
        uint8_t reserved[2];
        int32_t sequence; // changes when visible regions can change
        bool modified;

        // Crop is expressed in layer space coordinate.
        Rect crop;
        Rect requestedCrop;

        // finalCrop is expressed in display space coordinate.
        Rect finalCrop;
        Rect requestedFinalCrop;

        // If set, defers this state update until the identified Layer
        // receives a frame with the given frameNumber
        wp<Layer> barrierLayer;
        uint64_t frameNumber;

        // the transparentRegion hint is a bit special, it's latched only
        // when we receive a buffer -- this is because it's "content"
        // dependent.
        Region activeTransparentRegion;
        Region requestedTransparentRegion;

        int32_t appId;
        int32_t type;

        // If non-null, a Surface this Surface's Z-order is interpreted relative to.
        wp<Layer> zOrderRelativeOf;

        // A list of surfaces whose Z-order is interpreted relative to ours.
        SortedVector<wp<Layer>> zOrderRelatives;

        half4 color;
    };

    Layer(SurfaceFlinger* flinger, const sp<Client>& client, const String8& name, uint32_t w,
          uint32_t h, uint32_t flags);
    virtual ~Layer();

    void setPrimaryDisplayOnly() { mPrimaryDisplayOnly = true; }

    // ------------------------------------------------------------------------
    // Geometry setting functions.
    //
    // The following group of functions are used to specify the layers
    // bounds, and the mapping of the texture on to those bounds. According
    // to various settings changes to them may apply immediately, or be delayed until
    // a pending resize is completed by the producer submitting a buffer. For example
    // if we were to change the buffer size, and update the matrix ahead of the
    // new buffer arriving, then we would be stretching the buffer to a different
    // aspect before and after the buffer arriving, which probably isn't what we wanted.
    //
    // The first set of geometry functions are controlled by the scaling mode, described
    // in window.h. The scaling mode may be set by the client, as it submits buffers.
    // This value may be overriden through SurfaceControl, with setOverrideScalingMode.
    //
    // Put simply, if our scaling mode is SCALING_MODE_FREEZE, then
    // matrix updates will not be applied while a resize is pending
    // and the size and transform will remain in their previous state
    // until a new buffer is submitted. If the scaling mode is another value
    // then the old-buffer will immediately be scaled to the pending size
    // and the new matrix will be immediately applied following this scaling
    // transformation.

    // Set the default buffer size for the assosciated Producer, in pixels. This is
    // also the rendered size of the layer prior to any transformations. Parent
    // or local matrix transformations will not affect the size of the buffer,
    // but may affect it's on-screen size or clipping.
    bool setSize(uint32_t w, uint32_t h);
    // Set a 2x2 transformation matrix on the layer. This transform
    // will be applied after parent transforms, but before any final
    // producer specified transform.
    bool setMatrix(const layer_state_t::matrix22_t& matrix);

    // This second set of geometry attributes are controlled by
    // setGeometryAppliesWithResize, and their default mode is to be
    // immediate. If setGeometryAppliesWithResize is specified
    // while a resize is pending, then update of these attributes will
    // be delayed until the resize completes.

    // setPosition operates in parent buffer space (pre parent-transform) or display
    // space for top-level layers.
    bool setPosition(float x, float y, bool immediate);
    // Buffer space
    bool setCrop(const Rect& crop, bool immediate);
    // Parent buffer space/display space
    bool setFinalCrop(const Rect& crop, bool immediate);

    // TODO(b/38182121): Could we eliminate the various latching modes by
    // using the layer hierarchy?
    // -----------------------------------------------------------------------
    bool setLayer(int32_t z);
    bool setRelativeLayer(const sp<IBinder>& relativeToHandle, int32_t relativeZ);

    bool setAlpha(float alpha);
    bool setColor(const half3& color);
    bool setTransparentRegionHint(const Region& transparent);
    bool setFlags(uint8_t flags, uint8_t mask);
    bool setLayerStack(uint32_t layerStack);
    uint32_t getLayerStack() const;
    void deferTransactionUntil(const sp<IBinder>& barrierHandle, uint64_t frameNumber);
    void deferTransactionUntil(const sp<Layer>& barrierLayer, uint64_t frameNumber);
    bool setOverrideScalingMode(int32_t overrideScalingMode);
    void setInfo(int32_t type, int32_t appId);
    bool reparentChildren(const sp<IBinder>& layer);
    void setChildrenDrawingParent(const sp<Layer>& layer);
    bool reparent(const sp<IBinder>& newParentHandle);
    bool detachChildren();

    ui::Dataspace getDataSpace() const { return mCurrentDataSpace; }

    // Before color management is introduced, contents on Android have to be
    // desaturated in order to match what they appears like visually.
    // With color management, these contents will appear desaturated, thus
    // needed to be saturated so that they match what they are designed for
    // visually.
    bool isLegacyDataSpace() const;

    // If we have received a new buffer this frame, we will pass its surface
    // damage down to hardware composer. Otherwise, we must send a region with
    // one empty rect.
    virtual void useSurfaceDamage() {}
    virtual void useEmptyDamage() {}

    uint32_t getTransactionFlags(uint32_t flags);
    uint32_t setTransactionFlags(uint32_t flags);

    bool belongsToDisplay(uint32_t layerStack, bool isPrimaryDisplay) const {
        return getLayerStack() == layerStack && (!mPrimaryDisplayOnly || isPrimaryDisplay);
    }

    void computeGeometry(const RenderArea& renderArea, Mesh& mesh, bool useIdentityTransform) const;
    FloatRect computeBounds(const Region& activeTransparentRegion) const;
    FloatRect computeBounds() const;

    int32_t getSequence() const { return sequence; }

    // -----------------------------------------------------------------------
    // Virtuals
    virtual const char* getTypeId() const = 0;

    /*
     * isOpaque - true if this surface is opaque
     *
     * This takes into account the buffer format (i.e. whether or not the
     * pixel format includes an alpha channel) and the "opaque" flag set
     * on the layer.  It does not examine the current plane alpha value.
     */
    virtual bool isOpaque(const Layer::State&) const { return false; }

    /*
     * isSecure - true if this surface is secure, that is if it prevents
     * screenshots or VNC servers.
     */
    bool isSecure() const;

    /*
     * isVisible - true if this layer is visible, false otherwise
     */
    virtual bool isVisible() const = 0;

    /*
     * isHiddenByPolicy - true if this layer has been forced invisible.
     * just because this is false, doesn't mean isVisible() is true.
     * For example if this layer has no active buffer, it may not be hidden by
     * policy, but it still can not be visible.
     */
    bool isHiddenByPolicy() const;

    /*
     * isFixedSize - true if content has a fixed size
     */
    virtual bool isFixedSize() const { return true; }


    bool isPendingRemoval() const { return mPendingRemoval; }

    void writeToProto(LayerProto* layerInfo,
                      LayerVector::StateSet stateSet = LayerVector::StateSet::Drawing);

    void writeToProto(LayerProto* layerInfo, int32_t hwcId);

protected:
    /*
     * onDraw - draws the surface.
     */
    virtual void onDraw(const RenderArea& renderArea, const Region& clip,
                        bool useIdentityTransform) const = 0;

public:
    virtual void setDefaultBufferSize(uint32_t /*w*/, uint32_t /*h*/) {}

    virtual bool isHdrY410() const { return false; }

    void setGeometry(const sp<const DisplayDevice>& displayDevice, uint32_t z);
    void forceClientComposition(int32_t hwcId);
    bool getForceClientComposition(int32_t hwcId);
    virtual void setPerFrameData(const sp<const DisplayDevice>& displayDevice) = 0;

    // callIntoHwc exists so we can update our local state and call
    // acceptDisplayChanges without unnecessarily updating the device's state
    void setCompositionType(int32_t hwcId, HWC2::Composition type, bool callIntoHwc = true);
    HWC2::Composition getCompositionType(int32_t hwcId) const;
    void setClearClientTarget(int32_t hwcId, bool clear);
    bool getClearClientTarget(int32_t hwcId) const;
    void updateCursorPosition(const sp<const DisplayDevice>& hw);

    /*
     * called after page-flip
     */
    virtual void onLayerDisplayed(const sp<Fence>& releaseFence);

    virtual void abandon() {}

    virtual bool shouldPresentNow(const DispSync& /*dispSync*/) const { return false; }
    virtual void setTransformHint(uint32_t /*orientation*/) const { }

    /*
     * called before composition.
     * returns true if the layer has pending updates.
     */
    virtual bool onPreComposition(nsecs_t /*refreshStartTime*/) { return true; }

    /*
     * called after composition.
     * returns true if the layer latched a new buffer this frame.
     */
    virtual bool onPostComposition(const std::shared_ptr<FenceTime>& /*glDoneFence*/,
                                   const std::shared_ptr<FenceTime>& /*presentFence*/,
                                   const CompositorTiming& /*compositorTiming*/) {
        return false;
    }

    // If a buffer was replaced this frame, release the former buffer
    virtual void releasePendingBuffer(nsecs_t /*dequeueReadyTime*/) { }


    /*
     * draw - performs some global clipping optimizations
     * and calls onDraw().
     */
    void draw(const RenderArea& renderArea, const Region& clip) const;
    void draw(const RenderArea& renderArea, bool useIdentityTransform) const;
    void draw(const RenderArea& renderArea) const;

    /*
     * doTransaction - process the transaction. This is a good place to figure
     * out which attributes of the surface have changed.
     */
    uint32_t doTransaction(uint32_t transactionFlags);

    /*
     * setVisibleRegion - called to set the new visible region. This gives
     * a chance to update the new visible region or record the fact it changed.
     */
    void setVisibleRegion(const Region& visibleRegion);

    /*
     * setCoveredRegion - called when the covered region changes. The covered
     * region corresponds to any area of the surface that is covered
     * (transparently or not) by another surface.
     */
    void setCoveredRegion(const Region& coveredRegion);

    /*
     * setVisibleNonTransparentRegion - called when the visible and
     * non-transparent region changes.
     */
    void setVisibleNonTransparentRegion(const Region& visibleNonTransparentRegion);

    /*
     * Clear the visible, covered, and non-transparent regions.
     */
    void clearVisibilityRegions();

    /*
     * latchBuffer - called each time the screen is redrawn and returns whether
     * the visible regions need to be recomputed (this is a fairly heavy
     * operation, so this should be set only if needed). Typically this is used
     * to figure out if the content or size of a surface has changed.
     */
    virtual Region latchBuffer(bool& /*recomputeVisibleRegions*/, nsecs_t /*latchTime*/) {
        return {};
    }

    virtual bool isBufferLatched() const { return false; }

    bool isPotentialCursor() const { return mPotentialCursor; }
    /*
     * called with the state lock from a binder thread when the layer is
     * removed from the current list to the pending removal list
     */
    void onRemovedFromCurrentState();

    /*
     * called with the state lock from the main thread when the layer is
     * removed from the pending removal list
     */
    void onRemoved();

    // Updates the transform hint in our SurfaceFlingerConsumer to match
    // the current orientation of the display device.
    void updateTransformHint(const sp<const DisplayDevice>& hw) const;

    /*
     * returns the rectangle that crops the content of the layer and scales it
     * to the layer's size.
     */
    Rect getContentCrop() const;

    /*
     * Returns if a frame is queued.
     */
    bool hasQueuedFrame() const {
        return mQueuedFrames > 0 || mSidebandStreamChanged || mAutoRefresh;
    }

    int32_t getQueuedFrameCount() const { return mQueuedFrames; }

    // -----------------------------------------------------------------------

    bool createHwcLayer(HWComposer* hwc, int32_t hwcId);
    bool destroyHwcLayer(int32_t hwcId);
    void destroyAllHwcLayers();

    bool hasHwcLayer(int32_t hwcId) {
        return getBE().mHwcLayers.count(hwcId) > 0;
    }

    HWC2::Layer* getHwcLayer(int32_t hwcId) {
        if (getBE().mHwcLayers.count(hwcId) == 0) {
            return nullptr;
        }
        return getBE().mHwcLayers[hwcId].layer;
    }

    // -----------------------------------------------------------------------

    void clearWithOpenGL(const RenderArea& renderArea) const;
    void setFiltering(bool filtering);
    bool getFiltering() const;


    inline const State& getDrawingState() const { return mDrawingState; }
    inline const State& getCurrentState() const { return mCurrentState; }
    inline State& getCurrentState() { return mCurrentState; }

    LayerDebugInfo getLayerDebugInfo() const;

    /* always call base class first */
    static void miniDumpHeader(String8& result);
    void miniDump(String8& result, int32_t hwcId) const;
    void dumpFrameStats(String8& result) const;
    void dumpFrameEvents(String8& result);
    void clearFrameStats();
    void logFrameStats();
    void getFrameStats(FrameStats* outStats) const;

    virtual std::vector<OccupancyTracker::Segment> getOccupancyHistory(bool /*forceFlush*/) {
        return {};
    }

    void onDisconnect();
    void addAndGetFrameTimestamps(const NewFrameEventsEntry* newEntry,
                                  FrameEventHistoryDelta* outDelta);

    virtual bool getTransformToDisplayInverse() const { return false; }

    Transform getTransform() const;

    // Returns the Alpha of the Surface, accounting for the Alpha
    // of parent Surfaces in the hierarchy (alpha's will be multiplied
    // down the hierarchy).
    half getAlpha() const;
    half4 getColor() const;

    void traverseInReverseZOrder(LayerVector::StateSet stateSet,
                                 const LayerVector::Visitor& visitor);
    void traverseInZOrder(LayerVector::StateSet stateSet, const LayerVector::Visitor& visitor);

    /**
     * Traverse only children in z order, ignoring relative layers that are not children of the
     * parent.
     */
    void traverseChildrenInZOrder(LayerVector::StateSet stateSet,
                                  const LayerVector::Visitor& visitor);

    size_t getChildrenCount() const;
    void addChild(const sp<Layer>& layer);
    // Returns index if removed, or negative value otherwise
    // for symmetry with Vector::remove
    ssize_t removeChild(const sp<Layer>& layer);
    sp<Layer> getParent() const { return mCurrentParent.promote(); }
    bool hasParent() const { return getParent() != nullptr; }
    Rect computeScreenBounds(bool reduceTransparentRegion = true) const;
    bool setChildLayer(const sp<Layer>& childLayer, int32_t z);
    bool setChildRelativeLayer(const sp<Layer>& childLayer,
            const sp<IBinder>& relativeToHandle, int32_t relativeZ);

    // Copy the current list of children to the drawing state. Called by
    // SurfaceFlinger to complete a transaction.
    void commitChildList();
    int32_t getZ() const;
    void pushPendingState();

protected:
    // constant
    sp<SurfaceFlinger> mFlinger;
    /*
     * Trivial class, used to ensure that mFlinger->onLayerDestroyed(mLayer)
     * is called.
     */
    class LayerCleaner {
        sp<SurfaceFlinger> mFlinger;
        wp<Layer> mLayer;

    protected:
        ~LayerCleaner() {
            // destroy client resources
            mFlinger->onLayerDestroyed(mLayer);
        }

    public:
        LayerCleaner(const sp<SurfaceFlinger>& flinger, const sp<Layer>& layer)
              : mFlinger(flinger), mLayer(layer) {}
    };

    virtual void onFirstRef();

    friend class impl::SurfaceInterceptor;

    void commitTransaction(const State& stateToCommit);

    uint32_t getEffectiveUsage(uint32_t usage) const;

    FloatRect computeCrop(const sp<const DisplayDevice>& hw) const;
    // Compute the initial crop as specified by parent layers and the
    // SurfaceControl for this layer. Does not include buffer crop from the
    // IGraphicBufferProducer client, as that should not affect child clipping.
    // Returns in screen space.
    Rect computeInitialCrop(const sp<const DisplayDevice>& hw) const;

    // drawing
    void clearWithOpenGL(const RenderArea& renderArea, float r, float g, float b,
                         float alpha) const;

    void setParent(const sp<Layer>& layer);

    LayerVector makeTraversalList(LayerVector::StateSet stateSet, bool* outSkipRelativeZUsers);
    void addZOrderRelative(const wp<Layer>& relative);
    void removeZOrderRelative(const wp<Layer>& relative);

    class SyncPoint {
    public:
        explicit SyncPoint(uint64_t frameNumber)
              : mFrameNumber(frameNumber), mFrameIsAvailable(false), mTransactionIsApplied(false) {}

        uint64_t getFrameNumber() const { return mFrameNumber; }

        bool frameIsAvailable() const { return mFrameIsAvailable; }

        void setFrameAvailable() { mFrameIsAvailable = true; }

        bool transactionIsApplied() const { return mTransactionIsApplied; }

        void setTransactionApplied() { mTransactionIsApplied = true; }

    private:
        const uint64_t mFrameNumber;
        std::atomic<bool> mFrameIsAvailable;
        std::atomic<bool> mTransactionIsApplied;
    };

    // SyncPoints which will be signaled when the correct frame is at the head
    // of the queue and dropped after the frame has been latched. Protected by
    // mLocalSyncPointMutex.
    Mutex mLocalSyncPointMutex;
    std::list<std::shared_ptr<SyncPoint>> mLocalSyncPoints;

    // SyncPoints which will be signaled and then dropped when the transaction
    // is applied
    std::list<std::shared_ptr<SyncPoint>> mRemoteSyncPoints;

    // Returns false if the relevant frame has already been latched
    bool addSyncPoint(const std::shared_ptr<SyncPoint>& point);

    void popPendingState(State* stateToCommit);
    bool applyPendingStates(State* stateToCommit);

    void clearSyncPoints();

    // Returns mCurrentScaling mode (originating from the
    // Client) or mOverrideScalingMode mode (originating from
    // the Surface Controller) if set.
    virtual uint32_t getEffectiveScalingMode() const { return 0; }

public:
    /*
     * The layer handle is just a BBinder object passed to the client
     * (remote process) -- we don't keep any reference on our side such that
     * the dtor is called when the remote side let go of its reference.
     *
     * LayerCleaner ensures that mFlinger->onLayerDestroyed() is called for
     * this layer when the handle is destroyed.
     */
    class Handle : public BBinder, public LayerCleaner {
    public:
        Handle(const sp<SurfaceFlinger>& flinger, const sp<Layer>& layer)
              : LayerCleaner(flinger, layer), owner(layer) {}

        wp<Layer> owner;
    };

    sp<IBinder> getHandle();
    const String8& getName() const;
    virtual void notifyAvailableFrames() {}
    virtual PixelFormat getPixelFormat() const { return PIXEL_FORMAT_NONE; }
    bool getPremultipledAlpha() const;

protected:
    // -----------------------------------------------------------------------
    bool usingRelativeZ(LayerVector::StateSet stateSet);

    bool mPremultipliedAlpha;
    String8 mName;
    String8 mTransactionName; // A cached version of "TX - " + mName for systraces

    bool mPrimaryDisplayOnly = false;

    // these are protected by an external lock
    State mCurrentState;
    State mDrawingState;
    volatile int32_t mTransactionFlags;

    // Accessed from main thread and binder threads
    Mutex mPendingStateMutex;
    Vector<State> mPendingStates;

    // thread-safe
    volatile int32_t mQueuedFrames;
    volatile int32_t mSidebandStreamChanged; // used like an atomic boolean

    // Timestamp history for UIAutomation. Thread safe.
    FrameTracker mFrameTracker;

    // Timestamp history for the consumer to query.
    // Accessed by both consumer and producer on main and binder threads.
    Mutex mFrameEventHistoryMutex;
    ConsumerFrameEventHistory mFrameEventHistory;
    FenceTimeline mAcquireTimeline;
    FenceTimeline mReleaseTimeline;

    TimeStats& mTimeStats = TimeStats::getInstance();

    // main thread
    int mActiveBufferSlot;
    sp<GraphicBuffer> mActiveBuffer;
    sp<NativeHandle> mSidebandStream;
    ui::Dataspace mCurrentDataSpace = ui::Dataspace::UNKNOWN;
    Rect mCurrentCrop;
    uint32_t mCurrentTransform;
    // We encode unset as -1.
    int32_t mOverrideScalingMode;
    bool mCurrentOpacity;
    std::atomic<uint64_t> mCurrentFrameNumber;
    bool mFrameLatencyNeeded;
    // Whether filtering is forced on or not
    bool mFiltering;
    // Whether filtering is needed b/c of the drawingstate
    bool mNeedsFiltering;

    bool mPendingRemoval = false;

    // page-flip thread (currently main thread)
    bool mProtectedByApp; // application requires protected path to external sink

    // protected by mLock
    mutable Mutex mLock;

    const wp<Client> mClientRef;

    // This layer can be a cursor on some displays.
    bool mPotentialCursor;

    // Local copy of the queued contents of the incoming BufferQueue
    mutable Mutex mQueueItemLock;
    Condition mQueueItemCondition;
    Vector<BufferItem> mQueueItems;
    std::atomic<uint64_t> mLastFrameNumberReceived;
    bool mAutoRefresh;
    bool mFreezeGeometryUpdates;

    // Child list about to be committed/used for editing.
    LayerVector mCurrentChildren;
    // Child list used for rendering.
    LayerVector mDrawingChildren;

    wp<Layer> mCurrentParent;
    wp<Layer> mDrawingParent;

    mutable LayerBE mBE;

private:
    /**
     * Returns an unsorted vector of all layers that are part of this tree.
     * That includes the current layer and all its descendants.
     */
    std::vector<Layer*> getLayersInTree(LayerVector::StateSet stateSet);
    /**
     * Traverses layers that are part of this tree in the correct z order.
     * layersInTree must be sorted before calling this method.
     */
    void traverseChildrenInZOrderInner(const std::vector<Layer*>& layersInTree,
                                       LayerVector::StateSet stateSet,
                                       const LayerVector::Visitor& visitor);
    LayerVector makeChildrenTraversalList(LayerVector::StateSet stateSet,
                                          const std::vector<Layer*>& layersInTree);
};

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_LAYER_H
