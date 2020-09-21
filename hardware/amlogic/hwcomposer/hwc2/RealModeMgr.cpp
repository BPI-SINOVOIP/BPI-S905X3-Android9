/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <MesonLog.h>
#include <HwcConfig.h>
#include <hardware/hwcomposer2.h>

#include "RealModeMgr.h"

#define DEFUALT_DPI (159)
#define DEFAULT_REFRESH_RATE (60.0f)

static const drm_mode_info_t defaultMode = {
    .name              = "null",
    .dpiX              = DEFUALT_DPI,
    .dpiY              = DEFUALT_DPI,
    .pixelW            = 1920,
    .pixelH            = 1080,
    .refreshRate       = DEFAULT_REFRESH_RATE,
};

RealModeMgr::RealModeMgr() {
}

RealModeMgr::~RealModeMgr() {
}

hwc_modes_policy_t RealModeMgr::getPolicyType() {
    return REAL_MODE_POLICY;
}

const char * RealModeMgr::getName() {
    return "RealModeMgr";
}

void RealModeMgr::setFramebufferSize(uint32_t w, uint32_t h) {
    mHwcFbWidth = w;
    mHwcFbHeight =h;
}

void RealModeMgr::setDisplayResources(
    std::shared_ptr<HwDisplayCrtc> & crtc,
    std::shared_ptr<HwDisplayConnector> & connector) {
    mConnector = connector;
    mCrtc = crtc;
}

int32_t RealModeMgr::updateActiveConfig(const char* activeMode) {
    for (auto it = mModes.begin(); it != mModes.end(); ++it) {
        if (strncmp(activeMode, it->second.name, DRM_DISPLAY_MODE_LEN) == 0) {
            mActiveConfigId = it->first;
            MESON_LOGV("updateActiveConfig aciveConfigId = %d", mActiveConfigId);
            return HWC2_ERROR_NONE;
        }
    }

    mActiveConfigId = mModes.size()-1;
    MESON_LOGD("%s something error to (%s, %d)", __func__, activeMode, mActiveConfigId);

    return HWC2_ERROR_NONE;
}

void RealModeMgr::reset() {
    mModes.clear();
    mActiveConfigId = -1;
}

int32_t RealModeMgr::update() {
    bool useFakeMode = true;
    drm_mode_info_t realMode;
    std::map<uint32_t, drm_mode_info_t> supportModes;

    if (mConnector->isConnected()) {
        std::lock_guard<std::mutex> lock(mMutex);
        mConnector->getModes(supportModes);
        if (mCrtc->getMode(realMode) == 0) {
            if (realMode.name[0] != 0) {
                mCurMode = realMode;
                useFakeMode = false;
            }
        }
        if (useFakeMode) {
            mCurMode = defaultMode;
        }

#ifdef HWC_SUPPORT_MODES_LIST
        reset();
        for (auto it = supportModes.begin(); it != supportModes.end(); it++) {
            if (!strncmp(mCurMode.name, it->second.name, DRM_DISPLAY_MODE_LEN)
                && mCurMode.refreshRate == it->second.refreshRate) {
                if (useFakeMode) {
                    mCurMode.dpiX = it->second.dpiX;
                    mCurMode.dpiY = it->second.dpiY;
                }
                mModes.emplace(mModes.size(), mCurMode);
            } else {
                mModes.emplace(mModes.size(), it->second);
            }
        }

        if (useFakeMode) {
            mModes.emplace(mModes.size(), mCurMode);
        }
        updateActiveConfig(mCurMode.name);
#endif
    }

#ifndef HWC_SUPPORT_MODES_LIST
    if (useFakeMode)
            mCurMode = defaultMode;
#endif

    return HWC2_ERROR_NONE;
}

int32_t RealModeMgr::getDisplayMode(drm_mode_info_t & mode) {
    return mCrtc->getMode(mode);
}

int32_t  RealModeMgr::getDisplayConfigs(
    uint32_t * outNumConfigs, uint32_t * outConfigs) {
    std::lock_guard<std::mutex> lock(mMutex);
#ifdef HWC_SUPPORT_MODES_LIST
    *outNumConfigs = mModes.size();

    if (outConfigs) {
        std::map<uint32_t, drm_mode_info_t>::iterator it =
            mModes.begin();
        for (uint32_t index = 0; it != mModes.end(); ++it, ++index) {
            outConfigs[index] = it->first;
            MESON_LOGV("realmode getDisplayConfigs outConfig[%d]: %d %s.", index, outConfigs[index], it->second.name);
        }
    }
#else
    *outNumConfigs = 1;
    if (outConfigs)
        *outConfigs = 0;
#endif
    return HWC2_ERROR_NONE;
}

