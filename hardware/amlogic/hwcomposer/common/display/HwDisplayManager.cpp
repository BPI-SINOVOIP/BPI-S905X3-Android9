/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <MesonLog.h>
#include <HwDisplayManager.h>
#include <HwDisplayConnector.h>
#include <HwDisplayCrtc.h>
#include <systemcontrol.h>
#include <DrmTypes.h>

#include "HwConnectorFactory.h"
#include "DummyPlane.h"
#include "OsdPlane.h"
#include "CursorPlane.h"
#include "LegacyVideoPlane.h"
#include "LegacyExtVideoPlane.h"
#include "HwcVideoPlane.h"
#include "AmFramebuffer.h"

ANDROID_SINGLETON_STATIC_INSTANCE(HwDisplayManager)

HwDisplayManager::HwDisplayManager() {
    loadDrmResources();
}

HwDisplayManager::~HwDisplayManager() {
    freeDrmResources();
}

int32_t HwDisplayManager::getPlanes(
    std::vector<std::shared_ptr<HwDisplayPlane>> & planes) {
    mMutex.lock();
    for (const auto & plane : mPlanes)
        planes.push_back(plane.second);
    mMutex.unlock();
    return 0;
}

int32_t HwDisplayManager::getCrtcs(
        std::vector<std::shared_ptr<HwDisplayCrtc>> & crtcs) {
    mMutex.lock();
    for (const auto & crtc : mCrtcs)
        crtcs.push_back(crtc.second);
    mMutex.unlock();
    return 0;
}

int32_t HwDisplayManager::getConnector(
        std::shared_ptr<HwDisplayConnector> & connector,
        drm_connector_type_t type) {
    mMutex.lock();

    auto it = mConnectors.find(type);
    if (it != mConnectors.end()) {
        connector = it->second;
        MESON_LOGD("get existing connector %d-%p", type, connector.get());
    } else {
        int connectorId = CONNECTOR_IDX_MIN + mConnectors.size();
        std::shared_ptr<HwDisplayConnector> newConnector = HwConnectorFactory::create(
                type, -1, connectorId);
        MESON_ASSERT(newConnector != NULL, "unsupported connector type %d", type);
        MESON_LOGD("create new connector %d-%p", type, newConnector.get());
        connector = newConnector;
        mConnectors.emplace(type, newConnector);
    }

    mMutex.unlock();
    return 0;
}

/********************************************************************
 *   The following functions need update with drm.                  *
 *   Now is hard code for 1 crtc , 1 connector.                     *
 ********************************************************************/
int32_t HwDisplayManager::loadDrmResources() {
    mMutex.lock();
    /* must call loadPlanes() first, now it is the only
    *valid function.
    */
    loadPlanes();
    loadConnectors();
    loadCrtcs();

    mMutex.unlock();
    return 0;
}

int32_t HwDisplayManager::freeDrmResources() {
    mCrtcs.clear();
    mConnectors.clear();
    mPlanes.clear();
    return 0;
}

int32_t HwDisplayManager::loadCrtcs() {
    MESON_LOGV("Crtc loaded in loadPlanes: %d", mCrtcs.size());
    return 0;
}

int32_t HwDisplayManager::loadConnectors() {
    MESON_LOGV("No connector api, set empty, will create when hwc get.");
    return 0;
}

int32_t HwDisplayManager::loadPlanes() {
    /* scan /dev/graphics/fbx to get planes */
    int fd = -1;
    char path[64];
    int count_osd = 0, count_video = 0;
    int idx = 0, plane_idx = 0, video_idx_max = 0;
    int capability = 0x0;

    /*osd plane.*/
    do {
        snprintf(path, 64, "/dev/graphics/fb%u", idx);
        fd = open(path, O_RDWR, 0);
        if (fd >= 0) {
            plane_idx = OSD_PLANE_IDX_MIN + idx;
            if (ioctl(fd, FBIOGET_OSD_CAPBILITY, &capability) != 0) {
                MESON_LOGE("osd plane get capibility ioctl (%d) return(%d)", capability, errno);
                return -EINVAL;
            }
            if (capability & OSD_LAYER_ENABLE) {
                if (capability & OSD_HW_CURSOR) {
                    std::shared_ptr<CursorPlane> plane = std::make_shared<CursorPlane>(fd, plane_idx);
                    mPlanes.emplace(plane_idx, plane);
                } else {
                    std::shared_ptr<OsdPlane> plane = std::make_shared<OsdPlane>(fd, plane_idx);
                    mPlanes.emplace(plane_idx, plane);

                    /*add valid crtc.*/
                    uint32_t crtcs = plane->getPossibleCrtcs();
                    if ((crtcs & CRTC_VOUT1) && mCrtcs.count(CRTC_VOUT1) == 0) {
                        std::shared_ptr<HwDisplayCrtc> crtc =
                            std::make_shared<HwDisplayCrtc>(::dup(fd), CRTC_VOUT1);
                        mCrtcs.emplace(CRTC_VOUT1, crtc);
                    } else if ((crtcs & CRTC_VOUT2) && mCrtcs.count(CRTC_VOUT2) == 0) {
                        std::shared_ptr<HwDisplayCrtc> crtc =
                            std::make_shared<HwDisplayCrtc>(::dup(fd), CRTC_VOUT2);
                        mCrtcs.emplace(CRTC_VOUT2, crtc);
                    }
                }
                count_osd ++;
            }
        }
        idx ++;
    } while(fd >= 0);

    /*legacy video plane.*/
    video_idx_max = VIDEO_PLANE_IDX_MIN;
    idx = 0;
    do {
        if (idx == 0) {
            snprintf(path, 64, "/dev/amvideo");
        } else {
            snprintf(path, 64, "/dev/amvideo%u", idx);
        }
        fd = open(path, O_RDWR, 0);
        if (fd >= 0) {
            plane_idx = video_idx_max + count_video;
            std::shared_ptr<LegacyVideoPlane> plane =
                std::make_shared<LegacyVideoPlane>(fd, plane_idx);
            mPlanes.emplace(plane_idx, plane);
            count_video ++;
            plane_idx = plane_idx + count_video;
            std::shared_ptr<LegacyExtVideoPlane> extPlane =
                std::make_shared<LegacyExtVideoPlane>(fd, plane_idx);
            mPlanes.emplace(plane_idx, extPlane);
            count_video ++;
        }
        idx ++;
    } while(fd >= 0);

    /*hwc video plane.*/
    video_idx_max = video_idx_max + count_video;
    idx = 0;
    do {
        snprintf(path, 64, "/dev/video_hwc%u", idx);
        fd = open(path, O_RDWR, 0);
        if (fd >= 0) {
            plane_idx = video_idx_max + idx;
            std::shared_ptr<HwcVideoPlane> plane =
                std::make_shared<HwcVideoPlane>(fd, plane_idx);
            mPlanes.emplace(plane_idx, plane);
            count_video ++;
        }
        idx ++;
    } while(fd >= 0);

    MESON_LOGD("get osd planes (%d), video planes (%d)", count_osd, count_video);

    return 0;
}

