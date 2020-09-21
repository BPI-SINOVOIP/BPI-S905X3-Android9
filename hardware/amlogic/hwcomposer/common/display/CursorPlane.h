/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef CURSOR_PLANE_H
#define CURSOR_PLANE_H

#include <HwDisplayPlane.h>
#include <MesonLog.h>
#include "AmFramebuffer.h"

class CursorPlane : public HwDisplayPlane {
public:
    CursorPlane(int32_t drvFd, uint32_t id);
    ~CursorPlane();

    const char * getName();
    uint32_t getPlaneType();
    uint32_t getCapabilities();
    int32_t getFixedZorder();
    uint32_t getPossibleCrtcs();
    bool isFbSupport(std::shared_ptr<DrmFramebuffer> & fb);

    int32_t setPlane(std::shared_ptr<DrmFramebuffer> fb, uint32_t zorder, int blankOp);

    void dump(String8 & dumpstr);

private:
    int32_t setCursorPosition(int32_t x, int32_t y);
    int32_t updatePlaneInfo(int xres, int yres);
    int32_t updateCursorBuffer(std::shared_ptr<DrmFramebuffer> & fb);

    char mName[64];
    int32_t mLastTransform;
    bool mBlank;
    cursor_plane_info_t mPlaneInfo;
    std::shared_ptr<DrmFramebuffer> mDrmFb;
};

 #endif/*CURSOR_PLANE_H*/
