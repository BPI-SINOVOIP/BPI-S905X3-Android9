/*
 * Copyright 2018 The Android Open Source Project
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

#include <string>
#include <vector>

#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <android/hardware/graphics/composer/2.1/IComposerClient.h>

// TODO remove hwcomposer2.h dependency
#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_1 {
namespace hal {

using common::V1_0::ColorMode;
using common::V1_0::ColorTransform;
using common::V1_0::Dataspace;
using common::V1_0::Hdr;
using common::V1_0::PixelFormat;
using common::V1_0::Transform;

class ComposerHal {
   public:
    virtual ~ComposerHal() = default;

    virtual bool hasCapability(hwc2_capability_t capability) = 0;

    // dump the debug information
    virtual std::string dumpDebugInfo() = 0;

    class EventCallback {
       public:
        virtual ~EventCallback() = default;
        virtual void onHotplug(Display display, IComposerCallback::Connection connected) = 0;
        virtual void onRefresh(Display display) = 0;
        virtual void onVsync(Display display, int64_t timestamp) = 0;
    };

    // Register the event callback object.  The event callback object is valid
    // until it is unregistered.  A hotplug event must be generated for each
    // connected physical display, before or after this function returns.
    virtual void registerEventCallback(EventCallback* callback) = 0;

    // Unregister the event callback object.  It must block if there is any
    // callback in-flight.
    virtual void unregisterEventCallback() = 0;

    virtual uint32_t getMaxVirtualDisplayCount() = 0;
    virtual Error createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat* format,
                                       Display* outDisplay) = 0;
    virtual Error destroyVirtualDisplay(Display display) = 0;
    virtual Error createLayer(Display display, Layer* outLayer) = 0;
    virtual Error destroyLayer(Display display, Layer layer) = 0;

    virtual Error getActiveConfig(Display display, Config* outConfig) = 0;
    virtual Error getClientTargetSupport(Display display, uint32_t width, uint32_t height,
                                         PixelFormat format, Dataspace dataspace) = 0;
    virtual Error getColorModes(Display display, hidl_vec<ColorMode>* outModes) = 0;
    virtual Error getDisplayAttribute(Display display, Config config,
                                      IComposerClient::Attribute attribute, int32_t* outValue) = 0;
    virtual Error getDisplayConfigs(Display display, hidl_vec<Config>* outConfigs) = 0;
    virtual Error getDisplayName(Display display, hidl_string* outName) = 0;
    virtual Error getDisplayType(Display display, IComposerClient::DisplayType* outType) = 0;
    virtual Error getDozeSupport(Display display, bool* outSupport) = 0;
    virtual Error getHdrCapabilities(Display display, hidl_vec<Hdr>* outTypes,
                                     float* outMaxLuminance, float* outMaxAverageLuminance,
                                     float* outMinLuminance) = 0;

    virtual Error setActiveConfig(Display display, Config config) = 0;
    virtual Error setColorMode(Display display, ColorMode mode) = 0;
    virtual Error setPowerMode(Display display, IComposerClient::PowerMode mode) = 0;
    virtual Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled) = 0;

    virtual Error setColorTransform(Display display, const float* matrix, int32_t hint) = 0;
    virtual Error setClientTarget(Display display, buffer_handle_t target, int32_t acquireFence,
                                  int32_t dataspace, const std::vector<hwc_rect_t>& damage) = 0;
    virtual Error setOutputBuffer(Display display, buffer_handle_t buffer,
                                  int32_t releaseFence) = 0;
    virtual Error validateDisplay(Display display, std::vector<Layer>* outChangedLayers,
                                  std::vector<IComposerClient::Composition>* outCompositionTypes,
                                  uint32_t* outDisplayRequestMask,
                                  std::vector<Layer>* outRequestedLayers,
                                  std::vector<uint32_t>* outRequestMasks) = 0;
    virtual Error acceptDisplayChanges(Display display) = 0;
    virtual Error presentDisplay(Display display, int32_t* outPresentFence,
                                 std::vector<Layer>* outLayers,
                                 std::vector<int32_t>* outReleaseFences) = 0;

    virtual Error setLayerCursorPosition(Display display, Layer layer, int32_t x, int32_t y) = 0;
    virtual Error setLayerBuffer(Display display, Layer layer, buffer_handle_t buffer,
                                 int32_t acquireFence) = 0;
    virtual Error setLayerSurfaceDamage(Display display, Layer layer,
                                        const std::vector<hwc_rect_t>& damage) = 0;
    virtual Error setLayerBlendMode(Display display, Layer layer, int32_t mode) = 0;
    virtual Error setLayerColor(Display display, Layer layer, IComposerClient::Color color) = 0;
    virtual Error setLayerCompositionType(Display display, Layer layer, int32_t type) = 0;
    virtual Error setLayerDataspace(Display display, Layer layer, int32_t dataspace) = 0;
    virtual Error setLayerDisplayFrame(Display display, Layer layer, const hwc_rect_t& frame) = 0;
    virtual Error setLayerPlaneAlpha(Display display, Layer layer, float alpha) = 0;
    virtual Error setLayerSidebandStream(Display display, Layer layer, buffer_handle_t stream) = 0;
    virtual Error setLayerSourceCrop(Display display, Layer layer, const hwc_frect_t& crop) = 0;
    virtual Error setLayerTransform(Display display, Layer layer, int32_t transform) = 0;
    virtual Error setLayerVisibleRegion(Display display, Layer layer,
                                        const std::vector<hwc_rect_t>& visible) = 0;
    virtual Error setLayerZOrder(Display display, Layer layer, uint32_t z) = 0;
};

}  // namespace hal
}  // namespace V2_1
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android
