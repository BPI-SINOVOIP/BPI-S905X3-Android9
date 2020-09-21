/* Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <HwDisplayConnector.h>
#include <HwDisplayCrtc.h>
#include <MesonLog.h>
#include "AmVinfo.h"

HwDisplayConnector::HwDisplayConnector(int32_t drvFd, uint32_t id) {
    mDrvFd = drvFd;
    mId = id;
}

HwDisplayConnector::~HwDisplayConnector() {
}

int32_t HwDisplayConnector::setCrtc(HwDisplayCrtc * crtc) {
    mCrtc = crtc;
    return 0;
}

int32_t HwDisplayConnector::getModes(
    std::map<uint32_t, drm_mode_info_t> & modes) {
    modes = mDisplayModes;
    return 0;
}

void HwDisplayConnector::loadPhysicalSize() {
    struct vinfo_base_s info;
    if (!mCrtc)
        MESON_LOGE("loadPhysicalSize with non crtc, use CRTC 1.");
    int idx = mCrtc ? mCrtc->getId() : CRTC_VOUT1;
    int ret = read_vout_info(idx, &info);
    if (ret == 0) {
        mPhyWidth  = info.screen_real_width;
        mPhyHeight = info.screen_real_height;
    } else {
        mPhyWidth = mPhyHeight = 0;
        MESON_LOGE("read vout info failed return %d", ret);
    }
    MESON_LOGI("readDisplayPhySize physical size (%d x %d)", mPhyWidth, mPhyHeight);
}

int32_t HwDisplayConnector::addDisplayMode(std::string& mode) {
    vmode_e vmode = vmode_name_to_mode(mode.c_str());
    struct vinfo_s info;
    const struct vinfo_s* vinfo = NULL;

    if (VMODE_LCD == vmode) {
        /*panel display info is not fixed, need read from vout*/
        struct vinfo_base_s baseinfo;
        if (!mCrtc)
            MESON_LOGE("addDisplayMode(%s) with non crtc, use CRTC 1.", mode.c_str());
        int idx = mCrtc ? mCrtc->getId() : CRTC_VOUT1;
        int ret = read_vout_info(idx, &baseinfo);
        if (ret == 0) {
            vinfo = &info;

            info.name = mode.c_str();
            info.mode = vmode;
            info.width = baseinfo.width;
            info.height = baseinfo.height;
            info.field_height = baseinfo.field_height;
            info.aspect_ratio_num = baseinfo.aspect_ratio_num;
            info.aspect_ratio_den = baseinfo.aspect_ratio_den;
            info.sync_duration_num = baseinfo.sync_duration_num;
            info.sync_duration_den = baseinfo.sync_duration_den;
        } else {
            MESON_LOGE("addDisplayMode(%s) read_vout_info failed.", mode.c_str());
            return -ENOENT;
        }
    } else {
        vinfo = get_tv_info(vmode);
        if (vmode == VMODE_MAX || vinfo == NULL) {
            MESON_LOGE("addSupportedConfig meet error mode (%s, %d)", mode.c_str(), vmode);
            return -ENOENT;
        }
    }

    uint32_t dpiX  = DEFAULT_DISPLAY_DPI, dpiY = DEFAULT_DISPLAY_DPI;
    if (mPhyWidth > 16 && mPhyHeight > 9) {
        dpiX = (vinfo->width  * 25.4f) / mPhyWidth;
        dpiY = (vinfo->height  * 25.4f) / mPhyHeight;
        MESON_LOGI("add display mode real dpi (%d, %d)", dpiX, dpiY);
    }

    drm_mode_info_t modeInfo = {
        "",
        dpiX,
        dpiY,
        vinfo->width,
        vinfo->height,
        (float)vinfo->sync_duration_num/vinfo->sync_duration_den};
    strcpy(modeInfo.name, mode.c_str());

    mDisplayModes.emplace(mDisplayModes.size(), modeInfo);

    MESON_LOGI("add display mode (%s, %dx%d, %f)",
        mode.c_str(), vinfo->width, vinfo->height,
        (float)vinfo->sync_duration_num/vinfo->sync_duration_den);
    return 0;
}
