/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef HWC2_LAYER_H
#define HWC2_LAYER_H

#include <hardware/hwcomposer2.h>

#include <BasicTypes.h>
#include <DrmFramebuffer.h>


class Hwc2Layer : public DrmFramebuffer {
/*Interfaces for hwc2.0 api.*/
public:
    /*set layer data info, only one of tree functions called.*/
    hwc2_error_t setBuffer(buffer_handle_t buffer, int32_t acquireFence);
    hwc2_error_t setSidebandStream(const native_handle_t* stream);
    hwc2_error_t setColor(hwc_color_t color);

    hwc2_error_t setSourceCrop(hwc_frect_t crop);
    hwc2_error_t setDisplayFrame(hwc_rect_t frame);
    hwc2_error_t setBlendMode(hwc2_blend_mode_t mode);
    hwc2_error_t setPlaneAlpha(float alpha);
    hwc2_error_t setTransform(hwc_transform_t transform);
    hwc2_error_t setVisibleRegion(hwc_region_t visible);
    hwc2_error_t setSurfaceDamage(hwc_region_t damage);
    hwc2_error_t setCompositionType(hwc2_composition_t type);
    hwc2_error_t setDataspace(android_dataspace_t dataspace);
    hwc2_error_t setZorder(uint32_t z);
#ifdef HWC_HDR_METADATA_SUPPORT
    int32_t setPerFrameMetadata(
            uint32_t numElements, const int32_t* /*hw2_per_frame_metadata_key_t*/ keys,
            const float* metadata);
#endif

/*Extend api.*/
public:
    Hwc2Layer();
    virtual ~Hwc2Layer();

    bool isSecure() { return mSecure;}

    void setUniqueId(hwc2_layer_t id);
    hwc2_layer_t getUniqueId();

    int32_t commitCompType(hwc2_composition_t hwcComp);

    bool isUpdateZorder() { return mUpdateZorder;}
    void updateZorder(bool update);

public:
    android_dataspace_t mDataSpace;
    hwc2_composition_t mHwcCompositionType;
    hwc_region_t mVisibleRegion;
    hwc_region_t mDamageRegion;
    hwc2_layer_t mId;
    drm_rect_t mBackupDisplayFrame;

protected:
    hwc2_error_t handleDimLayer(buffer_handle_t buffer);

protected:
    bool mUpdateZorder;

};


#endif/*HWC2_LAYER_H*/
