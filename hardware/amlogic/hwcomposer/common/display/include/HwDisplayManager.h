/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef HW_DISPLAY_MANAGER_H
#define HW_DISPLAY_MANAGER_H

#include <time.h>

#include <BasicTypes.h>
#include <HwDisplayCrtc.h>
#include <HwDisplayPlane.h>
#include <HwDisplayConnector.h>

class HwDisplayManager
    :   public android::Singleton<HwDisplayManager> {
friend class HwDisplayCrtc;
friend class HwDisplayConnector;
friend class HwDisplayPlane;

public:
    HwDisplayManager();
    ~HwDisplayManager();

    /* get all HwDisplayIds.*/

    /* get displayplanes by hw display idx, the planes may change when connector changed.*/
    int32_t getPlanes(
        std::vector<std::shared_ptr<HwDisplayPlane>> & planes);

    /* get displayplanes by hw display idx, the planes may change when connector changed.*/
    int32_t getCrtcs(
        std::vector<std::shared_ptr<HwDisplayCrtc>> & crtcs);

    int32_t getConnector(
        std::shared_ptr<HwDisplayConnector> & connector,
        drm_connector_type_t type);

/*********************************************
 * drm apis.
 *********************************************/
protected:
    int32_t loadDrmResources();
    int32_t freeDrmResources();

    int32_t loadCrtcs();
    int32_t loadConnectors();
    int32_t loadPlanes();

protected:
    std::map<uint32_t, std::shared_ptr<HwDisplayPlane>> mPlanes;
    std::map<uint32_t, std::shared_ptr<HwDisplayCrtc>> mCrtcs;
    std::map<drm_connector_type_t, std::shared_ptr<HwDisplayConnector>> mConnectors;

    std::mutex mMutex;
};

#endif/*HW_DISPLAY_MANAGER_H*/
