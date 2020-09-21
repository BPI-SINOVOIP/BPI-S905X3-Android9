/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef DUMMY_PLANE_H
#define DUMMY_PLANE_H
#include <HwDisplayPlane.h>


class DummyPlane : public HwDisplayPlane {
public:
    DummyPlane(int32_t drvFd, uint32_t id);

    uint32_t getPlaneType();
    uint32_t getCapabilities();

    int32_t setPlane(std::shared_ptr<DrmFramebuffer> fb, uint32_t zorder, int blankOp);

    void dump(String8 & dumpstr);
};

#endif/*DUMMY_PLANE_H*/
