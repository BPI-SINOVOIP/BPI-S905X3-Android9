/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "DisplayDevice.h"
#include "SurfaceFlinger.h"

namespace android {

class EventThread;

namespace RE {
class RenderEngine;
}

namespace Hwc2 {
class Composer;
}

class TestableSurfaceFlinger {
public:
    // Extend this as needed for accessing SurfaceFlinger private (and public)
    // functions.

    void setupRenderEngine(std::unique_ptr<RE::RenderEngine> renderEngine) {
        mFlinger->getBE().mRenderEngine = std::move(renderEngine);
    }

    void setupComposer(std::unique_ptr<Hwc2::Composer> composer) {
        mFlinger->getBE().mHwc.reset(new HWComposer(std::move(composer)));
    }

    using CreateBufferQueueFunction = SurfaceFlinger::CreateBufferQueueFunction;
    void setCreateBufferQueueFunction(CreateBufferQueueFunction f) {
        mFlinger->mCreateBufferQueue = f;
    }

    using CreateNativeWindowSurfaceFunction = SurfaceFlinger::CreateNativeWindowSurfaceFunction;
    void setCreateNativeWindowSurface(CreateNativeWindowSurfaceFunction f) {
        mFlinger->mCreateNativeWindowSurface = f;
    }

    using HotplugEvent = SurfaceFlinger::HotplugEvent;

    /* ------------------------------------------------------------------------
     * Forwarding for functions being tested
     */

    auto createDisplay(const String8& displayName, bool secure) {
        return mFlinger->createDisplay(displayName, secure);
    }

    auto destroyDisplay(const sp<IBinder>& display) { return mFlinger->destroyDisplay(display); }

    auto resetDisplayState() { return mFlinger->resetDisplayState(); }

    auto setupNewDisplayDeviceInternal(const wp<IBinder>& display, int hwcId,
                                       const DisplayDeviceState& state,
                                       const sp<DisplaySurface>& dispSurface,
                                       const sp<IGraphicBufferProducer>& producer) {
        return mFlinger->setupNewDisplayDeviceInternal(display, hwcId, state, dispSurface,
                                                       producer);
    }

    auto handleTransactionLocked(uint32_t transactionFlags) {
        return mFlinger->handleTransactionLocked(transactionFlags);
    }

    auto onHotplugReceived(int32_t sequenceId, hwc2_display_t display,
                           HWC2::Connection connection) {
        return mFlinger->onHotplugReceived(sequenceId, display, connection);
    }

    auto setDisplayStateLocked(const DisplayState& s) { return mFlinger->setDisplayStateLocked(s); }

    auto onInitializeDisplays() { return mFlinger->onInitializeDisplays(); }

    auto setPowerModeInternal(const sp<DisplayDevice>& hw, int mode, bool stateLockHeld = false) {
        return mFlinger->setPowerModeInternal(hw, mode, stateLockHeld);
    }

    /* ------------------------------------------------------------------------
     * Read-only access to private data to assert post-conditions.
     */

    const auto& getAnimFrameTracker() const { return mFlinger->mAnimFrameTracker; }
    const auto& getHasPoweredOff() const { return mFlinger->mHasPoweredOff; }
    const auto& getHWVsyncAvailable() const { return mFlinger->mHWVsyncAvailable; }
    const auto& getVisibleRegionsDirty() const { return mFlinger->mVisibleRegionsDirty; }

    const auto& getCompositorTiming() const { return mFlinger->getBE().mCompositorTiming; }

    /* ------------------------------------------------------------------------
     * Read-write access to private data to set up preconditions and assert
     * post-conditions.
     */

    auto& mutableHasWideColorDisplay() { return SurfaceFlinger::hasWideColorDisplay; }

    auto& mutableBuiltinDisplays() { return mFlinger->mBuiltinDisplays; }
    auto& mutableCurrentState() { return mFlinger->mCurrentState; }
    auto& mutableDisplays() { return mFlinger->mDisplays; }
    auto& mutableDisplayColorSetting() { return mFlinger->mDisplayColorSetting; }
    auto& mutableDrawingState() { return mFlinger->mDrawingState; }
    auto& mutableEventControlThread() { return mFlinger->mEventControlThread; }
    auto& mutableEventQueue() { return mFlinger->mEventQueue; }
    auto& mutableEventThread() { return mFlinger->mEventThread; }
    auto& mutableHWVsyncAvailable() { return mFlinger->mHWVsyncAvailable; }
    auto& mutableInterceptor() { return mFlinger->mInterceptor; }
    auto& mutableMainThreadId() { return mFlinger->mMainThreadId; }
    auto& mutablePendingHotplugEvents() { return mFlinger->mPendingHotplugEvents; }
    auto& mutablePrimaryHWVsyncEnabled() { return mFlinger->mPrimaryHWVsyncEnabled; }
    auto& mutableTransactionFlags() { return mFlinger->mTransactionFlags; }
    auto& mutableUseHwcVirtualDisplays() { return mFlinger->mUseHwcVirtualDisplays; }

