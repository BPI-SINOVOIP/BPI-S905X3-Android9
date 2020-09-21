/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "OsdPlane.h"
#include <MesonLog.h>
#include <DebugHelper.h>

OsdPlane::OsdPlane(int32_t drvFd, uint32_t id)
    : HwDisplayPlane(drvFd, id),
      mBlank(false),
      mPossibleCrtcs(0),
      mDrmFb(NULL) {
    snprintf(mName, 64, "OSD-%d", id);
    mPlaneInfo.out_fen_fd = -1;
    getProperties();
}

OsdPlane::~OsdPlane() {
}

int32_t OsdPlane::getProperties() {
    int capacity;
    if (ioctl(mDrvFd, FBIOGET_OSD_CAPBILITY, &capacity) != 0) {
        MESON_LOGE("osd plane get capibility ioctl (%d) return(%d)", mCapability, errno);
        return 0;
    }

    mCapability = 0;

    if (capacity & OSD_UBOOT_LOGO) {
        mCapability |= PLANE_SHOW_LOGO;
    }
    if (capacity & OSD_ZORDER) {
        mCapability |= PLANE_SUPPORT_ZORDER;
    }
    if (capacity & OSD_PRIMARY) {
        mCapability |= PLANE_PRIMARY;
    }
    if (capacity & OSD_FREESCALE) {
        mCapability |= PLANE_SUPPORT_FREE_SCALE;
    }
    if (capacity & OSD_AFBC) {
        mCapability |= PLANE_SUPPORT_AFBC;
    }

    /*set possible crtc*/
    if (capacity & OSD_VIU1) {
        mPossibleCrtcs |= CRTC_VOUT1;
    }
    if (capacity & OSD_VIU2) {
        mPossibleCrtcs |= CRTC_VOUT2;
    }

    return 0;
}

const char * OsdPlane::getName() {
    return mName;
}

uint32_t OsdPlane::getPlaneType() {
    if (mIdle) {
        return INVALID_PLANE;
    }

    return OSD_PLANE;
}

uint32_t OsdPlane::getCapabilities() {
    return mCapability;
}

int32_t OsdPlane::getFixedZorder() {
    if (mCapability & PLANE_SUPPORT_ZORDER) {
        return INVALID_ZORDER;
    }

    return OSD_PLANE_FIXED_ZORDER;
}

uint32_t OsdPlane::getPossibleCrtcs() {
    return mPossibleCrtcs;
}

bool OsdPlane::isFbSupport(std::shared_ptr<DrmFramebuffer> & fb) {
    if (fb->isRotated())
         return false;

    //if cursor fb, check if buffer is cont
    switch (fb->mFbType) {
        case DRM_FB_CURSOR:
            if (!am_gralloc_is_coherent_buffer(fb->mBufferHandle))
                return false;
        case DRM_FB_SCANOUT:
            break;
        //case DRM_FB_COLOR:
            //return true;
        default:
            return false;
    }

    unsigned int blendMode = fb->mBlendMode;
    if (blendMode != DRM_BLEND_MODE_NONE
        && blendMode != DRM_BLEND_MODE_PREMULTIPLIED
        && blendMode != DRM_BLEND_MODE_COVERAGE) {
        MESON_LOGE("Blend mode is invalid!");
        return false;
    }

    int format = am_gralloc_get_format(fb->mBufferHandle);
    int afbc = am_gralloc_get_vpu_afbc_mask(fb->mBufferHandle);

    if (blendMode == DRM_BLEND_MODE_NONE && format == HAL_PIXEL_FORMAT_BGRA_8888) {
        MESON_LOGE("blend mode: %u, Layer format %d not support.", blendMode, format);
        return false;
    }
    if (afbc == 0) {
        switch (format) {
            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_RGB_888:
            case HAL_PIXEL_FORMAT_RGB_565:
            case HAL_PIXEL_FORMAT_BGRA_8888:
                break;
            default:
                MESON_LOGE("afbc: %d, Layer format %d not support.", afbc, format);
                return false;
        }
    } else {
        if ((mCapability & PLANE_SUPPORT_AFBC) == PLANE_SUPPORT_AFBC) {
            switch (format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGBX_8888:
                    break;
                default:
                    MESON_LOGE("afbc: %d, Layer format %d not support.", afbc, format);
                    return false;
            }
        } else {
            MESON_LOGI("AFBC buffer && unsupported AFBC plane, turn to GPU composition");
            return false;
        }
    }

    uint32_t sourceWidth = fb->mSourceCrop.bottom - fb->mSourceCrop.top;
    uint32_t sourceHeight = fb->mSourceCrop.right - fb->mSourceCrop.left;
    if (sourceWidth > OSD_INPUT_MAX_HEIGHT ||sourceHeight > OSD_INPUT_MAX_WIDTH)
        return false;
    if (sourceWidth < OSD_INPUT_MIN_HEIGHT ||sourceHeight < OSD_INPUT_MIN_WIDTH)
        return false;
    return true;
}

