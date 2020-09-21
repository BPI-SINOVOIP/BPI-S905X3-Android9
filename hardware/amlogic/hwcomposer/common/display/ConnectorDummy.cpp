/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "ConnectorDummy.h"

ConnectorDummy::ConnectorDummy(int32_t drvFd, uint32_t id)
    : HwDisplayConnector(drvFd, id) {
}

ConnectorDummy::~ConnectorDummy() {
}

int32_t ConnectorDummy::loadProperities() {
    return 0;
}

int32_t ConnectorDummy::update() {
    return 0;
}

int32_t ConnectorDummy::getModes(
    std::map<uint32_t, drm_mode_info_t> & modes) {
    modes.clear();

    drm_mode_info_t modeInfo = {
        "dummy_panel",
        160,
        160,
        1920,
        1080,
        60.0};

    modes.emplace(0, modeInfo);
    return 0;
}

const char * ConnectorDummy::getName() {
    return "dummy_panel";
}

drm_connector_type_t ConnectorDummy::getType() {
    return DRM_MODE_CONNECTOR_DUMMY;
}

bool ConnectorDummy::isRemovable() {
    return false;
}

bool ConnectorDummy::isConnected() {
    return true;
}

bool ConnectorDummy::isSecure() {
    return true;
}

void ConnectorDummy::getHdrCapabilities(drm_hdr_capabilities * caps) {
    UNUSED(caps);
}

void ConnectorDummy::dump(String8 & dumpstr) {
    UNUSED(dumpstr);
}

