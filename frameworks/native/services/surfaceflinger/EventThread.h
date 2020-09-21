/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <stdint.h>
#include <sys/types.h>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <android-base/thread_annotations.h>

#include <gui/DisplayEventReceiver.h>
#include <gui/IDisplayEventConnection.h>
#include <private/gui/BitTube.h>

#include <utils/Errors.h>
#include <utils/SortedVector.h>

#include "DisplayDevice.h"

// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

class EventThreadTest;
class SurfaceFlinger;
class String8;

// ---------------------------------------------------------------------------

class VSyncSource {
public:
    class Callback {
    public:
        virtual ~Callback() {}
        virtual void onVSyncEvent(nsecs_t when) = 0;
    };

    virtual ~VSyncSource() {}
    virtual void setVSyncEnabled(bool enable) = 0;
    virtual void setCallback(Callback* callback) = 0;
    virtual void setPhaseOffset(nsecs_t phaseOffset) = 0;
};

class EventThread {
public:
    virtual ~EventThread();

    virtual sp<BnDisplayEventConnection> createEventConnection() const = 0;

    // called before the screen is turned off from main thread
    virtual void onScreenReleased() = 0;

    // called after the screen is turned on from main thread
    virtual void onScreenAcquired() = 0;

    // called when receiving a hotplug event
    virtual void onHotplugReceived(int type, bool connected) = 0;

    virtual void dump(String8& result) const = 0;

    virtual void setPhaseOffset(nsecs_t phaseOffset) = 0;
};

namespace impl {

class EventThread : public android::EventThread, private VSyncSource::Callback {
    class Connection : public BnDisplayEventConnection {
    public:
        explicit Connection(EventThread* eventThread);
        virtual ~Connection();

        virtual status_t postEvent(const DisplayEventReceiver::Event& event);

        // count >= 1 : continuous event. count is the vsync rate
        // count == 0 : one-shot event that has not fired
        // count ==-1 : one-shot event that fired this round / disabled
        int32_t count;

    private:
        virtual void onFirstRef();
        status_t stealReceiveChannel(gui::BitTube* outChannel) override;
        status_t setVsyncRate(uint32_t count) override;
        void requestNextVsync() override; // asynchronous
        EventThread* const mEventThread;
        gui::BitTube mChannel;
    };

public:
    using ResyncWithRateLimitCallback = std::function<void()>;
    using InterceptVSyncsCallback = std::function<void(nsecs_t)>;

    EventThread(VSyncSource* src, ResyncWithRateLimitCallback resyncWithRateLimitCallback,
                InterceptVSyncsCallback interceptVSyncsCallback, const char* threadName);
    ~EventThread();

    sp<BnDisplayEventConnection> createEventConnection() const override;
    status_t registerDisplayEventConnection(const sp<Connection>& connection);

    void setVsyncRate(uint32_t count, const sp<Connection>& connection);
    void requestNextVsync(const sp<Connection>& connection);

    // called before the screen is turned off from main thread
    void onScreenReleased() override;

    // called after the screen is turned on from main thread
    void onScreenAcquired() override;

    // called when receiving a hotplug event
    void onHotplugReceived(int type, bool connected) override;

    void dump(String8& result) const override;

    void setPhaseOffset(nsecs_t phaseOffset) override;

private:
    friend EventThreadTest;

    void threadMain();
    Vector<sp<EventThread::Connection>> waitForEventLocked(std::unique_lock<std::mutex>* lock,
                                                           DisplayEventReceiver::Event* event)
            REQUIRES(mMutex);

    void removeDisplayEventConnectionLocked(const wp<Connection>& connection) REQUIRES(mMutex);
    void enableVSyncLocked() REQUIRES(mMutex);
    void disableVSyncLocked() REQUIRES(mMutex);

    // Implements VSyncSource::Callback
    void onVSyncEvent(nsecs_t timestamp) override;

    // constants
    VSyncSource* const mVSyncSource GUARDED_BY(mMutex) = nullptr;
    const ResyncWithRateLimitCallback mResyncWithRateLimitCallback;
    const InterceptVSyncsCallback mInterceptVSyncsCallback;

    std::thread mThread;
    mutable std::mutex mMutex;
    mutable std::condition_variable mCondition;

    // protected by mLock
    SortedVector<wp<Connection>> mDisplayEventConnections GUARDED_BY(mMutex);
    Vector<DisplayEventReceiver::Event> mPendingEvents GUARDED_BY(mMutex);
    DisplayEventReceiver::Event mVSyncEvent[DisplayDevice::NUM_BUILTIN_DISPLAY_TYPES] GUARDED_BY(
            mMutex);
    bool mUseSoftwareVSync GUARDED_BY(mMutex) = false;
    bool mVsyncEnabled GUARDED_BY(mMutex) = false;
    bool mKeepRunning GUARDED_BY(mMutex) = true;

    // for debugging
    bool mDebugVsyncEnabled GUARDED_BY(mMutex) = false;
};

// ---------------------------------------------------------------------------

} // namespace impl
} // namespace android
