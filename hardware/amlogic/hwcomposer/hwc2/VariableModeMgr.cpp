/*
 * Copyright (c) 2018 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "VariableModeMgr.h"

#include <HwcConfig.h>
#include <MesonLog.h>
#include <systemcontrol.h>
#include <hardware/hwcomposer2.h>

#include <string>

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

VariableModeMgr::VariableModeMgr()
    : mIsInit(true) {
}

VariableModeMgr::~VariableModeMgr() {

}

hwc_modes_policy_t VariableModeMgr::getPolicyType() {
    return FULL_ACTIVE_POLICY;
}

const char * VariableModeMgr::getName() {
    return "VariableMode";
}

void VariableModeMgr::setFramebufferSize(uint32_t w, uint32_t h) {
    mFbWidth = w;
    mFbHeight = h;
}

void VariableModeMgr::setDisplayResources(
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

    update();
}

int32_t VariableModeMgr::initDefaultDispResources() {
    mDefaultMode =
        findMatchedMode(mFbWidth, mFbHeight, DEFAULT_REFRESH_RATE_60);
    mHwcActiveModes.emplace(mHwcActiveModes.size(), mDefaultMode);
    updateHwcActiveConfig(mDefaultMode.name);
    MESON_LOGV("initDefaultDispResources (%s)", mDefaultMode.name);
    return 0;
}

int32_t VariableModeMgr::update() {
    MESON_LOG_FUN_ENTER();
    bool useFakeMode = false;

    if (mConnector->isConnected()) {
        updateHwcDispConfigs();
        drm_mode_info_t dispmode;
        if (getDisplayMode(dispmode) == 0) {
            updateHwcActiveConfig(dispmode.name);
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

int32_t VariableModeMgr::getDisplayMode(drm_mode_info_t & mode) {
    return mCrtc->getMode(mode);
}

int32_t VariableModeMgr::getDisplayConfigs(
    uint32_t * outNumConfigs, uint32_t * outConfigs) {
    *outNumConfigs = mHwcActiveModes.size();

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

int32_t VariableModeMgr::updateHwcDispConfigs() {
    std::map<uint32_t, drm_mode_info_t> activeModes;
    mHwcActiveModes.clear();

    mConnector->getModes(activeModes);
    for (auto it = activeModes.begin(); it != activeModes.end(); ++it) {
        // skip default / fake active mode as we add it to the end
        if (!strncmp(mDefaultMode.name, it->second.name, DRM_DISPLAY_MODE_LEN)
            && mDefaultMode.refreshRate == it->second.refreshRate) {
            mDefaultModeSupport = true;
            mDefaultMode.dpiX = it->second.dpiX;
            mDefaultMode.dpiY = it->second.dpiY;
        } else {
            MESON_LOGV("[%s]: Hwc modes %d.", __func__, mHwcActiveModes.size());
            mHwcActiveModes.emplace(mHwcActiveModes.size(), it->second);
        }
    }

    // Add default mode as last, unconditionally in all cases. This is to ensure
    // availability of 1080p mode always.
    MESON_LOGV("[%s]: Hwc modes %d.", __func__, mHwcActiveModes.size());
    mHwcActiveModes.emplace(mHwcActiveModes.size(), mDefaultMode);
    return HWC2_ERROR_NONE;
}

int32_t VariableModeMgr::updateSfDispConfigs() {
    // clear display modes
    mSfActiveModes.clear();

    mSfActiveModes = mHwcActiveModes;

    // Set active config id used by SF.
    mSfActiveConfigId = mHwcActiveConfigId;

    return HWC2_ERROR_NONE;
}

int32_t  VariableModeMgr::getDisplayAttribute(
    uint32_t config, int32_t attribute, int32_t * outValue, int32_t caller) {
    MESON_LOGV("getDisplayAttribute: config %d, fakeConfig %d,"
        "HwcActiveConfig %d, SfActiveConfig %d, mExtModeSet %d, caller (%s)",
        config, mFakeConfigId, mHwcActiveConfigId, mSfActiveConfigId, mExtModeSet,
        caller == CALL_FROM_SF ? "SF" : "HWC");

    std::map<uint32_t, drm_mode_info_t>::iterator it;
    if (CALL_FROM_SF == caller) {
        it = mSfActiveModes.find(config);
    } else if (CALL_FROM_HWC == caller) {
        it = mHwcActiveModes.find(config);
    }

    if (it != mHwcActiveModes.end() || it != mSfActiveModes.end()) {
        drm_mode_info_t curMode = it->second;

        switch (attribute) {
            case HWC2_ATTRIBUTE_WIDTH:
                *outValue = curMode.pixelW;
                break;
            case HWC2_ATTRIBUTE_HEIGHT:
                *outValue = curMode.pixelH;
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
    } else {
        MESON_LOGE("[%s]: no support display config: %d", __func__, config);
        return HWC2_ERROR_UNSUPPORTED;
    }
}

int32_t  VariableModeMgr::updateHwcActiveConfig(
    const char * activeMode) {
    mActiveConfigStr = activeMode;

    for (auto it = mHwcActiveModes.begin(); it != mHwcActiveModes.end(); ++it) {
        if (strncmp(activeMode, it->second.name, DRM_DISPLAY_MODE_LEN) == 0) {
            mHwcActiveConfigId = it->first;
            mFakeConfigId = mHwcActiveModes.size()-1;
            MESON_LOGD("updateActiveConfig to (%s, %d)", activeMode, mHwcActiveConfigId);
            return HWC2_ERROR_NONE;
        }
    }

    // If we reach here we are trying to set an unsupported mode. This can happen as
    // SystemControl does not guarantee to keep the EDID mode list and the active
    // mode id synchronised. We therefore handle the case where the active mode is
    // not supported by ensuring something sane is set instead.
    // NOTE: this is only really a workaround - HWC should instead guarantee that
    // the display mode list and active mode reported to SF are kept in sync with
    // hot plug events.
    mHwcActiveConfigId = mHwcActiveModes.size()-1;
    mFakeConfigId = mHwcActiveConfigId;
    MESON_LOGD("updateActiveConfig something error to (%s, %d)", activeMode, mHwcActiveConfigId);

    return HWC2_ERROR_NONE;
}

int32_t VariableModeMgr::getActiveConfig(
    uint32_t * outConfig, int32_t caller) {
    if (CALL_FROM_SF == caller) {
        *outConfig = mExtModeSet ? mSfActiveConfigId : mFakeConfigId;
    } else if (CALL_FROM_HWC == caller) {
        *outConfig = mExtModeSet ? mHwcActiveConfigId : mFakeConfigId;
    }
    MESON_LOGV("[%s]: ret %d, Sf id %d, Hwc id %d, fake id %d, mExtModeSet %d, caller (%s)",
        __func__ ,*outConfig, mSfActiveConfigId, mHwcActiveConfigId, mFakeConfigId,
        mExtModeSet, caller == CALL_FROM_SF ? "SF" : "HWC");
    return HWC2_ERROR_NONE;
}

int32_t VariableModeMgr::setActiveConfig(
    uint32_t config) {
    std::map<uint32_t, drm_mode_info_t>::iterator it =
        mSfActiveModes.find(config);
    if (it != mSfActiveModes.end()) {
        drm_mode_info_t cfg = it->second;

        // update real active config.
        updateHwcActiveConfig(cfg.name);

        // since SF is asking to set the mode, we should update mSFActiveConfigId
        // as well here so that when SF calls getActiveConfig, we return correct
        // mode. Without it, mSFActiveConfigId is getting updated only when
        // updateSfDispConfigs is called by getDisplayConfigs. But getDisplayConfigs
        // does not get called when user manually selects a mode and mSFActiveConfigId
        // stays stale.
        mSfActiveConfigId = mHwcActiveConfigId;

        // It is possible that default mode is not supported by the sink
        // and it was only advertised to the FWK to force 1080p UI.
        // Trap this case and do nothing. FWK will keep thinking
        // 1080p is supported and set.
        if (!mDefaultModeSupport
            && strncmp(cfg.name, mDefaultMode.name, DRM_DISPLAY_MODE_LEN) == 0) {
            MESON_LOGD("setActiveConfig default mode not supported");
            return HWC2_ERROR_NONE;
        }

        mCrtc->setMode(cfg);
        mExtModeSet = true;
        MESON_LOGD("setActiveConfig %d, mExtModeSet %d",
            config, mExtModeSet);
        return HWC2_ERROR_NONE;
    } else {
        MESON_LOGE("set invalild active config (%d)", config);
        return HWC2_ERROR_NOT_VALIDATED;
    }
}

void VariableModeMgr::reset() {
    mHwcActiveModes.clear();
    mSfActiveModes.clear();
    mDefaultModeSupport = false;
    mSfActiveConfigId = mHwcActiveConfigId = mFakeConfigId = -1;
    mExtModeSet = false;
}

const drm_mode_info_t VariableModeMgr::findMatchedMode(
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

void VariableModeMgr::dump(String8 & dumpstr) {
    dumpstr.appendFormat("VariableModeMgr: %s\n", mActiveConfigStr.c_str());
    dumpstr.append("---------------------------------------------------------"
        "-------------------------\n");
    dumpstr.append("|   CONFIG   |   VSYNC_PERIOD   |   WIDTH   |   HEIGHT   |"
        "   DPI_X   |   DPI_Y   |\n");
    dumpstr.append("+------------+------------------+-----------+------------+"
        "-----------+-----------+\n");
    std::map<uint32_t, drm_mode_info_t>::iterator it =
        mHwcActiveModes.begin();
    for (; it != mHwcActiveModes.end(); ++it) {
        int mode = it->first;
        drm_mode_info_t config = it->second;
        dumpstr.appendFormat("%s %2d     |      %.3f      |   %5d   |   %5d    |"
            "    %3d    |    %3d    \n",
            (mode == (int)mHwcActiveConfigId) ? "*   " : "    ",
            mode,
            config.refreshRate,
            config.pixelW,
            config.pixelH,
            config.dpiX,
            config.dpiY);
    }
    dumpstr.append("---------------------------------------------------------"
        "-------------------------\n");
}

