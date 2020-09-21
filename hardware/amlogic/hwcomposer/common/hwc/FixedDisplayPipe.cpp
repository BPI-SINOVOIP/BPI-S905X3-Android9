/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "FixedDisplayPipe.h"
#include <HwcConfig.h>
#include <MesonLog.h>

FixedDisplayPipe::FixedDisplayPipe()
    : HwcDisplayPipe() {
}

FixedDisplayPipe::~FixedDisplayPipe() {
}

int32_t FixedDisplayPipe::init(
    std::map<uint32_t, std::shared_ptr<HwcDisplay>> & hwcDisps) {
    HwcDisplayPipe::init(hwcDisps);
    return 0;
}

void FixedDisplayPipe::handleEvent(drm_display_event event, int val) {
    HwcDisplayPipe::handleEvent(event, val);

    std::lock_guard<std::mutex> lock(mMutex);
    if (event == DRM_EVENT_HDMITX_HOTPLUG) {
        bool connected = (val == 0) ? false : true;
        std::shared_ptr<PipeStat> pipe;
        drm_connector_type_t targetConnector = DRM_MODE_CONNECTOR_INVALID;
        for (auto statIt : mPipeStats) {
            hwc_connector_t connectorType = HwcConfig::getConnectorType((int)statIt.first);
            if (connectorType == HWC_HDMI_CVBS) {
                pipe = statIt.second;
                targetConnector = connected ?
                    DRM_MODE_CONNECTOR_HDMI : DRM_MODE_CONNECTOR_CVBS;

                MESON_LOGD("handleEvent  DRM_EVENT_HDMITX_HOTPLUG %d VS %d",
                    pipe->cfg.hwcConnectorType, targetConnector);
                if (pipe->cfg.hwcConnectorType != targetConnector) {
                    #if 0 /*TODO: for fixed pipe, let systemcontrol to set displaymode.*/
                    pipe->hwcCrtc->unbind();
                    pipe->modeCrtc->unbind();
                    #endif
                    /* we need latest connector status, and no one will update
                    *connector not bind to crtc, we update here.
                    */
                    std::shared_ptr<HwDisplayConnector> hwConnector;
                    getConnector(targetConnector, hwConnector);
                    hwConnector->update();
                    /*update display pipe.*/
                    updatePipe(pipe);
                    /*update display mode, workaround now.*/
                    initDisplayMode(pipe);
                }
                break;
            } else if (connectorType == HWC_HDMI_ONLY && connected) {
                initDisplayMode(pipe);
            }
        }
    }
}

int32_t FixedDisplayPipe::getPipeCfg(uint32_t hwcid, PipeCfg & cfg) {
    drm_connector_type_t  connector = getConnetorCfg(hwcid);
    cfg.hwcCrtcId = CRTC_VOUT1;
    cfg.hwcPostprocessorType = INVALID_POST_PROCESSOR;
    cfg.modeCrtcId = cfg.hwcCrtcId;
    cfg.modeConnectorType = cfg.hwcConnectorType = connector;
    return 0;
}

drm_connector_type_t FixedDisplayPipe::getConnetorCfg(uint32_t hwcid) {
    drm_connector_type_t  connector = HwcDisplayPipe::getConnetorCfg(hwcid);
    if (connector == DRM_MODE_CONNECTOR_INVALID &&
        HwcConfig::getConnectorType(hwcid) == HWC_HDMI_CVBS) {
        std::shared_ptr<HwDisplayConnector> hwConnector;
        getConnector(DRM_MODE_CONNECTOR_HDMI, hwConnector);
        if (hwConnector->isConnected()) {
            connector = DRM_MODE_CONNECTOR_HDMI;
        } else {
            connector = DRM_MODE_CONNECTOR_CVBS;
        }
    }

    MESON_LOGE("FixedDisplayPipe::getConnetorCfg %d", connector);
    return connector;
}

