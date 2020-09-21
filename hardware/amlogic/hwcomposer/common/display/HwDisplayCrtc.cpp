/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <HwDisplayManager.h>
#include <HwDisplayCrtc.h>
#include <MesonLog.h>
#include <DebugHelper.h>
#include <cutils/properties.h>
#include <systemcontrol.h>
#include <misc.h>
#include <math.h>
#include <OmxUtil.h>

#include "AmVinfo.h"
#include "AmFramebuffer.h"

static vframe_master_display_colour_s_t nullHdr;

#define VIU1_DISPLAY_MODE_SYSFS "/sys/class/display/mode"
#define VIU2_DISPLAY_MODE_SYSFS "/sys/class/display2/mode"
#define VIU_DISPLAY_ATTR_SYSFS "/sys/class/amhdmitx/amhdmitx0/attr"

HwDisplayCrtc::HwDisplayCrtc(int drvFd, int32_t id) {
    MESON_ASSERT(id == CRTC_VOUT1 || id == CRTC_VOUT2, "Invalid crtc id %d", id);

    mId = id;
    mDrvFd = drvFd;
    mFirstPresent = true;
    mBinded = false;
    /*for old vpu, always one channel.
    *for new vpu, it can be 1 or 2.
    */
    mOsdChannels = 1;
    memset(&nullHdr, 0, sizeof(nullHdr));

    hdrVideoInfo = malloc(sizeof(vframe_master_display_colour_s_t));

}

HwDisplayCrtc::~HwDisplayCrtc() {
    free(hdrVideoInfo);
}

int32_t HwDisplayCrtc::bind(
    std::shared_ptr<HwDisplayConnector>  connector,
    std::vector<std::shared_ptr<HwDisplayPlane>> planes) {
    if (mBinded) {
        if (mConnector.get())
            mConnector->setCrtc(NULL);
        mConnector.reset();
        mPlanes.clear();
        mBinded =  false;
    }

    mConnector = connector;
    mConnector->setCrtc(this);
    mPlanes = planes;
    mBinded = true;
    return 0;
}

int32_t HwDisplayCrtc::unbind() {
    if (mBinded) {
        static drm_mode_info_t nullMode = {
            DRM_DISPLAY_MODE_NULL,
            0, 0,
            0, 0,
            60.0
        };
        std::string dispmode(nullMode.name);
        writeCurDisplayMode(dispmode);
        if (mConnector.get())
            mConnector->setCrtc(NULL);
        mConnector.reset();
        mPlanes.clear();

        mBinded = false;
    }
    return 0;
}

int32_t HwDisplayCrtc::loadProperities() {
    /*load static information when pipeline present.*/
    {
        std::lock_guard<std::mutex> lock(mMutex);
        MESON_ASSERT(mConnector, "Crtc need setuped before load Properities.");
        mConnector->loadProperities();
        mModes.clear();
        mConnected = mConnector->isConnected();
        if (mConnected) {
            mConnector->getModes(mModes);
            /*TODO: add display mode filter here
            * to remove unsupported displaymode.
            */
        }
    }

    return 0;
}

int32_t HwDisplayCrtc::getId() {
    return mId;
}

int32_t HwDisplayCrtc::setMode(drm_mode_info_t & mode) {
    /*DRM_DISPLAY_MODE_NULL is always allowed.*/
    MESON_LOGI("Crtc setMode: %s", mode.name);
    std::string dispmode(mode.name);
    return writeCurDisplayMode(dispmode);
}

int32_t HwDisplayCrtc::getMode(drm_mode_info_t & mode) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mConnected || mCurModeInfo.name[0] == 0)
        return -EFAULT;

    mode = mCurModeInfo;
    return 0;
}

int32_t HwDisplayCrtc::waitVBlank(nsecs_t & timestamp) {
    int32_t ret = ioctl(mDrvFd, FBIO_WAITFORVSYNC_64, &timestamp);
    if (ret == -1) {
        ret = -errno;
        MESON_LOGE("fb ioctl vsync wait error, ret: %d", ret);
        return ret;
    } else {
        if (timestamp != 0) {
            return 0;
        } else {
            MESON_LOGE("wait for vsync fail");
            return -EINVAL;
        }
    }
}

