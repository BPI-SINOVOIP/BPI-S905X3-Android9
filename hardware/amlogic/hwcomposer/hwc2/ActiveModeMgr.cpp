/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "ActiveModeMgr.h"
#include <HwcConfig.h>
#include <MesonLog.h>
#include <systemcontrol.h>
#include <hardware/hwcomposer2.h>

#include <string>
#include <math.h>


#define DEFUALT_DPI (160)
#define DEFAULT_REFRESH_RATE_60 (60.0f)
#define DEFAULT_REFRESH_RATE_50 (50.0f)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* used for init default configs */
static const drm_mode_info_t mode_info[] = {
    { /* VMODE_720P */
        .name              = "720p60hz",
        .dpiX              = DEFUALT_DPI,
        .dpiY              = DEFUALT_DPI,
        .pixelW            = 1280,
        .pixelH            = 720,
        .refreshRate       = DEFAULT_REFRESH_RATE_60,
    },
    { /* VMODE_1080P */
        .name              = "1080p60hz",
        .dpiX              = DEFUALT_DPI,
        .dpiY              = DEFUALT_DPI,
        .pixelW            = 1920,
        .pixelH            = 1080,
        .refreshRate       = DEFAULT_REFRESH_RATE_60,
    },
    { /* VMODE_720P_50hz */
        .name              = "720p50hz",
        .dpiX              = DEFUALT_DPI,
        .dpiY              = DEFUALT_DPI,
        .pixelW            = 1280,
        .pixelH            = 720,
        .refreshRate       = DEFAULT_REFRESH_RATE_50,
    },
    { /* VMODE_1080P_50HZ */
        .name              = "1080p50hz",
        .dpiX              = DEFUALT_DPI,
        .dpiY              = DEFUALT_DPI,
        .pixelW            = 1920,
        .pixelH            = 1080,
        .refreshRate       = DEFAULT_REFRESH_RATE_50,
    },
    { /* DefaultMode */
        .name              = "DefaultMode",
        .dpiX              = DEFUALT_DPI,
        .dpiY              = DEFUALT_DPI,
        .pixelW            = 1920,
        .pixelH            = 1080,
        .refreshRate       = DEFAULT_REFRESH_RATE_60,
    },
};

ActiveModeMgr::ActiveModeMgr()
    : mIsInit(true) {
}

ActiveModeMgr::~ActiveModeMgr() {

}

hwc_modes_policy_t ActiveModeMgr::getPolicyType() {
    return ACTIVE_MODE_POLICY;
}

const char * ActiveModeMgr::getName() {
    return "ActiveMode";
}

void ActiveModeMgr::setFramebufferSize(uint32_t w, uint32_t h) {
    mFbWidth = w;
    mFbHeight = h;
}

void ActiveModeMgr::setDisplayResources(
    std::shared_ptr<HwDisplayCrtc> & crtc,
    std::shared_ptr<HwDisplayConnector> & connector) {
    mConnector = connector;
    mCrtc = crtc;

    /*
     * Only when it is first boot will come here,
     * initialize a default display mode according to the given FB size,
     * and will return this default mode to SF before it calls setActiveConfig().
     */
    if (mIsInit) {
        mIsInit = false;
        reset();
        initDefaultDispResources();
    }
}

int32_t ActiveModeMgr::initDefaultDispResources() {
    mDefaultMode =
        findMatchedMode(mFbWidth, mFbHeight, DEFAULT_REFRESH_RATE_60);
    mHwcActiveModes.emplace(mHwcActiveModes.size(), mDefaultMode);
    updateHwcActiveConfig(mDefaultMode.name);
    MESON_LOGV("initDefaultDispResources (%s)", mDefaultMode.name);
    return 0;
}

int32_t ActiveModeMgr::update() {
    MESON_LOG_FUN_ENTER();
    useFakeMode = false;
    if (mConnector->isConnected()) {
        //buidl config lists for hwc and sf
        drm_mode_info_t dispmode;
        int32_t rnt;
        rnt = getDisplayMode(dispmode);
        MESON_LOGD("ActiveModeMgr::update() dispmode.name [%s] mExtModeSet:%d rnt %d", dispmode.name, mExtModeSet, rnt);
        if (rnt == 0) {
            if (!mExtModeSet) {
                updateHwcDispConfigs();
                updateSfDispConfigs();
            }
        } else {
            useFakeMode = true;
            MESON_LOGD("Get invalid display mode.");
        }
    } else
        useFakeMode = true;

    if (useFakeMode)
        updateHwcActiveConfig(mDefaultMode.name);

    MESON_LOG_FUN_LEAVE();
    return 0;
}

int32_t ActiveModeMgr::getDisplayMode(drm_mode_info_t & mode) {
    return mCrtc->getMode(mode);
}

