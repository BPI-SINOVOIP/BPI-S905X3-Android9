/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "DualDisplayPipe.h"
#include <HwcConfig.h>
#include <MesonLog.h>
#include <systemcontrol.h>

#define DRM_DISPLAY_MODE_PANEL ("panel")
#define DRM_DISPLAY_MODE_DEFAULT ("1080p60hz")
#define DRM_DISPLAY_ATTR_DEFAULT ("444,8bit")
#define LCD_MUTE   "/sys/class/lcd/test"

DualDisplayPipe::DualDisplayPipe()
    : HwcDisplayPipe() {
        /*Todo:init status need to get from??*/
        mPrimaryConnectorType = DRM_MODE_CONNECTOR_INVALID;
        mExtendConnectorType = DRM_MODE_CONNECTOR_INVALID;
        mHdmi_connected = true;
}

DualDisplayPipe::~DualDisplayPipe() {
}

int32_t DualDisplayPipe::init(
    std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps) {
    HwcDisplayPipe::init(hwcDisps);

    MESON_ASSERT(HwcConfig::getDisplayNum() == 2,
        "DualDisplayPipe need 2 hwc display.");

    /*check dual pipeline display mode both init status*/
    bool init = true;
    for (auto stat : mPipeStats) {
        drm_mode_info_t curMode;
        int32_t ret = stat.second->modeCrtc->getMode(curMode);
        if ((ret < 0 || !strcmp(curMode.name, DRM_DISPLAY_MODE_NULL)) &&
            stat.second->modeConnector->isConnected()) {
            init = false;
        }
    }
    if (init == true)
        return 0;
    /*reinit dual display pipeline display mode*/
    static drm_mode_info_t displayMode = {
        DRM_DISPLAY_MODE_NULL,
        0, 0,
        0, 0,
        60.0
    };
    /*reset dual pipeline displaymode to NULL*/
    for (auto stat : mPipeStats) {
        stat.second->modeCrtc->setMode(displayMode);
    }
    /*set vout displaymode*/
    for (auto stat : mPipeStats) {
        switch (stat.second->cfg.modeConnectorType) {
            case DRM_MODE_CONNECTOR_CVBS:
                {
                    /*ToDo: add cvbs support*/
                }
                break;
            case DRM_MODE_CONNECTOR_HDMI:
                {
                    if (mHdmi_connected == true) {
                        /*get hdmi prefect display mode firstly for init default config*/
                        std::string prefdisplayMode;
                        if (sc_get_pref_display_mode(prefdisplayMode) == false) {
                            strcpy(displayMode.name, DRM_DISPLAY_MODE_DEFAULT);
                            prefdisplayMode = DRM_DISPLAY_MODE_DEFAULT;
                            MESON_LOGI("sc_get_pref_display_mode fail! use default mode");
                        } else {
                            strcpy(displayMode.name, prefdisplayMode.c_str());
                        }
                        sc_set_display_mode(prefdisplayMode);
                    }
                }
                break;
            case DRM_MODE_CONNECTOR_PANEL:
                {
                    strcpy(displayMode.name, DRM_DISPLAY_MODE_PANEL);
                    stat.second->modeCrtc->setMode(displayMode);
                }
                break;
            default:
                MESON_LOGE("Do Nothing in updateDisplayMode .");
                break;
        };
        MESON_LOGI("init set mode (%s)",displayMode.name);
    }
    return 0;
}

int32_t DualDisplayPipe::getPipeCfg(uint32_t hwcid, PipeCfg & cfg) {
    /*get hdmi hpd state firstly for init default config*/
    std::shared_ptr<HwDisplayConnector> hwConnector;
    getConnector(DRM_MODE_CONNECTOR_HDMI, hwConnector);
    hwConnector->update();
    mHdmi_connected = hwConnector->isConnected();
    drm_connector_type_t  connector = getConnetorCfg(hwcid);
    if (hwcid == 0) {
        if (HwcConfig::dynamicSwitchViuEnabled() == true &&
            mHdmi_connected == true &&
            connector == DRM_MODE_CONNECTOR_PANEL)
            cfg.hwcCrtcId = CRTC_VOUT2;
        else
            cfg.hwcCrtcId = CRTC_VOUT1;
        mPrimaryConnectorType = connector;
    } else if (hwcid == 1) {
        if (HwcConfig::dynamicSwitchViuEnabled() == true &&
            mHdmi_connected == true &&
            connector == DRM_MODE_CONNECTOR_HDMI)
            cfg.hwcCrtcId = CRTC_VOUT1;
        else
            cfg.hwcCrtcId = CRTC_VOUT2;
        mExtendConnectorType = connector;
    }
    MESON_LOGD("dual pipe line getpipecfg hwcid=%d,connector=%d,crtcid=%d.",
        hwcid, connector, cfg.hwcCrtcId);
    cfg.hwcPostprocessorType = INVALID_POST_PROCESSOR;
    cfg.modeCrtcId = cfg.hwcCrtcId;
    if (HwcConfig::dynamicSwitchConnectorEnabled() == true &&
        mPrimaryConnectorType == DRM_MODE_CONNECTOR_HDMI &&
        mExtendConnectorType != DRM_MODE_CONNECTOR_INVALID) {
        if (hwcid == 0) {
            if (mHdmi_connected == true)
                cfg.modeConnectorType = cfg.hwcConnectorType = mPrimaryConnectorType;
            else
                cfg.modeConnectorType = cfg.hwcConnectorType = mExtendConnectorType;
        }
        if (hwcid == 1) {
            if (mHdmi_connected == true)
                cfg.modeConnectorType = cfg.hwcConnectorType = mExtendConnectorType;
            else
                cfg.modeConnectorType = cfg.hwcConnectorType = mPrimaryConnectorType;
        }
    } else {
        cfg.modeConnectorType = cfg.hwcConnectorType = connector;
    }
    return 0;
}

