/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef MESON_HWC2_H
#define MESON_HWC2_H

#include <map>
#include <hardware/hardware.h>
#include <HwcDisplayPipe.h>

#include "Hwc2Display.h"


class MesonHwc2 {
/*hwc2 interface*/
public:
    void getCapabilities(uint32_t* outCount, int32_t* outCapabilities);
    int32_t getClientTargetSupport( hwc2_display_t display,
        uint32_t width, uint32_t height, int32_t format, int32_t dataspace);

    int32_t registerCallback(int32_t descriptor,
        hwc2_callback_data_t callbackData, hwc2_function_pointer_t pointer);

    /*Virtual display functions*/
    int32_t createVirtualDisplay(uint32_t width, uint32_t height,
        int32_t* format, hwc2_display_t* outDisplay);
    int32_t destroyVirtualDisplay(hwc2_display_t display);
    uint32_t getMaxVirtualDisplayCount();

    /*display functions*/
    int32_t  acceptDisplayChanges(hwc2_display_t display);
    int32_t getActiveConfig(hwc2_display_t display, hwc2_config_t* outConfig);
    int32_t  getDisplayConfigs(hwc2_display_t display,
        uint32_t* outNumConfigs, hwc2_config_t* outConfigs);

    int32_t  getColorModes(hwc2_display_t display, uint32_t* outNumModes,
        int32_t*  outModes);
    int32_t  getDisplayAttribute( hwc2_display_t display, hwc2_config_t config,
        int32_t attribute, int32_t* outValue);
    int32_t getDisplayName(hwc2_display_t display, uint32_t* outSize, char* outName);
    int32_t getDisplayRequests(hwc2_display_t display,
        int32_t* outDisplayRequests, uint32_t* outNumElements,
        hwc2_layer_t* outLayers, int32_t* outLayerRequests);
    int32_t  getDisplayType(hwc2_display_t display, int32_t* outType);
    int32_t getDozeSupport(hwc2_display_t display, int32_t* outSupport);
    int32_t getHdrCapabilities(hwc2_display_t display, uint32_t* outNumTypes,
        int32_t* outTypes, float* outMaxLuminance,
        float* outMaxAverageLuminance, float* outMinLuminance);
    int32_t presentDisplay(hwc2_display_t display, int32_t* outPresentFence);
    int32_t setActiveConfig(hwc2_display_t display, hwc2_config_t config);
    int32_t setClientTarget(hwc2_display_t display, buffer_handle_t target,
        int32_t acquireFence, int32_t dataspace, hwc_region_t damage);
    int32_t setColorMode(hwc2_display_t display, int32_t mode);
    int32_t setColorTransform(hwc2_display_t display, const float* matrix,
        int32_t hint);
    int32_t setOutputBuffer(hwc2_display_t display, buffer_handle_t buffer,
        int32_t releaseFence);
    int32_t setPowerMode(hwc2_display_t display, int32_t mode);
    int32_t setVsyncEnable(hwc2_display_t display, int32_t enabled);
    int32_t getReleaseFences(hwc2_display_t display, uint32_t* outNumElements,
        hwc2_layer_t* outLayers, int32_t* outFences);
    int32_t validateDisplay(hwc2_display_t display, uint32_t* outNumTypes,
        uint32_t* outNumRequests);
    int32_t getChangedCompositionTypes(hwc2_display_t display,
        uint32_t* outNumElements, hwc2_layer_t* outLayers, int32_t*  outTypes);
    int32_t setCalibrateInfo(hwc2_display_t display);

    /*layer functions*/
    int32_t createLayer(hwc2_display_t display, hwc2_layer_t* outLayer);
    int32_t destroyLayer(hwc2_display_t display, hwc2_layer_t layer);
    int32_t setCursorPosition(hwc2_display_t display, hwc2_layer_t layer,
        int32_t x, int32_t y);
    int32_t setLayerBuffer(hwc2_display_t display, hwc2_layer_t layer,
        buffer_handle_t buffer, int32_t acquireFence);
    int32_t setLayerSurfaceDamage(hwc2_display_t display, hwc2_layer_t layer,
        hwc_region_t damage);
    int32_t setLayerBlendMode(hwc2_display_t display, hwc2_layer_t layer,
        int32_t mode);
    int32_t setLayerColor(hwc2_display_t display, hwc2_layer_t layer,
        hwc_color_t color);
    int32_t setLayerCompositionType(hwc2_display_t display, hwc2_layer_t layer,
        int32_t type);
    int32_t setLayerDataspace(hwc2_display_t display, hwc2_layer_t layer,
        int32_t dataspace);
    int32_t setLayerDisplayFrame(hwc2_display_t display, hwc2_layer_t layer,
        hwc_rect_t frame);
    int32_t setLayerPlaneAlpha(hwc2_display_t display, hwc2_layer_t layer,
        float alpha);
    int32_t setLayerSidebandStream(hwc2_display_t display, hwc2_layer_t layer,
        const native_handle_t* stream);
    int32_t setLayerSourceCrop(hwc2_display_t display, hwc2_layer_t layer,
        hwc_frect_t crop);
    int32_t setLayerTransform(hwc2_display_t display, hwc2_layer_t layer,
        int32_t transform);
    int32_t setLayerVisibleRegion(hwc2_display_t display, hwc2_layer_t layer,
        hwc_region_t visible);
    int32_t  setLayerZorder(hwc2_display_t display, hwc2_layer_t layer,
        uint32_t z);

#ifdef HWC_HDR_METADATA_SUPPORT
    int32_t setLayerPerFrameMetadata(
            hwc2_display_t display, hwc2_layer_t layer,
            uint32_t numElements, const int32_t* /*hw2_per_frame_metadata_key_t*/ keys,
            const float* metadata);
    int32_t getPerFrameMetadataKeys(
            hwc2_display_t display,
            uint32_t* outNumKeys,
            int32_t* /*hwc2_per_frame_metadata_key_t*/ outKeys);
#endif

    void dump(uint32_t* outSize, char* outBuffer);

/*amlogic ext display interface*/
public:
    int32_t setPostProcessor(bool bEnable);

    uint32_t getDisplayRequest();
    int32_t handleDisplayRequest(uint32_t request);

/*implement*/
public:
    void refresh(hwc2_display_t  display);
    void onVsync(hwc2_display_t display, int64_t timestamp);
    void onHotplug(hwc2_display_t display, bool connected);

public:
    MesonHwc2();
    virtual ~MesonHwc2();

protected:
    int32_t initialize();
    bool isDisplayValid(hwc2_display_t display);

    uint32_t getVirtualDisplayId();
    void freeVirtualDisplayId(uint32_t id);

protected:
    std::map<hwc2_display_t, std::shared_ptr<Hwc2Display>> mDisplays;

    std::shared_ptr<HwcDisplayPipe> mDisplayPipe;

    HWC2_PFN_HOTPLUG mHotplugFn;
    hwc2_callback_data_t mHotplugData;
    HWC2_PFN_REFRESH mRefreshFn;
    hwc2_callback_data_t mRefreshData;
    HWC2_PFN_VSYNC mVsyncFn;
    hwc2_callback_data_t mVsyncData;

    uint32_t mVirtualDisplayIds;

    uint32_t mDisplayRequests;
};

#endif/*MESON_HWC2_H*/
