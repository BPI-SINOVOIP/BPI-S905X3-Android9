/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "LegacyVideoPlane.h"
#include "AmFramebuffer.h"

#include <misc.h>
#include <OmxUtil.h>
#include <MesonLog.h>

#include <sys/ioctl.h>
#include <math.h>


//#define AMVIDEO_DEBUG

LegacyVideoPlane::LegacyVideoPlane(int32_t drvFd, uint32_t id)
    : HwDisplayPlane(drvFd, id) {
    snprintf(mName, 64, "AmVideo-%d", id);

    if (getMute(mPlaneMute) != 0) {
        MESON_LOGE("get video mute failed.");
        mPlaneMute = false;
    }

    mOmxKeepLastFrame = 0;
    mVideoType = DRM_FB_UNDEFINED;
    mLegacyVideoFb.reset();
    mBackupTransform = 0xff;
    memset(&mBackupDisplayFrame, 0, sizeof(drm_rect_t));
    getOmxKeepLastFrame(mOmxKeepLastFrame);
}

LegacyVideoPlane::~LegacyVideoPlane() {

}

uint32_t LegacyVideoPlane::getPlaneType() {
    return LEGACY_VIDEO_PLANE;
}

const char * LegacyVideoPlane::getName() {
    return mName;
}

uint32_t LegacyVideoPlane::getCapabilities() {
    return 0;
}

int32_t LegacyVideoPlane::getFixedZorder() {
    /*Legacy video plane not support PLANE_SUPPORT_ZORDER, always at bottom*/
    return LEGACY_VIDEO_PLANE_FIXED_ZORDER;
}

uint32_t LegacyVideoPlane::getPossibleCrtcs() {
    return CRTC_VOUT1;
}

bool LegacyVideoPlane::isFbSupport(std::shared_ptr<DrmFramebuffer> & fb) {
    if (fb->mFbType == DRM_FB_VIDEO_OVERLAY ||
        fb->mFbType == DRM_FB_VIDEO_SIDEBAND ||
        fb->mFbType == DRM_FB_VIDEO_OMX_PTS)
        return true;

    return false;
}

bool LegacyVideoPlane::shouldUpdateAxis(
    std::shared_ptr<DrmFramebuffer> &fb) {
    bool bUpdate = false;

    drm_rect_t *displayFrame = &(fb->mDisplayFrame);
    if (memcmp(&mBackupDisplayFrame, displayFrame, sizeof(drm_rect_t))) {
        memcpy(&mBackupDisplayFrame, displayFrame, sizeof(drm_rect_t));
        bUpdate = true;
    }

    if (mBackupTransform != fb->mTransform) {
        mBackupTransform = fb->mTransform;
        bUpdate = true;
    }

    return bUpdate;
}

int32_t LegacyVideoPlane::setPlane(
    std::shared_ptr<DrmFramebuffer> fb,
    uint32_t zorder, int blankOp) {
    if (fb) {
        /*this is added to slove this situation:
         *when source has the signal, then playing video in MoivePlayer.
         *Then, back to home from MoviePlayer.Garbage appears.
         */
        if (mLegacyVideoFb &&
            mVideoType == DRM_FB_VIDEO_OMX_PTS &&
            fb->mFbType != DRM_FB_VIDEO_OMX_PTS) {
            setVideodisableStatus(2);
        }

        mLegacyVideoFb = fb;
        mVideoType = mLegacyVideoFb->mFbType;

        native_handle_t * buf = fb->mBufferHandle;
#if 0
        /*set video crop:echo top left bottom right > /sys/class/video/crop*/
        if (mVideoType == DRM_FB_VIDEO_OMX_PTS) {
            char videoCropStr[AXIS_STR_LEN] = {0};
            sprintf(videoCropStr, "%d %d %d %d",
                fb->mSourceCrop.top, fb->mSourceCrop.left,
                am_gralloc_get_height(buf) - fb->mSourceCrop.bottom,
                am_gralloc_get_width(buf) - fb->mSourceCrop.right);
            sysfs_set_string(SYSFS_VIDEO_CROP, videoCropStr);
        }
#endif

        /*set video axis & rotation.*/
        if (shouldUpdateAxis(fb)) {
            char videoValStr[AXIS_STR_LEN] = {0};

            drm_rect_t * videoAxis = &(fb->mDisplayFrame);
            sprintf(videoValStr, "%d %d %d %d", videoAxis->left, videoAxis->top,
                videoAxis->right - 1, videoAxis->bottom - 1);
            sysfs_set_string(SYSFS_VIDEO_AXIS, videoValStr);

            int rotation = 0;
            switch (mLegacyVideoFb->mTransform) {
                case 0:
                    rotation = 0;
                    break;
                case HAL_TRANSFORM_ROT_90:
                    rotation = 90;
                    break;
                case HAL_TRANSFORM_ROT_180:
                    rotation = 180;
                    break;
                case HAL_TRANSFORM_ROT_270:
                    rotation = 270;
                    break;
                default:
                    rotation = 0;
                    break;
            };
            rotation = (rotation / 90) & 3;
            sprintf(videoValStr, "%d", rotation);
            sysfs_set_string(SYSFS_PPMGR_ANGLE, videoValStr);
        }

        /*set omx pts.*/
        if (am_gralloc_is_omx_metadata_buffer(buf)) {
            char *base = NULL;
            if (0 == gralloc_lock_dma_buf(buf, (void **)&base)) {
                set_omx_pts(base, &mDrvFd);
                gralloc_unlock_dma_buf(buf);
            } else {
                MESON_LOGE("set omx pts failed.");
            }
        }

        setZorder(zorder);
    }

    /*Update video plane blank status.*/
    if (!mLegacyVideoFb)
        return 0;

    if (blankOp == BLANK_FOR_SECURE_CONTENT) {
        setMute(true);
        return 0;
    } else if (blankOp == BLANK_FOR_NO_CONTENT) {
        setMute(true);
    } else if (blankOp == UNBLANK) {
        setMute(false);
    } else {
        MESON_LOGE("not support blank type: %d", blankOp);
    }

    int blankStatus = 0;
    getVideodisableStatus(blankStatus);

    if (mVideoType == DRM_FB_VIDEO_OVERLAY) {
        if (blankOp == BLANK_FOR_NO_CONTENT && (blankStatus == 0 || blankStatus == 2)) {
            setVideodisableStatus(1);
        }

        if (blankOp == UNBLANK && (blankStatus == 1 || blankStatus == 2)) {
            setVideodisableStatus(0);
        }
    } else if (mVideoType == DRM_FB_VIDEO_SIDEBAND || mVideoType == DRM_FB_VIDEO_OMX_PTS) {
        if (mOmxKeepLastFrame) {
            if (blankOp == BLANK_FOR_NO_CONTENT && (blankStatus == 0 || blankStatus == 2)) {
                setVideodisableStatus(1);
            }

            if (blankOp == UNBLANK && blankStatus == 1) {
                setVideodisableStatus(2);
            }
        }
    } else {
        MESON_LOGI("not support video fb type: %d", mVideoType);
    }

    if (blankOp == BLANK_FOR_NO_CONTENT) {
        mVideoType = DRM_FB_UNDEFINED;
        memset(&mBackupDisplayFrame, 0, sizeof(drm_rect_t));
        mBackupTransform = 0xff;
        mLegacyVideoFb.reset();
    }

    return 0;
}

