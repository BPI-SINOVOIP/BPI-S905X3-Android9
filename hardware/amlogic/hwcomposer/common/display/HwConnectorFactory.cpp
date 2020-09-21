/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "HwConnectorFactory.h"
#include "ConnectorHdmi.h"
#include "ConnectorCvbs.h"
#include "ConnectorPanel.h"
#include "ConnectorDummy.h"

std::shared_ptr<HwDisplayConnector> HwConnectorFactory::create(
    drm_connector_type_t connectorType,
    int32_t connectorDrv,
    uint32_t connectorId) {
    std::shared_ptr<HwDisplayConnector> connector = NULL;
    switch (connectorType) {
        case DRM_MODE_CONNECTOR_HDMI:
            connector =  std::make_shared<ConnectorHdmi>(connectorDrv, connectorId);
            break;
        case DRM_MODE_CONNECTOR_PANEL:
            connector =  std::make_shared<ConnectorPanel>(connectorDrv, connectorId);
            break;
        case DRM_MODE_CONNECTOR_DUMMY:
            connector =  std::make_shared<ConnectorDummy>(connectorDrv, connectorId);
            break;
        case DRM_MODE_CONNECTOR_CVBS:
            connector =  std::make_shared<ConnectorCvbs>(connectorDrv, connectorId);
            break;
        default:
            break;
    }

    return connector;
}