int32_t  RealModeMgr::getDisplayAttribute(
    uint32_t config, int32_t attribute, int32_t * outValue,
    int32_t caller __unused) {
    std::lock_guard<std::mutex> lock(mMutex);
#ifdef HWC_SUPPORT_MODES_LIST
    std::map<uint32_t, drm_mode_info_t>::iterator it;
    it = mModes.find(config);

    if (it != mModes.end()) {
        drm_mode_info_t curMode = it->second;
#else
        UNUSED(config);
        drm_mode_info_t curMode = mCurMode;
#endif
        switch (attribute) {
            case HWC2_ATTRIBUTE_WIDTH:
                *outValue = curMode.pixelW > mHwcFbWidth ? mHwcFbWidth : curMode.pixelW;
                break;
            case HWC2_ATTRIBUTE_HEIGHT:
                *outValue = curMode.pixelH > mHwcFbHeight ? mHwcFbHeight : curMode.pixelH;
                break;
            case HWC2_ATTRIBUTE_VSYNC_PERIOD:
                if (HwcConfig::isHeadlessMode()) {
                    *outValue = 1e9 / HwcConfig::headlessRefreshRate();
                } else {
                    *outValue = 1e9 / curMode.refreshRate;
                }
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
#ifdef HWC_SUPPORT_MODES_LIST
    } else {
        MESON_LOGE("[%s]: no support display config: %d", __func__, config);
        return HWC2_ERROR_UNSUPPORTED;
    }
#endif

    return HWC2_ERROR_NONE;
}

int32_t RealModeMgr::getActiveConfig(uint32_t * outConfig, int32_t caller __unused) {
    std::lock_guard<std::mutex> lock(mMutex);
#ifdef HWC_SUPPORT_MODES_LIST
    *outConfig = mActiveConfigId;
#else
    *outConfig = 0;
#endif
    MESON_LOGV("%s ActiveConfigid=%d", __func__, *outConfig);

    return HWC2_ERROR_NONE;
}

int32_t RealModeMgr::setActiveConfig(uint32_t config) {
    std::lock_guard<std::mutex> lock(mMutex);
#ifdef HWC_SUPPORT_MODES_LIST
    std::map<uint32_t, drm_mode_info_t>::iterator it =
        mModes.find(config);

    if (it != mModes.end()) {
        drm_mode_info_t cfg = it->second;

        updateActiveConfig(cfg.name);
        if (strncmp(cfg.name, defaultMode.name, DRM_DISPLAY_MODE_LEN) == 0) {
            MESON_LOGD("setActiveConfig default mode not supported");
            return HWC2_ERROR_NONE;
        }

        mCrtc->setMode(cfg);
    } else {
        MESON_LOGE("set invalild active config (%d)", config);
        return HWC2_ERROR_NOT_VALIDATED;
    }
#else
    if (config > 0)
        MESON_LOGE("RealModeMgr not support config (%d)", config);
#endif

    return HWC2_ERROR_NONE;
}

void RealModeMgr::dump(String8 & dumpstr) {
    dumpstr.appendFormat("RealModeMgr:(%s)\n", mCurMode.name);
    dumpstr.append("---------------------------------------------------------"
        "-------------------------------------\n");
    dumpstr.append("|  CONFIG   |   VSYNC_PERIOD   |   WIDTH   |   HEIGHT   |"
        "   DPI_X   |   DPI_Y   |    NAME    |\n");
    dumpstr.append("+------------+------------------+-----------+------------+"
        "-----------+-----------+-----------+\n");

#ifdef HWC_SUPPORT_MODES_LIST
    std::map<uint32_t, drm_mode_info_t>::iterator it =
        mModes.begin();

    for (; it != mModes.end(); ++it) {
        int mode = it->first;
        drm_mode_info_t config = it->second;
#else
        int mode = 0;
        mActiveConfigId = 0;
        drm_mode_info_t config = mCurMode;
#endif
        dumpstr.appendFormat("%s %2d     |      %.3f      |   %5d   |   %5d    |"
            "    %3d    |    %3d    | %10s |\n",
            (mode == (int)mActiveConfigId) ? "*   " : "    ",
            mode,
            config.refreshRate,
            config.pixelW,
            config.pixelH,
            config.dpiX,
            config.dpiY,
            config.name);
#ifdef HWC_SUPPORT_MODES_LIST
    }
#endif
    dumpstr.append("---------------------------------------------------------"
        "-------------------------------------\n");
}
