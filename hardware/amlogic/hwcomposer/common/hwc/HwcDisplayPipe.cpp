/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "HwcDisplayPipe.h"
#include "FixedDisplayPipe.h"
#include "LoopbackDisplayPipe.h"
#include "DualDisplayPipe.h"
#include <HwcConfig.h>
#include <systemcontrol.h>
#include <misc.h>

#include <HwDisplayManager.h>

#define HWC_BOOTED_PROP "vendor.sys.hwc.booted"

HwcDisplayPipe::PipeStat::PipeStat(uint32_t id) {
    hwcId = id;
    cfg.hwcCrtcId = cfg.modeCrtcId = 0;
    cfg.hwcConnectorType = cfg.modeConnectorType = DRM_MODE_CONNECTOR_INVALID;
    cfg.hwcPostprocessorType = INVALID_POST_PROCESSOR;
}

HwcDisplayPipe::PipeStat::~PipeStat() {
    hwcDisplay.reset();
    hwcCrtc.reset();
    hwcConnector.reset();
    hwcPlanes.clear();

    hwcPostProcessor.reset();
    modeMgr.reset();
    modeCrtc.reset();
    modeConnector.reset();
}

HwcDisplayPipe::HwcDisplayPipe() {
    /*load display resources.*/
    HwDisplayManager::getInstance().getCrtcs(mCrtcs);
    HwDisplayManager::getInstance().getPlanes(mPlanes);
}

HwcDisplayPipe::~HwcDisplayPipe() {
}

int32_t HwcDisplayPipe::init(std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps) {
    std::lock_guard<std::mutex> lock(mMutex);
    HwDisplayEventListener::getInstance().registerHandler(
        DRM_EVENT_ALL, (HwDisplayEventHandler*)this);

    for (auto dispIt = hwcDisps.begin(); dispIt != hwcDisps.end(); dispIt++) {
        uint32_t hwcId = dispIt->first;
        std::shared_ptr<PipeStat> stat = std::make_shared<PipeStat>(hwcId);
        mPipeStats.emplace(hwcId, stat);

        /*set HwcDisplay*/
        stat->hwcDisplay = dispIt->second;
        /*set modeMgr*/
        uint32_t fbW = 0, fbH = 0;
#if defined(ODROID)
        std::string mode;
        sc_get_display_mode(mode);
        MESON_LOGE("Get hdmimode(%s) from systemcontrol service", mode.c_str());
        int calibrateCoordinates[4];
        std::string dispModeStr(mode);
        if (0 == sc_get_osd_position(dispModeStr, calibrateCoordinates)) {
            fbW = calibrateCoordinates[2];
            fbH = calibrateCoordinates[3];
        }
        /* limit fb size to 1920x1080 in case of higher resolution than 2560x1080 */
        if ((fbW >= 2560) && (fbH > 1080)) {
            fbW = 1920;
            fbH = 1080;
	}
#else
        HwcConfig::getFramebufferSize (hwcId, fbW, fbH);
#endif
        std::shared_ptr<HwcModeMgr> modeMgr =
        createModeMgr(HwcConfig::getModePolicy(hwcId));
        modeMgr->setFramebufferSize(fbW, fbH);
        stat->modeMgr = modeMgr;
        /*create vsync.*/
        stat->hwcVsync = std::make_shared<HwcVsync>();
        /*init display pipe.*/
        updatePipe(stat);

        /* in case of composer servce restart */
        if (sys_get_bool_prop(HWC_BOOTED_PROP, false)) {
            MESON_LOGD("composer service has restarted, need blank display");
            stat->hwcDisplay->blankDisplay();
        }

        sc_set_property(HWC_BOOTED_PROP, "true");
    }

    return 0;
}

int32_t HwcDisplayPipe::getCrtc(
    int32_t crtcid, std::shared_ptr<HwDisplayCrtc> & crtc) {
    for (auto crtcIt = mCrtcs.begin(); crtcIt != mCrtcs.end(); ++ crtcIt) {
        if ((*crtcIt)->getId() == crtcid) {
            crtc = *crtcIt;
            break;
        }
    }
    MESON_ASSERT(crtc != NULL, "get crtc %d failed.", crtcid);

    return 0;
}