int32_t ActiveModeMgr::getDisplayConfigs(
    uint32_t * outNumConfigs, uint32_t * outConfigs) {
    *outNumConfigs = mSfActiveModes.size();
    MESON_LOGD("ActiveModeMgr::getDisplayConfigs(a,b)");
    if (outConfigs) {
        updateSfDispConfigs();
        std::map<uint32_t, drm_mode_info_t>::iterator it =
            mSfActiveModes.begin();
        for (uint32_t index = 0; it != mSfActiveModes.end(); ++it, ++index) {
            outConfigs[index] = it->first;
            MESON_LOGV("outConfig[%d]: %d.", index, outConfigs[index]);
        }
    }
    return HWC2_ERROR_NONE;
}


bool ActiveModeMgr::isFracRate(float refreshRate) {
    return refreshRate > floor(refreshRate) ? true : false;
}

void ActiveModeMgr::getActiveHwcMeta(const char * activeMode) {
    for (auto it = mHwcActiveModes.begin(); it != mHwcActiveModes.end(); ++it) {
        if (strncmp(activeMode, it->second.name, DRM_DISPLAY_MODE_LEN) == 0
                && !isFracRate(it->second.refreshRate)) {
            mActiveHwcH = it->second.pixelH;
            mActiveHwcW = it->second.pixelW;
            mActiveHwcRate = it->second.refreshRate;
        }
    }
}

int32_t ActiveModeMgr::updateHwcDispConfigs() {
    std::map<uint32_t, drm_mode_info_t> supportedModes;
    drm_mode_info_t activeMode;
    mHwcActiveModes.clear();
    MESON_LOGD("ActiveModeMgr::updateHwcDispConfigs()");
    mConnector->getModes(supportedModes);
    if (mCrtc->getMode(activeMode) != 0) {
        activeMode = mDefaultMode;
    }
    mActiveConfigStr = activeMode.name;
    for (auto it = supportedModes.begin(); it != supportedModes.end(); ++it) {
        // skip default / fake active mode as we add it to the end
        if (!strncmp(activeMode.name, it->second.name, DRM_DISPLAY_MODE_LEN)
            && activeMode.refreshRate == it->second.refreshRate
            && !isFracRate(activeMode.refreshRate)) {
            mActiveConfigStr = activeMode.name;
        } else {
            MESON_LOGV("[%s]: add Hwc modes %s.", __func__, activeMode.name);
            mHwcActiveModes.emplace(mHwcActiveModes.size(), it->second);
        }
    }

    mHwcActiveModes.emplace(mHwcActiveModes.size(), activeMode);
    mConnector->setMode(activeMode);
    mHwcActiveConfigId = mHwcActiveModes.size() - 1;
    MESON_LOGD("[%s]: updateHwcDispConfigs Hwc active modes %s. refreshRate %f",
                __func__, activeMode.name, activeMode.refreshRate);
    return HWC2_ERROR_NONE;
}

int32_t ActiveModeMgr::updateSfDispConfigs() {
    // clear display modes
    mSfActiveModes.clear();

    std::map<uint32_t, drm_mode_info_t> mTmpModes;
    mTmpModes = mHwcActiveModes;

    MESON_LOGD("ActiveModeMgr::updateSfDispConfigs()");

    std::map<float, drm_mode_info_t> tmpList;
    std::map<float, drm_mode_info_t>::iterator tmpIt;

    for (auto it = mTmpModes.begin(); it != mTmpModes.end(); ++it) {
        //first check the fps, if there is not the same fps, add it to sf list
        //then check the width, add the biggest width one.
        drm_mode_info_t cfg = it->second;
        std::string cMode = cfg.name;
        float cFps = cfg.refreshRate;
        uint32_t cWidth = cfg.pixelW;

        tmpIt = tmpList.find(cFps);
        if (tmpIt != tmpList.end()) {
            drm_mode_info_t iCfg = tmpIt->second;
            uint32_t iWidth = iCfg.pixelW;
            if ((cWidth >= iWidth) && (cWidth != 4096)) {
                tmpList.erase(tmpIt);
                tmpList.emplace(cFps, it->second);
            }
        } else {
                tmpList.emplace(cFps, it->second);
        }
    }

    getActiveHwcMeta(mActiveConfigStr.c_str());

    for (tmpIt = tmpList.begin(); tmpIt != tmpList.end(); ++tmpIt) {
        drm_mode_info_t config = tmpIt->second;
        float cRate = config.refreshRate;
        //judge whether it is the current display mode and add it to the end of the SF list
        if ((cRate != mActiveHwcRate)) {
            mSfActiveModes.emplace(mSfActiveModes.size(), tmpIt->second);
        } else {
            MESON_LOGD("curMode: %s", mActiveConfigStr.c_str());
        }
    }

    auto it = mHwcActiveModes.find(mHwcActiveConfigId);
    if (it != mHwcActiveModes.end()) {
        mSfActiveModes.emplace(mSfActiveModes.size(), it->second);
    }
    MESON_LOGD("update Sf list with no default mode done, size (%zu)", mSfActiveModes.size());
    mSfActiveConfigId = mSfActiveModes.size() - 1;
    mFakeConfigId = mSfActiveConfigId;
    return HWC2_ERROR_NONE;
}

