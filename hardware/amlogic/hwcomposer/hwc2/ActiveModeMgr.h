/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef ACTIVE_MODE_MGR_H
#define ACTIVE_MODE_MGR_H

#include "HwcModeMgr.h"

/*
 * ActiveModeMgr:
 * This class designed for removeable device(hdmi, cvbs)
 * to support real activeModes.
 * Config list will changed when device disconnect/connect.
 */
class  ActiveModeMgr : public  HwcModeMgr {
public:
     ActiveModeMgr();
    ~ ActiveModeMgr();

    hwc_modes_policy_t getPolicyType();
    const char * getName();

    void setFramebufferSize(uint32_t w, uint32_t h);
    void setDisplayResources(
        std::shared_ptr<HwDisplayCrtc> & crtc,
        std::shared_ptr<HwDisplayConnector> & connector);
    int32_t update();
    bool needCallHotPlug(){return mCallOnHotPlug;};
    int32_t getDisplayMode(drm_mode_info_t & mode);

    int32_t  getDisplayConfigs(
        uint32_t* outNumConfigs, uint32_t * outConfigs);
    int32_t  getDisplayAttribute(
        uint32_t config, int32_t attribute, int32_t* outValue, int32_t caller);
    int32_t getActiveConfig(uint32_t * outConfig, int32_t caller);
    int32_t setActiveConfig(uint32_t configId);
    void resetTags();
    void dump(String8 & dumpstr);

protected:
    int32_t initDefaultDispResources();
    int32_t updateHwcDispConfigs();
    int32_t updateSfDispConfigs();
    int32_t updateHwcActiveConfig(const char * activeMode);
    int32_t updateSfActiveConfig(uint32_t config, drm_mode_info_t cfg);
    void getActiveHwcMeta(const char * activeMode);
    bool isFracRate(float refreshRate);
    void reset();
    const drm_mode_info_t findMatchedMode(
        uint32_t width, uint32_t height, float refreshrate);


protected:
    std::shared_ptr<HwDisplayConnector> mConnector;
    std::shared_ptr<HwDisplayCrtc> mCrtc;

    uint32_t mFbWidth;
    uint32_t mFbHeight;

    //for activeHwc w&h&refresh rate
    uint32_t mActiveHwcW;
    uint32_t mActiveHwcH;
    float mActiveHwcRate;

    bool mIsInit; // first boot flag
    bool mExtModeSet; // setActiveConfig() flag
    bool mDefaultModeSupport;
    bool mCallOnHotPlug;
    std::string mActiveConfigStr;
    uint32_t mFakeConfigId;
    bool useFakeMode;
    drm_mode_info_t mDefaultMode;

    // Used for HWC
    uint32_t mHwcActiveConfigId;
    std::map<uint32_t, drm_mode_info_t> mHwcActiveModes;

    // Passed to SF
    uint32_t mSfActiveConfigId;
    std::map<uint32_t, drm_mode_info_t> mSfActiveModes;

};

#endif/*ACTIVE_MODE_MGR_H*/
