/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef HW_DISPLAY_CRTC_H
#define HW_DISPLAY_CRTC_H

#include <stdlib.h>
#include <DrmTypes.h>
#include <BasicTypes.h>
#include <HwDisplayConnector.h>
#include <HwDisplayPlane.h>

class HwDisplayPlane;

#define CRTC_VOUT1 (1 << 0)
#define CRTC_VOUT2 (1 << 1)

class HwDisplayCrtc {
public:
    HwDisplayCrtc(int drvFd, int32_t id);
    ~HwDisplayCrtc();

    int32_t bind(std::shared_ptr<HwDisplayConnector>  connector,
                   std::vector<std::shared_ptr<HwDisplayPlane>> planes);
    int32_t unbind();

    /*load the fixed informations: displaymode list, hdr cap, etc...*/
    int32_t loadProperities();
    int32_t getHdrMetadataKeys(std::vector<drm_hdr_meatadata_t> & keys);

    /*update the dynamic informations, current display mode now.*/
     int32_t update();
    int32_t setHdrMetadata(std::map<drm_hdr_meatadata_t, float> & hdrmedata);

    /*get current display mode.*/
    int32_t getMode(drm_mode_info_t & mode);
    /*set current display mode.*/
    int32_t setMode(drm_mode_info_t & mode);
    int32_t waitVBlank(nsecs_t & timestamp);

    int32_t getId();

    /*Functions for compose & pageflip*/
    /*set the crtc display axis, and source axis,
    * it is used to do scale in vpu.
    */
    int32_t setDisplayFrame(display_zoom_info_t & info);
    /*
    * set if we need compose all ui layers into one display channel.
    * TODO: need pass it in a general way.
    */
    int32_t setOsdChannels(int32_t channels);

    int32_t pageFlip(int32_t & out_fence);

    int32_t readCurDisplayMode(std::string & dispmode);
    int32_t writeCurDisplayMode(std::string & dispmode);
    int32_t writeCurDisplayAttr(std::string & dispattr);


protected:
    void closeLogoDisplay();
    bool updateHdrMetadata(std::map<drm_hdr_meatadata_t, float> & hdrmedata);

protected:
    int32_t mId;
    int mDrvFd;
    uint32_t mOsdChannels;

    bool mFirstPresent;
    bool mConnected;

    drm_mode_info_t mCurModeInfo;
    display_zoom_info_t mScaleInfo;

    std::map<uint32_t, drm_mode_info_t> mModes;
    std::shared_ptr<HwDisplayConnector>  mConnector;
    std::vector<std::shared_ptr<HwDisplayPlane>> mPlanes;

    void * hdrVideoInfo;
    bool mBinded;

    std::mutex mMutex;
};

#endif/*HW_DISPLAY_CRTC_H*/
