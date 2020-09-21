/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef CONNECTOR_DUMMY_H
#define CONNECTOR_DUMMY_H
#include <HwDisplayConnector.h>

class ConnectorDummy :public HwDisplayConnector {
public:
    ConnectorDummy(int32_t drvFd, uint32_t id);
    ~ConnectorDummy();

    int32_t loadProperities();
    int32_t update();

    int32_t getModes(std::map<uint32_t, drm_mode_info_t> & modes);
    const char * getName();
    drm_connector_type_t getType();
    bool isRemovable();
    bool isConnected();
    bool isSecure();

    void getHdrCapabilities(drm_hdr_capabilities * caps);
    void dump(String8 & dumpstr);
};

#endif