    auto& mutableComposerSequenceId() { return mFlinger->getBE().mComposerSequenceId; }
    auto& mutableHwcDisplayData() { return mFlinger->getBE().mHwc->mDisplayData; }
    auto& mutableHwcDisplaySlots() { return mFlinger->getBE().mHwc->mHwcDisplaySlots; }

    ~TestableSurfaceFlinger() {
        // All these pointer and container clears help ensure that GMock does
        // not report a leaked object, since the SurfaceFlinger instance may
        // still be referenced by something despite our best efforts to destroy
        // it after each test is done.
        mutableDisplays().clear();
        mutableEventControlThread().reset();
        mutableEventQueue().reset();
        mutableEventThread().reset();
        mutableInterceptor().reset();
        mFlinger->getBE().mHwc.reset();
        mFlinger->getBE().mRenderEngine.reset();
    }

    /* ------------------------------------------------------------------------
     * Wrapper classes for Read-write access to private data to set up
     * preconditions and assert post-conditions.
     */

    struct HWC2Display : public HWC2::Display {
        HWC2Display(Hwc2::Composer& composer,
                    const std::unordered_set<HWC2::Capability>& capabilities, hwc2_display_t id,
                    HWC2::DisplayType type)
              : HWC2::Display(composer, capabilities, id, type) {}
        ~HWC2Display() {
            // Prevents a call to disable vsyncs.
            mType = HWC2::DisplayType::Invalid;
        }

        auto& mutableIsConnected() { return this->mIsConnected; }
        auto& mutableConfigs() { return this->mConfigs; }
    };

    class FakeHwcDisplayInjector {
    public:
        static constexpr hwc2_display_t DEFAULT_HWC_DISPLAY_ID = 1000;
        static constexpr int32_t DEFAULT_WIDTH = 1920;
        static constexpr int32_t DEFAULT_HEIGHT = 1280;
        static constexpr int32_t DEFAULT_REFRESH_RATE = 16'666'666;
        static constexpr int32_t DEFAULT_DPI = 320;
        static constexpr int32_t DEFAULT_ACTIVE_CONFIG = 0;

        FakeHwcDisplayInjector(DisplayDevice::DisplayType type, HWC2::DisplayType hwcDisplayType)
              : mType(type), mHwcDisplayType(hwcDisplayType) {}

        auto& setHwcDisplayId(hwc2_display_t displayId) {
            mHwcDisplayId = displayId;
            return *this;
        }

        auto& setWidth(int32_t width) {
            mWidth = width;
            return *this;
        }

        auto& setHeight(int32_t height) {
            mHeight = height;
            return *this;
        }

        auto& setRefreshRate(int32_t refreshRate) {
            mRefreshRate = refreshRate;
            return *this;
        }

        auto& setDpiX(int32_t dpi) {
            mDpiX = dpi;
            return *this;
        }

        auto& setDpiY(int32_t dpi) {
            mDpiY = dpi;
            return *this;
        }

        auto& setActiveConfig(int32_t config) {
            mActiveConfig = config;
            return *this;
        }

        auto& addCapability(HWC2::Capability cap) {
            mCapabilities.emplace(cap);
            return *this;
        }

        void inject(TestableSurfaceFlinger* flinger, Hwc2::Composer* composer) {
            auto display = std::make_unique<HWC2Display>(*composer, mCapabilities, mHwcDisplayId,
                                                         mHwcDisplayType);

            auto config = HWC2::Display::Config::Builder(*display, mActiveConfig);
            config.setWidth(mWidth);
            config.setHeight(mHeight);
            config.setVsyncPeriod(mRefreshRate);
            config.setDpiX(mDpiX);
            config.setDpiY(mDpiY);
            display->mutableConfigs().emplace(mActiveConfig, config.build());
            display->mutableIsConnected() = true;

            ASSERT_TRUE(flinger->mutableHwcDisplayData().size() > static_cast<size_t>(mType));
            flinger->mutableHwcDisplayData()[mType].reset();
            flinger->mutableHwcDisplayData()[mType].hwcDisplay = display.get();
            flinger->mutableHwcDisplaySlots().emplace(mHwcDisplayId, mType);

            flinger->mFakeHwcDisplays.push_back(std::move(display));
        }