int32_t LegacyVideoPlane::getMute(bool & staus) {
    uint32_t val = 1;
    if (ioctl(mDrvFd, AMSTREAM_IOC_GLOBAL_GET_VIDEO_OUTPUT, &val) != 0) {
        MESON_LOGE("AMSTREAM_GET_VIDEO_OUTPUT ioctl fail(%d)", errno);
        return -EINVAL;
    }
    staus = (val == 0) ? true : false;

    return 0;
}

int32_t LegacyVideoPlane::setMute(bool status) {
    if (mPlaneMute != status) {
        MESON_LOGD("muteVideo to %d", status);
        uint32_t val = status ? 0 : 1;
        if (ioctl(mDrvFd, AMSTREAM_IOC_GLOBAL_SET_VIDEO_OUTPUT, val) != 0) {
            MESON_LOGE("AMSTREAM_SET_VIDEO_OUTPUT ioctl (%d) return(%d)", status, errno);
            return -EINVAL;
        }
        mPlaneMute = status;
    } else {
#ifdef AMVIDEO_DEBUG
        bool val = true;
        getMute(val);
        if (mPlaneMute != val) {
            MESON_LOGE("status video (%d) vs (%d)", mPlaneMute, val);
        }
#endif
    }

    return 0;
}

int32_t LegacyVideoPlane::setZorder(uint32_t zorder) {
    if (ioctl(mDrvFd, AMSTREAM_IOC_SET_ZORDER, &zorder) != 0) {
        MESON_LOGE("AMSTREAM_IOC_SET_ZORDER ioctl return(%d)", errno);
        return -EINVAL;
    }
    return 0;
}

int32_t LegacyVideoPlane::getVideodisableStatus(int& status) {
    int ret = ioctl(mDrvFd, AMSTREAM_IOC_GET_VIDEO_DISABLE_MODE, &status);
    if (ret < 0) {
        MESON_LOGE("getvideodisable error, ret=%d", ret);
        return ret;
    }
    return 0;
}

int32_t LegacyVideoPlane::setVideodisableStatus(int status) {
    int ret = ioctl(mDrvFd, AMSTREAM_IOC_SET_VIDEO_DISABLE_MODE, &status);
    if (ret < 0) {
        MESON_LOGE("setvideodisable error, ret=%d", ret);
        return ret;
    }
    return 0;
}

int32_t LegacyVideoPlane::getOmxKeepLastFrame(unsigned int & keep) {
    int omx_info = 0;
    int ret = ioctl(mDrvFd, AMSTREAM_IOC_GET_OMX_INFO, (unsigned long)&omx_info);
    if (ret < 0) {
        MESON_LOGE("get omx info error, ret =%d", ret);
        keep = 0;
    } else {
        keep = omx_info & 0x1; //omx_info bit0: keep last frmame
        ret = 0;
    }
    return ret;
}

void LegacyVideoPlane::dump(String8 & dumpstr __unused) {
    MESON_LOG_EMPTY_FUN();
}

