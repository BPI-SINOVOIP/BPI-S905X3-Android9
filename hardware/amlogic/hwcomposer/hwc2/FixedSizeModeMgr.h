/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef FIXED_SIZE_MODE_MGR_H
#define FIXED_SIZE_MODE_MGR_H

#include "HwcModeMgr.h"

/*
 * FixedSizeModeMgr:
 * This class designed for TV or reference box.
 * The display size is fixed (the framebuffer size).
 * But the refresh rate, dpi will update when display changed.
 * For example:user choosed a new display mode in setting.
 */
class FixedSizeModeMgr : public HwcModeMgr {
public:
    FixedSizeModeMgr();
    ~FixedSizeModeMgr();

    hwc_modes_policy_t getPolicyType();
    const char * getName();

    void setFramebufferSize(uint32_t w, uint32_t h);
    void setDisplayResources(
    std::shared_ptr<HwDisplayCrtc> & crtc,
    std::shared_ptr<HwDisplayConnector> & connector);
    int32_t update();
    bool needCallHotPlug(){return true;};
    int32_t getDisplayMode(drm_mode_info_t & mode);

    int32_t  getDisplayConfigs(
    uint32_t * outNumConfigs, uint32_t * outConfigs);
    int32_t  getDisplayAttribute(
    uint32_t config, int32_t attribute, int32_t* outValue, int32_t caller);
    int32_t getActiveConfig(uint32_t * outConfig, int32_t caller);
    int32_t setActiveConfig(uint32_t config);
    void resetTags(){};
    void dump(String8 & dumpstr);

protected:
    std::shared_ptr<HwDisplayConnector> mConnector;
    std::shared_ptr<HwDisplayCrtc> mCrtc;

    std::map<uint32_t, drm_mode_info_t> mModes;
    drm_mode_info_t mCurMode;

};

#endif/*FIXED_SIZE_MODE_MGR_H*/