int32_t HwcDisplayPipe::getPlanes(
    int32_t crtcid, std::vector<std::shared_ptr<HwDisplayPlane>> & planes) {
    planes.clear();
    for (auto planeIt = mPlanes.begin(); planeIt != mPlanes.end(); ++ planeIt) {
        std::shared_ptr<HwDisplayPlane> plane = *planeIt;
        if (plane->getPossibleCrtcs() & crtcid) {
            if ((plane->getPlaneType() == CURSOR_PLANE) &&
                HwcConfig::cursorPlaneDisabled()) {
                continue;
            }
            planes.push_back(plane);
        }
    }
    MESON_ASSERT(planes.size() > 0, "get planes for crtc %d failed.", crtcid);
    return 0;
}

int32_t HwcDisplayPipe::getConnector(
    drm_connector_type_t type, std::shared_ptr<HwDisplayConnector> & connector) {
    auto it = mConnectors.find(type);
    if (it != mConnectors.end()) {
        connector = it->second;
    } else {
        HwDisplayManager::getInstance().getConnector(connector, type);
        mConnectors.emplace(type, connector);
        /*TODO: init current status, for we may need it later.*/
        connector->update();
    }

    return 0;
}

int32_t HwcDisplayPipe::getPostProcessor(
    hwc_post_processor_t type, std::shared_ptr<HwcPostProcessor> & processor) {
    UNUSED(type);
    UNUSED(processor);
    processor = NULL;
    return 0;
}

drm_connector_type_t HwcDisplayPipe::getConnetorCfg(uint32_t hwcid) {
    drm_connector_type_t  connector = DRM_MODE_CONNECTOR_INVALID;
    switch (HwcConfig::getConnectorType(hwcid)) {
        case HWC_PANEL_ONLY:
            connector = DRM_MODE_CONNECTOR_PANEL;
            break;
        case HWC_HDMI_ONLY:
            connector = DRM_MODE_CONNECTOR_HDMI;
            break;
        case HWC_CVBS_ONLY:
            connector = DRM_MODE_CONNECTOR_CVBS;
            break;
        case HWC_HDMI_CVBS:
            {
                std::shared_ptr<HwDisplayConnector> hwConnector;
                getConnector(DRM_MODE_CONNECTOR_HDMI, hwConnector);
                if (hwConnector->isConnected()) {
                    connector = DRM_MODE_CONNECTOR_HDMI;
                } else {
                    connector = DRM_MODE_CONNECTOR_CVBS;
                }
            }
            break;
        default:
            MESON_ASSERT(0, "Unknow connector config");
            break;
    }

    return connector;
}

int32_t HwcDisplayPipe::updatePipe(std::shared_ptr<PipeStat> & stat) {
    MESON_LOGD("HwcDisplayPipe::updatePipe.");

    PipeCfg cfg;
    getPipeCfg(stat->hwcId, cfg);

    if (memcmp((const void *)&cfg, (const void *)&(stat->cfg), sizeof(PipeCfg)) == 0) {
            MESON_LOGD("Config is not updated.");
            return 0;
    }

    /*update pipestats*/
    bool resChanged = false;
    if (cfg.hwcCrtcId != stat->cfg.hwcCrtcId) {
        getCrtc(cfg.hwcCrtcId, stat->hwcCrtc);
        getPlanes(cfg.hwcCrtcId, stat->hwcPlanes);
        stat->cfg.hwcCrtcId = cfg.hwcCrtcId;
        resChanged = true;
    }
    if (cfg.hwcConnectorType != stat->cfg.hwcConnectorType) {
        getConnector(cfg.hwcConnectorType, stat->hwcConnector);
        stat->cfg.hwcConnectorType = cfg.hwcConnectorType;
        resChanged = true;
    }
    if (cfg.hwcPostprocessorType != stat->cfg.hwcPostprocessorType) {
        getPostProcessor(cfg.hwcPostprocessorType, stat->hwcPostProcessor);
        stat->cfg.hwcPostprocessorType = cfg.hwcPostprocessorType;
        resChanged = true;
    }
    if (cfg.modeCrtcId != stat->cfg.modeCrtcId) {
        getCrtc(cfg.modeCrtcId, stat->modeCrtc);
        stat->cfg.modeCrtcId = cfg.modeCrtcId;
        resChanged = true;
    }
    if (cfg.modeConnectorType != stat->cfg.modeConnectorType) {
        getConnector(cfg.modeConnectorType, stat->modeConnector);
        stat->cfg.modeConnectorType = cfg.modeConnectorType;
        resChanged = true;
    }

    if (resChanged) {
        MESON_LOGD("HwcDisplayPipe::updatePipe %d changed", stat->hwcId);
        stat->hwcCrtc->bind(stat->hwcConnector, stat->hwcPlanes);
        stat->hwcCrtc->loadProperities();
        stat->hwcCrtc->update();

        if (cfg.modeCrtcId != cfg.hwcCrtcId) {
            std::vector<std::shared_ptr<HwDisplayPlane>> planes;
            getPlanes (cfg.modeCrtcId,  planes);
            stat->modeCrtc->bind(stat->modeConnector, planes);
            stat->modeCrtc->loadProperities();
            stat->modeCrtc->update();
        }

        stat->modeMgr->setDisplayResources(stat->modeCrtc, stat->modeConnector);
        stat->modeMgr->update();

        if (HwcConfig::softwareVsyncEnabled()) {
            stat->hwcVsync->setSoftwareMode();
        } else {
            stat->hwcVsync->setHwMode(stat->modeCrtc);
        }

        stat->hwcDisplay->setVsync(stat->hwcVsync);
        stat->hwcDisplay->setModeMgr(stat->modeMgr);
        stat->hwcDisplay->setDisplayResource(
            stat->hwcCrtc, stat->hwcConnector, stat->hwcPlanes);

        stat->hwcDisplay->setPostProcessor(stat->hwcPostProcessor);
    }

    return 0;
}