int32_t  ActiveModeMgr::getDisplayAttribute(
    uint32_t config, int32_t attribute, int32_t * outValue, int32_t caller __unused) {
/*    MESON_LOGV("getDisplayAttribute: config %d, fakeConfig %d,"
        "HwcActiveConfig %d, SfActiveConfig %d, mExtModeSet %d, caller (%s)",
        config, mFakeConfigId, mHwcActiveConfigId, mSfActiveConfigId, mExtModeSet,
        caller == CALL_FROM_SF ? "SF" : "HWC");*/

    std::map<uint32_t, drm_mode_info_t>::iterator it;
    it = mSfActiveModes.find(config);
    if (it != mSfActiveModes.end()) {
        drm_mode_info_t curMode = it->second;
        switch (attribute) {
            case HWC2_ATTRIBUTE_WIDTH:
                *outValue = mDefaultMode.pixelW;
                break;
            case HWC2_ATTRIBUTE_HEIGHT:
                *outValue = mDefaultMode.pixelH;
                break;
            case HWC2_ATTRIBUTE_VSYNC_PERIOD:
    #ifdef HWC_HEADLESS
                *outValue = 1e9 / (HWC_HEADLESS_REFRESHRATE);
    #else
                *outValue = 1e9 / curMode.refreshRate;
    #endif
                break;
            case HWC2_ATTRIBUTE_DPI_X:
                *outValue = curMode.dpiX * 1000.0f;
                break;
            case HWC2_ATTRIBUTE_DPI_Y:
                *outValue = curMode.dpiY * 1000.0f;
                break;
            default:
                MESON_LOGE("Unkown display attribute(%d)", attribute);
                break;
        }
        return HWC2_ERROR_NONE;
   }
   else {
        MESON_LOGE("[%s]: no support display config: %d", __func__, config);
        return HWC2_ERROR_UNSUPPORTED;
   }
}

int32_t  ActiveModeMgr::updateHwcActiveConfig(
    const char * activeMode) {
    MESON_LOGD("ActiveModeMgr::updateHwcActiveConfig (mode)");
    for (auto it = mHwcActiveModes.begin(); it != mHwcActiveModes.end(); ++it) {
        if (strncmp(activeMode, it->second.name, DRM_DISPLAY_MODE_LEN) == 0
                && !isFracRate(it->second.refreshRate)) {
            mDefaultModeSupport = true;
            mHwcActiveConfigId = mHwcActiveModes.size()-1;
            MESON_LOGD("updateActiveConfig to (%s, %d)", activeMode, mHwcActiveConfigId);
            return HWC2_ERROR_NONE;
        }
    }

    mHwcActiveConfigId = mHwcActiveModes.size()-1;
    MESON_LOGD("updateActiveConfig something error to (%s, %d)", activeMode, mHwcActiveConfigId);

    return HWC2_ERROR_NONE;
}


int32_t  ActiveModeMgr::updateSfActiveConfig(
    uint32_t configId, drm_mode_info_t cfg) {
    mActiveConfigStr = cfg.name;
    mSfActiveConfigId = configId;
    mFakeConfigId = mSfActiveModes.size()-1;
    MESON_LOGD("updateSfActiveConfig(mode) to (%s, %d)", cfg.name, mSfActiveConfigId);
	return HWC2_ERROR_NONE;
}

int32_t ActiveModeMgr::getActiveConfig(
    uint32_t * outConfig, int32_t caller __unused) {
    *outConfig = mExtModeSet ? mSfActiveConfigId : mFakeConfigId;
/*    MESON_LOGV("[%s]: ret %d, Sf id %d, Hwc id %d, fake id %d, mExtModeSet %d, caller (%s)",
        __func__ ,*outConfig, mSfActiveConfigId, mHwcActiveConfigId, mFakeConfigId,
        mExtModeSet, caller == CALL_FROM_SF ? "SF" : "HWC");*/
    return HWC2_ERROR_NONE;
}

