/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef HWC_DISPLAY_H
#define HWC_DISPLAY_H

#include <HwDisplayCrtc.h>
#include <HwDisplayPlane.h>
#include <HwDisplayConnector.h>
#include <HwcModeMgr.h>
#include <HwcVsync.h>

#include <HwcPostProcessor.h>


class HwcDisplay {
public:
    HwcDisplay() { }
    virtual ~HwcDisplay() { }

    virtual int32_t initialize() = 0;

    virtual int32_t setDisplayResource(
        std::shared_ptr<HwDisplayCrtc> & crtc,
        std::shared_ptr<HwDisplayConnector> & connector,
        std::vector<std::shared_ptr<HwDisplayPlane>> & planes) = 0;
    virtual int32_t setModeMgr(std::shared_ptr<HwcModeMgr> & mgr) = 0;
    virtual int32_t setPostProcessor(
        std::shared_ptr<HwcPostProcessor> processor) = 0;
    virtual int32_t setVsync(std::shared_ptr<HwcVsync> vsync) = 0;
    virtual int32_t blankDisplay() = 0;

    virtual void onHotplug(bool connected) = 0;
    virtual void onUpdate(bool bHdcp) = 0;
    virtual void onModeChanged(int stage) = 0;
    virtual void cleanupBeforeDestroy() = 0;
};

#endif