void DualDisplayPipe::handleEvent(drm_display_event event, int val) {
    if (event == DRM_EVENT_HDMITX_HOTPLUG &&
        (HwcConfig::dynamicSwitchViuEnabled() == true ||
        HwcConfig::dynamicSwitchConnectorEnabled() == true)) {
        std::lock_guard<std::mutex> lock(mMutex);
        MESON_LOGD("Hotplug handle value %d.",val);
        bool connected = (val == 0) ? false : true;
        mHdmi_connected = connected;
        static drm_mode_info_t displayMode = {
            DRM_DISPLAY_MODE_NULL,
            0, 0,
            0, 0,
            60.0
        };
        if (mPrimaryConnectorType != DRM_MODE_CONNECTOR_INVALID &&
            mExtendConnectorType != DRM_MODE_CONNECTOR_INVALID) {
            for (auto statIt : mPipeStats) {
                PipeCfg cfg;
                getPipeCfg(statIt.second->hwcId, cfg);
                if ((statIt.second->cfg.modeConnectorType != cfg.modeConnectorType) ||
                        (statIt.second->cfg.hwcCrtcId != cfg.hwcCrtcId)) {
                    /* If display pip stat resource change, then
                     * reset vout displaymode, for we need do pipeline switch*/
                    statIt.second->hwcCrtc->unbind();
                }
            }
            /*update display pipe.*/
            for (auto statIt : mPipeStats) {
                updatePipe(statIt.second);
            }
            /*update display mode*/
            for (auto statIt : mPipeStats) {
                if (statIt.second->modeConnector->getType() == DRM_MODE_CONNECTOR_HDMI) {
                    std::string displayattr(DRM_DISPLAY_ATTR_DEFAULT);
                    std::string prefdisplayMode;
                    if (connected == false) {
                        strcpy(displayMode.name, DRM_DISPLAY_MODE_NULL);
                        prefdisplayMode = DRM_DISPLAY_MODE_NULL;
                    } else {
                        /*get hdmi prefect display mode*/
                        if (sc_get_pref_display_mode(prefdisplayMode) == false) {
                            strcpy(displayMode.name, DRM_DISPLAY_MODE_DEFAULT);
                            prefdisplayMode = DRM_DISPLAY_MODE_DEFAULT;
                            MESON_LOGI("sc_get_pref_display_mode fail! use default mode");
                        } else {
                            strcpy(displayMode.name, prefdisplayMode.c_str());
                        }
                    }
                    sc_set_display_mode(prefdisplayMode);
                } else if (statIt.second->modeConnector->getType() == DRM_MODE_CONNECTOR_PANEL) {
                    std::string lcd_mute("8");
                    sc_write_sysfs(LCD_MUTE, lcd_mute);
                    MESON_LOGD("DualDisplayPipe::handleEvent lcd mute");
                    strcpy(displayMode.name, DRM_DISPLAY_MODE_PANEL);
                    statIt.second->modeCrtc->setMode(displayMode);
                    /*mode change config move here for dual display panle*/
                    statIt.second->modeCrtc->loadProperities();
                    statIt.second->modeCrtc->update();
                    statIt.second->modeMgr->update();
                    statIt.second->hwcDisplay->onModeChanged(val);
                    /*update display dynamic info.*/
                    drm_mode_info_t mode;
                    if (HwcConfig::softwareVsyncEnabled()) {
                        if (0 == statIt.second->modeMgr->getDisplayMode(mode)) {
                            statIt.second->hwcVsync->setPeriod(1e9 / mode.refreshRate);
                        }
                    }
                    if (mode.refreshRate)
                        usleep(1000000 / mode.refreshRate);
                    else
                        usleep(20000);
                    std::string lcd_unmute("0");
                    sc_write_sysfs(LCD_MUTE, lcd_unmute);
                    MESON_LOGD("HwcDisplayPipe::handleEvent lcd unmute");
                } else {
                    strcpy(displayMode.name, DRM_DISPLAY_MODE_NULL);
                    statIt.second->modeCrtc->setMode(displayMode);
                }
                MESON_LOGI("DualDisplayPipe::handleEvent set mode (%s)",displayMode.name);
                statIt.second->modeMgr->setDisplayResources(statIt.second->modeCrtc, statIt.second->modeConnector);
                statIt.second->modeMgr->update();
            }
        }
        for (auto statIt : mPipeStats) {
            if (statIt.second->modeConnector->getType() == DRM_MODE_CONNECTOR_HDMI) {
                statIt.second->modeConnector->update();
                statIt.second->hwcDisplay->onHotplug(connected);
            }
        }
    } else {
        MESON_LOGI("Receive DualDisplayPipe unhandled event %d", event);
        HwcDisplayPipe::handleEvent(event, val);
    }
}
