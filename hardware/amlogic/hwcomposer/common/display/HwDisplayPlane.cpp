/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <unistd.h>

#include <HwDisplayPlane.h>

HwDisplayPlane::HwDisplayPlane(int32_t drvFd, uint32_t id) {
    mDrvFd = drvFd;
    mId = id;
    mIdle = false;
}

HwDisplayPlane::~HwDisplayPlane() {
    close(mDrvFd);
}