    private:
        DisplayDevice::DisplayType mType;
        HWC2::DisplayType mHwcDisplayType;
        hwc2_display_t mHwcDisplayId = DEFAULT_HWC_DISPLAY_ID;
        int32_t mWidth = DEFAULT_WIDTH;
        int32_t mHeight = DEFAULT_HEIGHT;
        int32_t mRefreshRate = DEFAULT_REFRESH_RATE;
        int32_t mDpiX = DEFAULT_DPI;
        int32_t mDpiY = DEFAULT_DPI;
        int32_t mActiveConfig = DEFAULT_ACTIVE_CONFIG;
        std::unordered_set<HWC2::Capability> mCapabilities;
    };

    class FakeDisplayDeviceInjector {
    public:
        FakeDisplayDeviceInjector(TestableSurfaceFlinger& flinger, DisplayDevice::DisplayType type,
                                  int hwcId)
              : mFlinger(flinger), mType(type), mHwcId(hwcId) {}

        sp<IBinder> token() const { return mDisplayToken; }

        DisplayDeviceState& mutableDrawingDisplayState() {
            return mFlinger.mutableDrawingState().displays.editValueFor(mDisplayToken);
        }

        DisplayDeviceState& mutableCurrentDisplayState() {
            return mFlinger.mutableCurrentState().displays.editValueFor(mDisplayToken);
        }

        const auto& getDrawingDisplayState() {
            return mFlinger.mutableDrawingState().displays.valueFor(mDisplayToken);
        }

        const auto& getCurrentDisplayState() {
            return mFlinger.mutableCurrentState().displays.valueFor(mDisplayToken);
        }

        auto& mutableDisplayDevice() { return mFlinger.mutableDisplays().valueFor(mDisplayToken); }

        auto& setNativeWindow(const sp<ANativeWindow>& nativeWindow) {
            mNativeWindow = nativeWindow;
            return *this;
        }

        auto& setDisplaySurface(const sp<DisplaySurface>& displaySurface) {
            mDisplaySurface = displaySurface;
            return *this;
        }

        auto& setRenderSurface(std::unique_ptr<RE::Surface> renderSurface) {
            mRenderSurface = std::move(renderSurface);
            return *this;
        }

        auto& setSecure(bool secure) {
            mSecure = secure;
            return *this;
        }

        sp<DisplayDevice> inject() {
            std::unordered_map<ui::ColorMode, std::vector<ui::RenderIntent>> hdrAndRenderIntents;
            sp<DisplayDevice> device =
                    new DisplayDevice(mFlinger.mFlinger.get(), mType, mHwcId, mSecure, mDisplayToken,
                                      mNativeWindow, mDisplaySurface, std::move(mRenderSurface), 0,
                                      0, false, HdrCapabilities(), 0, hdrAndRenderIntents,
                                      HWC_POWER_MODE_NORMAL);
            mFlinger.mutableDisplays().add(mDisplayToken, device);

            DisplayDeviceState state(mType, mSecure);
            mFlinger.mutableCurrentState().displays.add(mDisplayToken, state);
            mFlinger.mutableDrawingState().displays.add(mDisplayToken, state);

            if (mType >= DisplayDevice::DISPLAY_PRIMARY && mType < DisplayDevice::DISPLAY_VIRTUAL) {
                mFlinger.mutableBuiltinDisplays()[mType] = mDisplayToken;
            }

            return device;
        }

    private:
        TestableSurfaceFlinger& mFlinger;
        sp<BBinder> mDisplayToken = new BBinder();
        DisplayDevice::DisplayType mType;
        int mHwcId;
        sp<ANativeWindow> mNativeWindow;
        sp<DisplaySurface> mDisplaySurface;
        std::unique_ptr<RE::Surface> mRenderSurface;
        bool mSecure = false;
    };

    sp<SurfaceFlinger> mFlinger = new SurfaceFlinger(SurfaceFlinger::SkipInitialization);

    // We need to keep a reference to these so they are properly destroyed.
    std::vector<std::unique_ptr<HWC2Display>> mFakeHwcDisplays;
};

} // namespace android
