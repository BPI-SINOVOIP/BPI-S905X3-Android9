/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef HW_CONNECTOR_FACTORY_H
#define HW_CONNECTOR_FACTORY_H

#include <HwDisplayConnector.h>

class HwConnectorFactory {
public:
    static std::shared_ptr<HwDisplayConnector> create(
        drm_connector_type_t connectorType,
        int32_t connectorDrv,
        uint32_t connectorId);
};


#endif/*HW_CONNECTOR_FACTORY_H*/
