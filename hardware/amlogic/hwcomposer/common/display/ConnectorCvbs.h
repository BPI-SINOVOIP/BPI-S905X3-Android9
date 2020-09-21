/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef CONNECTOR_CVBS_H
#define CONNECTOR_CVBS_H
#include <HwDisplayConnector.h>

class ConnectorCvbs : public HwDisplayConnector {
public:
    ConnectorCvbs(int32_t drvFd, uint32_t id);
    virtual ~ConnectorCvbs();

    virtual int32_t loadProperities();
    virtual int32_t update();

    virtual const char * getName();
    virtual drm_connector_type_t getType();
    virtual bool isRemovable();
    virtual bool isSecure();
    virtual bool isConnected();

    virtual void getHdrCapabilities(drm_hdr_capabilities * caps);
    virtual void dump(String8 & dumpstr);

private:
    char mName[64];
};

#endif

