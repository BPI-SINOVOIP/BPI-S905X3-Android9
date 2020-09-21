/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef IHWC_MODE_MGR_H
#define IHWC_MODE_MGR_H

#include <BasicTypes.h>
#include <HwDisplayConnector.h>
#include <HwDisplayCrtc.h>

typedef enum {
    FIXED_SIZE_POLICY = 0,
    FULL_ACTIVE_POLICY = 1,
    ACTIVE_MODE_POLICY,
    REAL_MODE_POLICY,
} hwc_modes_policy_t;

enum {
    CALL_FROM_SF = 0,
    CALL_FROM_HWC = 1,
};

/*
 *For different connectors, we need different manage strategy.
 *This class defines basic interface to manage display modes.
 */
class HwcModeMgr {
public:
    HwcModeMgr() {}
    virtual ~HwcModeMgr() {}

    virtual hwc_modes_policy_t getPolicyType() = 0;
    virtual const char * getName() = 0;

    virtual void setFramebufferSize(uint32_t w, uint32_t h) = 0;
    virtual void setDisplayResources(
        std::shared_ptr<HwDisplayCrtc> & crtc,
        std::shared_ptr<HwDisplayConnector> & connector) = 0;
    virtual int32_t update() = 0;
    virtual bool needCallHotPlug() = 0;
    virtual int32_t getDisplayMode(drm_mode_info_t & mode) = 0;

    virtual int32_t  getDisplayConfigs(
        uint32_t* outNumConfigs, uint32_t* outConfigs) = 0;
    virtual int32_t  getDisplayAttribute(
        uint32_t config, int32_t attribute, int32_t* outValue,
        int32_t caller = CALL_FROM_HWC) = 0;
    virtual int32_t getActiveConfig(uint32_t * outConfig,
        int32_t caller = CALL_FROM_HWC) = 0;
    virtual int32_t setActiveConfig(uint32_t config) = 0;
    virtual void resetTags() = 0;
    virtual void dump(String8 & dumpstr) = 0;
};

std::shared_ptr<HwcModeMgr> createModeMgr(hwc_modes_policy_t policy);

#endif/*IHWC_MODE_MGR_H*/
