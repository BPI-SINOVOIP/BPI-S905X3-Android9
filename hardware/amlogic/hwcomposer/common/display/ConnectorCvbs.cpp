/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <ConnectorCvbs.h>

ConnectorCvbs::ConnectorCvbs(int32_t drvFd, uint32_t id)
    :   HwDisplayConnector(drvFd, id) {
    snprintf(mName, 64, "CVBS-%d", id);
}

ConnectorCvbs::~ConnectorCvbs() {
}

int32_t ConnectorCvbs::loadProperities() {
    loadPhysicalSize();
    mDisplayModes.clear();

    std::string cvbs576("576cvbs");
    std::string cvbs480("480cvbs");
    std::string pal_m("pal_m");
    std::string pal_n("pal_n");
    std::string ntsc_m("ntsc_m");
    addDisplayMode(cvbs576);
    addDisplayMode(cvbs480);
    addDisplayMode(pal_m);
    addDisplayMode(pal_n);
    addDisplayMode(ntsc_m);
    return 0;
}

int32_t ConnectorCvbs::update() {
    return 0;
}

const char * ConnectorCvbs::getName() {
    return mName;
}

drm_connector_type_t ConnectorCvbs::getType() {
    return DRM_MODE_CONNECTOR_CVBS;
}

bool ConnectorCvbs::isRemovable() {
    return true;
}

bool ConnectorCvbs::isSecure() {
    return false;
}

bool ConnectorCvbs::isConnected() {
    return true;
}

void ConnectorCvbs::getHdrCapabilities(drm_hdr_capabilities * caps) {
    UNUSED(caps);
}

void ConnectorCvbs::dump(String8 & dumpstr) {
    UNUSED(dumpstr);
}