int32_t HwDisplayCrtc::update() {
    /*update dynamic information which may change.*/
    std::lock_guard<std::mutex> lock(mMutex);
    memset(&mCurModeInfo, 0, sizeof(drm_mode_info_t));
    if (mConnector)
        mConnector->update();
    if (mConnected) {
        /*1. update current displayMode.*/
        std::string displayMode;
        readCurDisplayMode(displayMode);
        if (displayMode.empty()) {
             MESON_LOGE("displaymode should not null when connected.");
        } else {
            for (auto it = mModes.begin(); it != mModes.end(); it ++) {
                MESON_LOGD("update: (%s) mode (%s)", displayMode.c_str(), it->second.name);
                if (strcmp(it->second.name, displayMode.c_str()) == 0
                     && it->second.refreshRate == floor(it->second.refreshRate)) {
                    memcpy(&mCurModeInfo, &it->second, sizeof(drm_mode_info_t));
                    break;
                }
            }
            MESON_LOGD("crtc(%d) update (%s) (%d) -> (%s).",
                mId, displayMode.c_str(), mModes.size(), mCurModeInfo.name);
        }
    } else {
        MESON_LOGD("crtc(%d) update with no connector", mId);
    }

    return 0;
}

int32_t HwDisplayCrtc::setDisplayFrame(display_zoom_info_t & info) {
    mScaleInfo = info;
    /*not used now, clear to 0.*/
    mScaleInfo.crtc_w = 0;
    mScaleInfo.crtc_h = 0;
    return 0;
}

int32_t HwDisplayCrtc::setOsdChannels(int32_t channels) {
    mOsdChannels = channels;
    return 0;
}

int32_t HwDisplayCrtc::pageFlip(int32_t &out_fence) {
    if (mFirstPresent) {
        mFirstPresent = false;
        closeLogoDisplay();
    }

    osd_page_flip_info_t flipInfo;
    flipInfo.background_w = mScaleInfo.framebuffer_w;
    flipInfo.background_h = mScaleInfo.framebuffer_h;
    flipInfo.fullScreen_w = mScaleInfo.framebuffer_w;
    flipInfo.fullScreen_h = mScaleInfo.framebuffer_h;
    flipInfo.curPosition_x = mScaleInfo.crtc_display_x;
    flipInfo.curPosition_y = mScaleInfo.crtc_display_y;
    flipInfo.curPosition_w = mScaleInfo.crtc_display_w;
    flipInfo.curPosition_h = mScaleInfo.crtc_display_h;
    flipInfo.hdr_mode = (mOsdChannels == 1) ? 1 : 0;

    ioctl(mDrvFd, FBIOPUT_OSD_DO_HWC, &flipInfo);

    if (DebugHelper::getInstance().discardOutFence()) {
        std::shared_ptr<DrmFence> outfence =
            std::make_shared<DrmFence>(flipInfo.out_fen_fd);
        outfence->waitForever("crtc-output");
        out_fence = -1;
    } else {
        out_fence = (flipInfo.out_fen_fd >= 0) ? flipInfo.out_fen_fd : -1;
    }

    return 0;
}

int32_t HwDisplayCrtc::getHdrMetadataKeys(
    std::vector<drm_hdr_meatadata_t> & keys) {
    static drm_hdr_meatadata_t supportedKeys[] = {
        DRM_DISPLAY_RED_PRIMARY_X,
        DRM_DISPLAY_RED_PRIMARY_Y,
        DRM_DISPLAY_GREEN_PRIMARY_X,
        DRM_DISPLAY_GREEN_PRIMARY_Y,
        DRM_DISPLAY_BLUE_PRIMARY_X,
        DRM_DISPLAY_BLUE_PRIMARY_Y,
        DRM_WHITE_POINT_X,
        DRM_WHITE_POINT_Y,
        DRM_MAX_LUMINANCE,
        DRM_MIN_LUMINANCE,
        DRM_MAX_CONTENT_LIGHT_LEVEL,
        DRM_MAX_FRAME_AVERAGE_LIGHT_LEVEL,
    };

    for (uint32_t i = 0;i < sizeof(supportedKeys)/sizeof(drm_hdr_meatadata_t); i++) {
        keys.push_back(supportedKeys[i]);
    }

    return 0;
}

int32_t HwDisplayCrtc::setHdrMetadata(
    std::map<drm_hdr_meatadata_t, float> & hdrmedata) {
    if (updateHdrMetadata(hdrmedata) == true)
        return set_hdr_info((vframe_master_display_colour_s_t*)hdrVideoInfo);

    return 0;
}

