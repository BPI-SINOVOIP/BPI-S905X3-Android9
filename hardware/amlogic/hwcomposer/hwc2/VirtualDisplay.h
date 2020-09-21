/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef VIRTUAL_DISPLAY_H
#define VIRTUAL_DISPLAY_H

#include "Hwc2Display.h"

class VirtualDisplay : public Hwc2Display {
/*HWC2 interfaces.*/
public:
    /*Connector releated.*/
    const char * getName();
    const drm_hdr_capabilities_t * getHdrCapabilities();

    /*Vsync*/
    hwc2_error_t setVsyncEnable(hwc2_vsync_t enabled);

    /*Layer releated.*/
    hwc2_error_t setCursorPosition(hwc2_layer_t layer,
        int32_t x, int32_t y);

    hwc2_error_t setColorTransform(const float* matrix,
        android_color_transform_t hint);
    hwc2_error_t setPowerMode(hwc2_power_mode_t mode);

    /*Compose flow*/
    hwc2_error_t validateDisplay(uint32_t* outNumTypes,
        uint32_t* outNumRequests);
    hwc2_error_t presentDisplay(int32_t* outPresentFence);
    hwc2_error_t acceptDisplayChanges();
    hwc2_error_t getChangedCompositionTypes(
        uint32_t* outNumElements, hwc2_layer_t* outLayers,
        int32_t*  outTypes);
    hwc2_error_t getDisplayRequests(
        int32_t* outDisplayRequests, uint32_t* outNumElements,
        hwc2_layer_t* outLayers,int32_t* outLayerRequests);
    hwc2_error_t setClientTarget( buffer_handle_t target,
        int32_t acquireFence, int32_t dataspace, hwc_region_t damage);
    hwc2_error_t getReleaseFences(uint32_t* outNumElements,
        hwc2_layer_t* outLayers, int32_t* outFences);

    /*display attrbuites*/
    hwc2_error_t  getDisplayConfigs(
        uint32_t* outNumConfigs, hwc2_config_t* outConfigs);
    hwc2_error_t  getDisplayAttribute(
        hwc2_config_t config, int32_t attribute, int32_t* outValue);
    hwc2_error_t getActiveConfig(hwc2_config_t* outConfig);
    hwc2_error_t setActiveConfig(hwc2_config_t config);

    /*set virtual display output buffer.*/
    hwc2_error_t setFormat(int32_t* format);
    hwc2_error_t setOutputBuffer(buffer_handle_t buffer,
        int32_t releaseFence);

/*Additional interfaces.*/
public:
    VirtualDisplay(uint32_t width, uint32_t height);
    ~VirtualDisplay();

    int32_t initialize();
    void dump(String8 & dumpstr);

protected:
    uint mWidth;
    uint mHeight;

    std::shared_ptr<DrmFence> mOutputFence;
    std::shared_ptr<DrmFence> mClientFence;
};


#endif/*VIRTUAL_DISPLAY_H*/
