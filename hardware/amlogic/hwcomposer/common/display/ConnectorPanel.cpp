/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "ConnectorPanel.h"
#include <HwDisplayCrtc.h>
#include <misc.h>
#include <MesonLog.h>
#include "AmFramebuffer.h"
#include "AmVinfo.h"

ConnectorPanel::ConnectorPanel(int32_t drvFd, uint32_t id)
    :   HwDisplayConnector(drvFd, id) {
    snprintf(mName, 64, "Panel-%d", id);
}

ConnectorPanel::~ConnectorPanel() {
}

int32_t ConnectorPanel::loadProperities() {
    loadPhysicalSize();
    loadDisplayModes();
    return 0;
}

int32_t ConnectorPanel::update() {
    return 0;
}

const char * ConnectorPanel::getName() {
    return mName;
}

drm_connector_type_t ConnectorPanel::getType() {
    return DRM_MODE_CONNECTOR_PANEL;
}

bool ConnectorPanel::isRemovable() {
    return false;
}

bool ConnectorPanel::isConnected(){
    return true;
}

bool ConnectorPanel::isSecure(){
    return true;
}

int32_t ConnectorPanel::loadDisplayModes() {
    mDisplayModes.clear();

    std::string dispmode;
    vmode_e vmode = VMODE_MAX;
    if (NULL != mCrtc) {
        mCrtc->readCurDisplayMode(dispmode);
        vmode = vmode_name_to_mode(dispmode.c_str());
    }

    if (vmode == VMODE_MAX) {
        drm_mode_info_t modeInfo = {
            "panel",
            DEFAULT_DISPLAY_DPI,
            DEFAULT_DISPLAY_DPI,
            1920,
            1080,
            60};
        mDisplayModes.emplace(mDisplayModes.size(), modeInfo);
        MESON_LOGE("use default value,get display mode: %s", dispmode.c_str());
    } else {
        addDisplayMode(dispmode);

        //for tv display mode.
        const unsigned int pos = dispmode.find("60hz", 0);
        if (pos != std::string::npos) {
            dispmode.replace(pos, 4, "50hz");
            addDisplayMode(dispmode);
        } else {
            MESON_LOGE("loadDisplayModes can not find 60hz in %s", dispmode.c_str());
        }
    }
    return 0;
}

void ConnectorPanel::getHdrCapabilities(drm_hdr_capabilities * caps) {
    if (caps) {
        memset(caps, 0, sizeof(drm_hdr_capabilities));
    }
}

void ConnectorPanel:: dump(String8& dumpstr) {
    dumpstr.appendFormat("Connector (Panel,  %d)\n",1);
    dumpstr.append("   CONFIG   |   VSYNC_PERIOD   |   WIDTH   |   HEIGHT   |"
        "   DPI_X   |   DPI_Y   \n");
    dumpstr.append("------------+------------------+-----------+------------+"
        "-----------+-----------\n");

    std::map<uint32_t, drm_mode_info_t>::iterator it = mDisplayModes.begin();
    for ( ; it != mDisplayModes.end(); ++it) {
        dumpstr.appendFormat(" %2d     |      %.3f      |   %5d   |   %5d    |"
            "    %3d    |    %3d    \n",
                 it->first,
                 it->second.refreshRate,
                 it->second.pixelW,
                 it->second.pixelH,
                 it->second.dpiX,
                 it->second.dpiY);
    }
}

