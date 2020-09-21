/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

 #ifndef HWC_VIDEO_PLANE_H
#define HWC_VIDEO_PLANE_H

#include <HwDisplayPlane.h>

class HwcVideoPlane : public HwDisplayPlane {
public:
    HwcVideoPlane(int32_t drvFd, uint32_t id);
    ~HwcVideoPlane();

    const char * getName();
    uint32_t getPlaneType();
    uint32_t getCapabilities();
    int32_t getFixedZorder();
    uint32_t getPossibleCrtcs();
    bool isFbSupport(std::shared_ptr<DrmFramebuffer> & fb);

    int32_t setPlane(std::shared_ptr<DrmFramebuffer> fb, uint32_t zorder, int blankOp);

    void dump(String8 & dumpstr);

protected:
    char mName[64];
};

 #endif/*HWC_VIDEO_PLANE_H*/