int32_t ActiveModeMgr::setActiveConfig(
    uint32_t configId) {
    std::map<uint32_t, drm_mode_info_t>::iterator it =
        mSfActiveModes.find(configId);
    MESON_LOGD("ActiveModeMgr::setActiveConfig %d", configId);
    if (it != mSfActiveModes.end()) {
        drm_mode_info_t cfg = it->second;

        // update real active config.
        updateSfActiveConfig(configId, cfg);

        // It is possible that default mode is not supported by the sink
        // and it was only advertised to the FWK to force 1080p UI.
        // Trap this case and do nothing. FWK will keep thinking
        // 1080p is supported and set.
        if (!mDefaultModeSupport
            && strncmp(cfg.name, mDefaultMode.name, DRM_DISPLAY_MODE_LEN) == 0) {
            MESON_LOGD("setActiveConfig default mode not supported");
            return HWC2_ERROR_NONE;
        }

        mExtModeSet = true;
        mCallOnHotPlug = false;
        mConnector->setMode(cfg);
        mCrtc->setMode(cfg);
        MESON_LOGD("setActiveConfig %d, mExtModeSet %d",
            configId, mExtModeSet);
        return HWC2_ERROR_NONE;
    } else {
        MESON_LOGE("set invalild active config (%d)", configId);
        return HWC2_ERROR_NOT_VALIDATED;
    }
}

void ActiveModeMgr::reset() {
    mHwcActiveModes.clear();
    mSfActiveModes.clear();
    mDefaultModeSupport = false;
    mSfActiveConfigId = mHwcActiveConfigId = mFakeConfigId = -1;
    mExtModeSet = false;
    mCallOnHotPlug = true;
}

void ActiveModeMgr::resetTags() {
    if (!useFakeMode) {
        mCallOnHotPlug = true;
        mExtModeSet = false;
    }
};


const drm_mode_info_t ActiveModeMgr::findMatchedMode(
    uint32_t width, uint32_t height, float refreshrate) {
    uint32_t i = 0;
    uint32_t size = ARRAY_SIZE(mode_info);
    for (i = 0; i < size; i++) {
        if (mode_info[i].pixelW == width &&
            mode_info[i].pixelH == height &&
            mode_info[i].refreshRate == refreshrate) {
        return mode_info[i];
        }
    }
    return mode_info[size-1];
}

void ActiveModeMgr::dump(String8 & dumpstr) {
    dumpstr.appendFormat("ActiveModeMgr: %s\n", mActiveConfigStr.c_str());
    dumpstr.append("---------------------------------------------------------"
        "-------------------------------------------------\n");
    dumpstr.append("|   CONFIG   |   VSYNC_PERIOD   |   WIDTH   |   HEIGHT   |"
        "   DPI_X   |   DPI_Y   |   FRAC    |     mode   \n");
    dumpstr.append("+------------+------------------+-----------+------------+"
        "-----------+-----------+-----------+-----------+\n");
    std::map<uint32_t, drm_mode_info_t>::iterator it =
        mHwcActiveModes.begin();
    for (; it != mHwcActiveModes.end(); ++it) {
        int mode = it->first;
        drm_mode_info_t config = it->second;
        dumpstr.appendFormat("%s %2d     |      %.3f      |   %5d   |   %5d    |"
            "    %3d    |    %3d    |   %d    |    %s   \n",
            (mode == (int)mHwcActiveConfigId) ? "*   " : "    ",
            mode,
            config.refreshRate,
            config.pixelW,
            config.pixelH,
            config.dpiX,
            config.dpiY,
            isFracRate(config.refreshRate),
            config.name);
    }
    dumpstr.append("---------------------------------------------------------"
        "-------------------------------------------------\n");

    dumpstr.appendFormat("ActiveModeMgr: %s\n", mActiveConfigStr.c_str());
    dumpstr.append("---------------------------------------------------------"
        "-------------------------------------------------\n");
    dumpstr.append("|   CONFIG   |   VSYNC_PERIOD   |   WIDTH   |   HEIGHT   |"
        "   DPI_X   |   DPI_Y   |   FRAC    |     mode   \n");
    dumpstr.append("+------------+------------------+-----------+------------+"
        "-----------+-----------+-----------+-----------+\n");
    std::map<uint32_t, drm_mode_info_t>::iterator it1 =
        mSfActiveModes.begin();
    for (; it1 != mSfActiveModes.end(); ++it1) {
        int mode1 = it1->first;
        drm_mode_info_t config = it1->second;
        dumpstr.appendFormat("%s %2d     |      %.3f      |   %5d   |   %5d    |"
            "    %3d    |    %3d    |   %d    |    %s   \n",
            (mode1 == (int)mSfActiveConfigId) ? "*   " : "    ",
            mode1,
            config.refreshRate,
            config.pixelW,
            config.pixelH,
            config.dpiX,
            config.dpiY,
            isFracRate(config.refreshRate),
            config.name);
    }
    dumpstr.append("---------------------------------------------------------"
        "-------------------------------------------------\n");
}