int32_t OsdPlane::setPlane(std::shared_ptr<DrmFramebuffer> fb, uint32_t zorder, int blankOp) {
    MESON_ASSERT(mDrvFd >= 0, "osd plane fd is not valiable!");
    MESON_ASSERT(zorder > 0, "osd driver request zorder > 0");// driver request zorder > 0

    memset(&mPlaneInfo, 0, sizeof(mPlaneInfo));
    mPlaneInfo.magic         = OSD_SYNC_REQUEST_RENDER_MAGIC_V2;
    mPlaneInfo.len           = sizeof(osd_plane_info_t);
    mPlaneInfo.type          = DIRECT_COMPOSE_MODE;
    mPlaneInfo.zorder        = zorder;
    mPlaneInfo.shared_fd     = -1;
    mPlaneInfo.in_fen_fd     = -1;
    mPlaneInfo.out_fen_fd    = -1;

    bool bBlank = blankOp == UNBLANK ? false : true;
    if (!bBlank) {
        if (!fb) {
            MESON_LOGE("For osd plane unblank, the fb should not be null!");
            return 0;
        }

        drm_rect_t srcCrop       = fb->mSourceCrop;
        drm_rect_t disFrame      = fb->mDisplayFrame;
        buffer_handle_t buf      = fb->mBufferHandle;

        mPlaneInfo.xoffset       = srcCrop.left;
        mPlaneInfo.yoffset       = srcCrop.top;
        mPlaneInfo.width         = srcCrop.right    - srcCrop.left;
        mPlaneInfo.height        = srcCrop.bottom   - srcCrop.top;
        mPlaneInfo.dst_x         = disFrame.left;
        mPlaneInfo.dst_y         = disFrame.top;
        mPlaneInfo.dst_w         = disFrame.right   - disFrame.left;
        mPlaneInfo.dst_h         = disFrame.bottom  - disFrame.top;
        mPlaneInfo.blend_mode    = fb->mBlendMode;
        mPlaneInfo.op           |= OSD_BLANK_OP_BIT;

        if (fb->mBufferHandle != NULL) {
            mPlaneInfo.fb_width  = am_gralloc_get_width(fb->mBufferHandle);
            mPlaneInfo.fb_height = am_gralloc_get_height(fb->mBufferHandle);
        } else {
            mPlaneInfo.fb_width  = -1;
            mPlaneInfo.fb_height = -1;
        }

        if (fb->mFbType == DRM_FB_COLOR) {
            /*reset buffer layer info*/
            mPlaneInfo.shared_fd = -1;

            mPlaneInfo.dim_layer = 1;
              /*osd canot support plane alpha when ouput dim layer.
            *so we handle the plane on color here.
            */
            mPlaneInfo.dim_color = (((unsigned char)(fb->mColor.r * fb->mPlaneAlpha) << 24) |
                                                    ((unsigned char)(fb->mColor.g * fb->mPlaneAlpha) << 16) |
                                                    ((unsigned char)(fb->mColor.b * fb->mPlaneAlpha) << 8) |
                                                    ((unsigned char)(fb->mColor.a * fb->mPlaneAlpha)));
            mPlaneInfo.plane_alpha = 255;
            mPlaneInfo.afbc_inter_format = 0;
        } else  {
            //reset dim layer info.
            mPlaneInfo.dim_layer = 0;
            mPlaneInfo.dim_color = 0;

            mPlaneInfo.shared_fd     = ::dup(am_gralloc_get_buffer_fd(buf));
            mPlaneInfo.format        = am_gralloc_get_format(buf);
            mPlaneInfo.byte_stride   = am_gralloc_get_stride_in_byte(buf);
            mPlaneInfo.pixel_stride  = am_gralloc_get_stride_in_pixel(buf);
            mPlaneInfo.afbc_inter_format = am_gralloc_get_vpu_afbc_mask(buf);
            mPlaneInfo.plane_alpha   = (unsigned char)255 * fb->mPlaneAlpha; //kenrel need alpha 0 ~ 255

            /*
              OSD only handle premultiplied and coverage,
              So HWC set format to RGBX when blend mode is NONE.
            */
            if (mPlaneInfo.blend_mode == DRM_BLEND_MODE_NONE
                && mPlaneInfo.format == HAL_PIXEL_FORMAT_RGBA_8888) {
                mPlaneInfo.format = HAL_PIXEL_FORMAT_RGBX_8888;
            }
        }

        if (DebugHelper::getInstance().discardInFence()) {
            fb->getAcquireFence()->waitForever("osd-input");
            mPlaneInfo.in_fen_fd = -1;
        } else {
            mPlaneInfo.in_fen_fd     = fb->getAcquireFence()->dup();
        }
    } else {
        /*For nothing to display, post blank to osd which will signal the last retire fence.*/

        //Already set blank, return.
        if (mBlank == bBlank)
            return 0;

        mPlaneInfo.op &= ~(OSD_BLANK_OP_BIT);
    }
    mBlank = bBlank;

    if (ioctl(mDrvFd, FBIOPUT_OSD_SYNC_RENDER_ADD, &mPlaneInfo) != 0) {
        MESON_LOGE("osd plane FBIOPUT_OSD_SYNC_RENDER_ADD return(%d)", errno);
        return -EINVAL;
    }

    if (mDrmFb.get()) {
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
    if (bBlank)
        mDrmFb.reset();
    else
        mDrmFb = fb;
    return 0;
}

void OsdPlane::dump(String8 & dumpstr) {
    if (!mBlank) {
        dumpstr.appendFormat("| osd%2d |"
                " %4d | %4d | %4d %4d %4d %4d | %4d %4d %4d %4d | %2d | %2d | %4d |"
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
}

