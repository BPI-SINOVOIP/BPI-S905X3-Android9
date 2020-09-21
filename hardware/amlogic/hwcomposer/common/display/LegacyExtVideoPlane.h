/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

 #ifndef LEGACY_EXT_VIDEO_PLANE_H
#define LEGACY_EXT_VIDEO_PLANE_H

#include <HwDisplayPlane.h>

class LegacyExtVideoPlane : public HwDisplayPlane {
public:
    LegacyExtVideoPlane(int32_t drvFd, uint32_t id);
    ~LegacyExtVideoPlane();

    const char * getName();
    uint32_t getPlaneType();
    uint32_t getCapabilities();
    int32_t getFixedZorder();
    uint32_t getPossibleCrtcs();
    bool isFbSupport(std::shared_ptr<DrmFramebuffer> & fb);

    int32_t setPlane(std::shared_ptr<DrmFramebuffer> fb, uint32_t zorder, int blankOp);

    void dump(String8 & dumpstr);

protected:
    int32_t getOmxKeepLastFrame(unsigned int & keep);
    bool shouldUpdateAxis(std::shared_ptr<DrmFramebuffer> & fb);

    int32_t getMute(bool& status);
    int32_t setMute(bool status);
    int32_t setZorder(uint32_t zorder);

    int32_t getVideodisableStatus(int & status);
    int32_t setVideodisableStatus(int status);

protected:
    char mName[64];
    unsigned int mOmxKeepLastFrame;

    bool mPlaneMute;

    int32_t mBackupTransform;
    drm_rect_t mBackupDisplayFrame;
    std::shared_ptr<DrmFramebuffer> mLegacyExtVideoFb;
};

 #endif/*LEGACY_EXT_VIDEO_PLANE_H*/
