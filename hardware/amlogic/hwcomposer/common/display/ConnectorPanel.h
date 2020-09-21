/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _CONNECTORPANEL_H
#define _CONNECTORPANEL_H
#include <HwDisplayConnector.h>

class ConnectorPanel :public HwDisplayConnector {
public:
    ConnectorPanel(int32_t drvFd, uint32_t id);
    virtual ~ConnectorPanel();

    virtual int32_t loadProperities();
    virtual int32_t update();

    virtual const char * getName();
    virtual drm_connector_type_t getType();
    virtual bool isRemovable();
    virtual bool isConnected();
    virtual bool isSecure();

    virtual void getHdrCapabilities(drm_hdr_capabilities * caps);
    virtual void dump(String8 & dumpstr);

protected:
    int32_t loadDisplayModes();

    char mName[64];
};

#endif