bool HwDisplayCrtc::updateHdrMetadata(
    std::map<drm_hdr_meatadata_t, float> & hdrmedata) {
    vframe_master_display_colour_s_t newHdr;
    memset(&newHdr,0,sizeof(vframe_master_display_colour_s_t));
    if (!hdrmedata.empty()) {
        for (auto iter = hdrmedata.begin(); iter != hdrmedata.end(); ++iter) {
            switch (iter->first) {
                case DRM_DISPLAY_RED_PRIMARY_X:
                    newHdr.primaries[2][0] = (u32)(iter->second * 50000); //mR.x
                    break;
                case DRM_DISPLAY_RED_PRIMARY_Y:
                    newHdr.primaries[2][1] = (u32)(iter->second * 50000); //mR.Y
                    break;
                case DRM_DISPLAY_GREEN_PRIMARY_X:
                    newHdr.primaries[0][0] = (u32)(iter->second * 50000);//mG.x
                    break;
                case DRM_DISPLAY_GREEN_PRIMARY_Y:
                    newHdr.primaries[0][1] = (u32)(iter->second * 50000);//mG.y
                    break;
                case DRM_DISPLAY_BLUE_PRIMARY_X:
                    newHdr.primaries[1][0] = (u32)(iter->second * 50000);//mB.x
                    break;
                case DRM_DISPLAY_BLUE_PRIMARY_Y:
                    newHdr.primaries[1][1] = (u32)(iter->second * 50000);//mB.Y
                    break;
                case DRM_WHITE_POINT_X:
                    newHdr.white_point[0] = (u32)(iter->second * 50000);//mW.x
                    break;
                case DRM_WHITE_POINT_Y:
                    newHdr.white_point[1] = (u32)(iter->second * 50000);//mW.Y
                    break;
                case DRM_MAX_LUMINANCE:
                    newHdr.luminance[0] = (u32)(iter->second * 1000); //mMaxDL
                    break;
                case DRM_MIN_LUMINANCE:
                    newHdr.luminance[1] = (u32)(iter->second * 10000);//mMinDL
                    break;
                case DRM_MAX_CONTENT_LIGHT_LEVEL:
                    newHdr.content_light_level.max_content = (u32)(iter->second); //mMaxCLL
                    newHdr.content_light_level.present_flag = 1;
                    break;
                case DRM_MAX_FRAME_AVERAGE_LIGHT_LEVEL:
                    newHdr.content_light_level.max_pic_average = (u32)(iter->second);//mMaxFALL
                    newHdr.content_light_level.present_flag = 1;
                    break;
                default:
                    MESON_LOGE("unkown key %d",iter->first);
                    break;
            }
        }
    }

    if (memcmp(hdrVideoInfo, &nullHdr, sizeof(vframe_master_display_colour_s_t)) == 0)
        return false;
    newHdr.present_flag = 1;

    if (memcmp(hdrVideoInfo, &newHdr, sizeof(vframe_master_display_colour_s_t)) == 0)
        return false;

    vframe_master_display_colour_s_t * hdrinfo =
        (vframe_master_display_colour_s_t *)hdrVideoInfo;
    *hdrinfo = newHdr;
    return true;
}

void HwDisplayCrtc::closeLogoDisplay() {
    sysfs_set_string(DISPLAY_LOGO_INDEX, "-1");
    sysfs_set_string(DISPLAY_FB0_FREESCALE_SWTICH, "0x10001");
}

int32_t  HwDisplayCrtc::readCurDisplayMode(std::string & dispmode) {
    int32_t ret = 0;
    if (mId == CRTC_VOUT1) {
        ret = sc_read_sysfs(VIU1_DISPLAY_MODE_SYSFS, dispmode);
    }  else if (mId == CRTC_VOUT2) {
        ret = sc_read_sysfs(VIU2_DISPLAY_MODE_SYSFS, dispmode);
    }

    return ret;
}

int32_t HwDisplayCrtc::writeCurDisplayMode(std::string & dispmode) {
    int32_t ret = 0;
    if (mId == CRTC_VOUT1) {
        if (!strcmp(dispmode.c_str(), DRM_DISPLAY_MODE_NULL))
            ret = sc_write_sysfs(VIU1_DISPLAY_MODE_SYSFS, dispmode);
        else
            ret = sc_set_display_mode(dispmode);
    } else if (mId == CRTC_VOUT2) {
        ret = sc_write_sysfs(VIU2_DISPLAY_MODE_SYSFS, dispmode);
    }

    return ret;
}

int32_t HwDisplayCrtc::writeCurDisplayAttr(std::string & dispattr) {
    int32_t ret = 0;
    ret = sc_write_sysfs(VIU_DISPLAY_ATTR_SYSFS, dispattr);

    return ret;
}