int32_t HwcDisplayPipe::handleRequest(uint32_t flags) {
    UNUSED(flags);
    return 0;
}

void HwcDisplayPipe::handleEvent(drm_display_event event, int val) {
    std::lock_guard<std::mutex> lock(mMutex);
    switch (event) {
        case DRM_EVENT_HDMITX_HDCP:
            {
                MESON_LOGD("Hdcp handle value %d.", val);
                for (auto statIt : mPipeStats) {
                    if (statIt.second->modeConnector->getType() == DRM_MODE_CONNECTOR_HDMI) {
                        statIt.second->modeCrtc->update();
                        statIt.second->hwcDisplay->onUpdate((val == 0) ? false : true);
                    }
                }
            }
            break;
        case DRM_EVENT_HDMITX_HOTPLUG:
            {
                MESON_LOGD("Hotplug handle value %d.",val);
                bool connected = (val == 0) ? false : true;
                for (auto statIt : mPipeStats) {
                    if (statIt.second->modeConnector->getType() == DRM_MODE_CONNECTOR_HDMI) {
                        statIt.second->modeConnector->update();
                        statIt.second->hwcDisplay->onHotplug(connected);
                    }
                }
            }
            break;
        /*VIU1 mode changed.*/
        case DRM_EVENT_VOUT1_MODE_CHANGED:
        case DRM_EVENT_VOUT2_MODE_CHANGED:
            {
                MESON_LOGD("ModeChange state: [%s]", val == 1 ? "Complete" : "Begin to change");
                if (val == 1) {
                    int crtcid = CRTC_VOUT1;
                    if (event == DRM_EVENT_VOUT2_MODE_CHANGED)
                        crtcid = CRTC_VOUT2;
                    for (auto statIt : mPipeStats) {
                        if (statIt.second->modeCrtc->getId() == crtcid) {
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
                        }
                    }
                }
            }
            break;

        default:
            MESON_LOGE("Receive unhandled event %d", event);
            break;
    }
}

int32_t HwcDisplayPipe::initDisplayMode(std::shared_ptr<PipeStat> & stat) {
    switch (stat->cfg.modeConnectorType) {
        case DRM_MODE_CONNECTOR_CVBS:
            {
                const char * cvbs_config_key = "ubootenv.var.cvbsmode";
                std::string modeName;
                if (0 == sc_read_bootenv(cvbs_config_key, modeName)) {
                    stat->modeCrtc->writeCurDisplayMode(modeName);
                }
            }
            break;
        case DRM_MODE_CONNECTOR_HDMI:
            {
                /*TODO:*/
            }
            break;
        case DRM_MODE_CONNECTOR_PANEL:
            {
                /*TODO*/
            }
            break;
        default:
            MESON_LOGE("Do Nothing in updateDisplayMode .");
            break;
    };

    return 0;
}

std::shared_ptr<HwcDisplayPipe> createDisplayPipe(hwc_pipe_policy_t pipet) {
    switch (pipet) {
        case HWC_PIPE_DEFAULT:
            return std::make_shared<FixedDisplayPipe>();
        case HWC_PIPE_DUAL:
            return std::make_shared<DualDisplayPipe>();
        case HWC_PIPE_LOOPBACK:
            return std::make_shared<LoopbackDisplayPipe>();
        default:
            MESON_ASSERT(0, "unknown display pipe %d", pipet);
    };

    return NULL;
}


