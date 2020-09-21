/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <MesonLog.h>
#include "HwcVideoPlane.h"
#include "AmFramebuffer.h"


HwcVideoPlane::HwcVideoPlane(int32_t drvFd, uint32_t id)
    : HwDisplayPlane(drvFd, id) {
    snprintf(mName, 64, "HwcVideo-%d", id);
}

HwcVideoPlane::~HwcVideoPlane() {
}

const char * HwcVideoPlane::getName() {
    return mName;
}

uint32_t HwcVideoPlane::getPlaneType() {
    return HWC_VIDEO_PLANE;
}

uint32_t HwcVideoPlane::getCapabilities() {
    /*HWCVideoplane always support zorder.*/
    return PLANE_SUPPORT_ZORDER;
}

int32_t HwcVideoPlane::getFixedZorder() {
    return INVALID_ZORDER;
}

uint32_t HwcVideoPlane::getPossibleCrtcs() {
    return CRTC_VOUT1;
}

bool HwcVideoPlane::isFbSupport(std::shared_ptr<DrmFramebuffer> & fb) {
    if (fb->mFbType == DRM_FB_VIDEO_OMX_V4L)
        return true;

    return false;
}

int32_t HwcVideoPlane::setPlane(
    std::shared_ptr<DrmFramebuffer> fb __unused,
    uint32_t zorder __unused, int blankOp __unused) {
    if (mDrvFd < 0) {
        MESON_LOGE("hwcvideo plane fd is not valiable!");
        return -EBADF;
    }
#if 0
    drm_rect_t srcCrop       = fb->mSourceCrop;
    drm_rect_t disFrame      = fb->mDisplayFrame;
    buffer_handle_t buf      = fb->mBufferHandle;

    mPlaneInfo.magic         = OSD_SYNC_REQUEST_RENDER_MAGIC_V2;
    mPlaneInfo.len           = sizeof(osd_plane_info_t);
    mPlaneInfo.type          = DIRECT_COMPOSE_MODE;

    mPlaneInfo.xoffset       = srcCrop.left;
    mPlaneInfo.yoffset       = srcCrop.top;
    mPlaneInfo.width         = srcCrop.right    - srcCrop.left;
    mPlaneInfo.height        = srcCrop.bottom   - srcCrop.top;
    mPlaneInfo.dst_x         = disFrame.left;
    mPlaneInfo.dst_y         = disFrame.top;
    mPlaneInfo.dst_w         = disFrame.right   - disFrame.left;
    mPlaneInfo.dst_h         = disFrame.bottom  - disFrame.top;

    if (DebugHelper::getInstance().discardInFence()) {
        fb->getAcquireFence()->waitForever("osd-input");
        mPlaneInfo.in_fen_fd = -1;
    } else {
        mPlaneInfo.in_fen_fd     = fb->getAcquireFence()->dup();
    }
    mPlaneInfo.format        = am_gralloc_get_format(buf);
    mPlaneInfo.shared_fd     = am_gralloc_get_buffer_fd(buf);
    mPlaneInfo.byte_stride   = am_gralloc_get_stride_in_byte(buf);
    mPlaneInfo.pixel_stride  = am_gralloc_get_stride_in_pixel(buf);
    /* osd request plane zorder > 0 */
    mPlaneInfo.zorder        = fb->mZorder + 1;
    mPlaneInfo.blend_mode    = fb->mBlendMode;
    mPlaneInfo.plane_alpha   = fb->mPlaneAlpha;
    mPlaneInfo.op            &= ~(OSD_BLANK_OP_BIT);
    mPlaneInfo.afbc_inter_format = am_gralloc_get_vpu_afbc_mask(buf);

    if (ioctl(mDrvFd, FBIOPUT_OSD_SYNC_RENDER_ADD, &mPlaneInfo) != 0) {
        MESON_LOGE("osd plane FBIOPUT_OSD_SYNC_RENDER_ADD return(%d)", errno);
        return -EINVAL;
    }

    if (mDrmFb) {
    /* dup a out fence fd for layer's release fence, we can't close this fd
    * now, cause display retire fence will also use this fd. will be closed
    * on SF side*/
        if (DebugHelper::getInstance().discardOutFence()) {
            mDrmFb->setReleaseFence(-1);
        } else {
            mDrmFb->setReleaseFence((mPlaneInfo.out_fen_fd >= 0) ? ::dup(mPlaneInfo.out_fen_fd) : -1);
        }
    }

    // update drm fb.
    mDrmFb = fb;

    mPlaneInfo.in_fen_fd  = -1;
    mPlaneInfo.out_fen_fd = -1;
#endif
    return 0;
}

void HwcVideoPlane::dump(String8 & dumpstr __unused) {
#if 0
    if (!mBlank) {
        dumpstr.appendFormat("HwcVideo%2d "
                "     %3d | %1d | %4d, %4d, %4d, %4d |  %4d, %4d, %4d, %4d | %2d | %2d | %4d |"
                " %4d | %5d | %5d | %4x |%8x  |\n",
                 mId,
                 mPlaneInfo.zorder,
                 mPlaneInfo.type,
                 mPlaneInfo.xoffset, mPlaneInfo.yoffset, mPlaneInfo.width, mPlaneInfo.height,
                 mPlaneInfo.dst_x, mPlaneInfo.dst_y, mPlaneInfo.dst_w, mPlaneInfo.dst_h,
                 mPlaneInfo.shared_fd,
                 mPlaneInfo.format,
                 mPlaneInfo.byte_stride,
                 mPlaneInfo.pixel_stride,
                 mPlaneInfo.blend_mode,
                 mPlaneInfo.plane_alpha,
                 mPlaneInfo.op,
                 mPlaneInfo.afbc_inter_format);
    }
#endif
}

