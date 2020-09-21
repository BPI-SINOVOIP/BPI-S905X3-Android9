/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <sys/mman.h>
#include <misc.h>

#include "CursorPlane.h"

#define DEFAULT_CUSOR_SIZE (256)
#define CUSOR_BPP (4)

CursorPlane::CursorPlane(int32_t drvFd, uint32_t id)
    : HwDisplayPlane(drvFd, id),
      mLastTransform(0),
      mBlank(true),
      mDrmFb(NULL) {
    snprintf(mName, 64, "CURSOR-%d", id);

    /*call mmap here to let osd alloc buffer from ion*/
    updatePlaneInfo(DEFAULT_CUSOR_SIZE, DEFAULT_CUSOR_SIZE);
    void *cbuffer =
        mmap(NULL, mPlaneInfo.fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, mDrvFd, 0);
    if (cbuffer != MAP_FAILED) {
        munmap(cbuffer, mPlaneInfo.fbSize);
    } else {
        MESON_LOGE("Cursor plane buffer mmap fail!");
    }
}

CursorPlane::~CursorPlane() {

}

const char * CursorPlane::getName() {
    return mName;
}

uint32_t CursorPlane::getPlaneType() {
    return CURSOR_PLANE;
}

uint32_t CursorPlane::getCapabilities() {
    return 0;
};

int32_t CursorPlane::getFixedZorder() {
    return CURSOR_PLANE_FIXED_ZORDER;
}

uint32_t CursorPlane::getPossibleCrtcs() {
    return CRTC_VOUT1;
}

bool CursorPlane::isFbSupport(std::shared_ptr<DrmFramebuffer> & fb) {
    if (fb->mFbType == DRM_FB_CURSOR &&
        am_gralloc_get_format(fb->mBufferHandle) == HAL_PIXEL_FORMAT_RGBA_8888) {
        return true;
    }

    return false;
}

int32_t CursorPlane::setPlane(
    std::shared_ptr<DrmFramebuffer> fb,
    uint32_t zorder __unused, int blankOp) {
    if (mDrvFd < 0) {
        MESON_LOGE("cursor plane fd is not valiable!");
        return -EBADF;
    }

    if (fb) {
        drm_rect_t disFrame      = fb->mDisplayFrame;
        buffer_handle_t buf      = fb->mBufferHandle;

        /* osd request plane zorder > 0 */
        mPlaneInfo.zorder        = fb->mZorder + 1;
        mPlaneInfo.transform     = fb->mTransform;
        mPlaneInfo.dst_x         = disFrame.left;
        mPlaneInfo.dst_y         = disFrame.top;
        mPlaneInfo.format        = am_gralloc_get_format(buf);
        mPlaneInfo.shared_fd     = am_gralloc_get_buffer_fd(buf);
        mPlaneInfo.stride        = am_gralloc_get_stride_in_pixel(buf);
        mPlaneInfo.buf_w         = am_gralloc_get_width(buf);
        mPlaneInfo.buf_h         = am_gralloc_get_height(buf);

        updateCursorBuffer(fb);
        setCursorPosition(mPlaneInfo.dst_x, mPlaneInfo.dst_y);
    }

    bool bBlank = blankOp == UNBLANK ? false : true;
    if (mBlank != bBlank) {
        uint32_t val = bBlank ? 1 : 0;
        if (ioctl(mDrvFd, FBIOPUT_OSD_SYNC_BLANK, &val) != 0) {
            MESON_LOGE("cursor plane blank ioctl (%d) return(%d)", bBlank, errno);
            return -EINVAL;
        }
        mBlank = bBlank;
    }

    mDrmFb = fb;
    return 0;
}

int32_t CursorPlane::updateCursorBuffer(
    std::shared_ptr<DrmFramebuffer> & fb) {
    int cbwidth =
        HWC_ALIGN(CUSOR_BPP * mPlaneInfo.stride, CUSOR_BPP * 8) / CUSOR_BPP;

    if (mPlaneInfo.info.xres != (uint32_t)cbwidth ||
        mPlaneInfo.info.yres != (uint32_t)mPlaneInfo.buf_h) {
        updatePlaneInfo(cbwidth, mPlaneInfo.buf_h);
        void *cbuffer =
            mmap(NULL, mPlaneInfo.fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, mDrvFd, 0);
        if (cbuffer != MAP_FAILED) {
            memset(cbuffer, 1, mPlaneInfo.fbSize);

            /*copy to dev buffer*/
            unsigned char *base = NULL;
            if (0 == gralloc_lock_dma_buf(fb->mBufferHandle, (void **)&base)) {
                char* cpyDst = (char*)cbuffer;
                char* cpySrc = (char*)base;
                for (int irow = 0; irow < mPlaneInfo.buf_h; irow++) {
                    memcpy(cpyDst, cpySrc, CUSOR_BPP * mPlaneInfo.buf_w);
                    cpyDst += CUSOR_BPP * cbwidth;
                    cpySrc += CUSOR_BPP * mPlaneInfo.stride;
                }
                gralloc_unlock_dma_buf(fb->mBufferHandle);
            }

            munmap(cbuffer, mPlaneInfo.fbSize);
            MESON_LOGV("setCursor ok");
        } else {
            MESON_LOGE("Cursor plane buffer mmap fail!");
            return -EBADF;
        }
    }

    return 0;
}

int32_t CursorPlane::updatePlaneInfo(int xres, int yres) {
    struct fb_fix_screeninfo finfo;
    if (ioctl(mDrvFd, FBIOGET_FSCREENINFO, &finfo) != 0)
        return -EINVAL;

    struct fb_var_screeninfo info;
    if (ioctl(mDrvFd, FBIOGET_VSCREENINFO, &info) != 0)
        return -EINVAL;

    MESON_LOGI("vinfo. %d %d", info.xres, info.yres);

    info.xoffset = info.yoffset = 0;
    info.bits_per_pixel = 32;

    /*
    * Explicitly request 8/8/8/8
    */
    info.bits_per_pixel = 32;
    info.red.offset     = 0;
    info.red.length     = 8;
    info.green.offset   = 8;
    info.green.length   = 8;
    info.blue.offset    = 16;
    info.blue.length    = 8;
    info.transp.offset  = 24;
    info.transp.length  = 8;

    info.xres_virtual = info.xres = xres;
    info.yres_virtual = info.yres = yres;

    if (ioctl(mDrvFd, FBIOPUT_VSCREENINFO, &info) != 0)
        return -EINVAL;

    if (ioctl(mDrvFd, FBIOGET_VSCREENINFO, &info) != 0)
        return -EINVAL;

    if (int(info.width) <= 0 || int(info.height) <= 0)
    {
        // the driver doesn't return that information
        // default to 160 dpi
        info.width  = ((info.xres * 25.4f)/160.0f + 0.5f);
        info.height = ((info.yres * 25.4f)/160.0f + 0.5f);
    }

    if (ioctl(mDrvFd, FBIOGET_FSCREENINFO, &finfo) != 0)
        return -EINVAL;

    MESON_LOGI("using (fd=%d)\n"
        "id           = %s\n"
        "xres         = %d px\n"
        "yres         = %d px\n"
        "xres_virtual = %d px\n"
        "yres_virtual = %d px\n"
        "bpp          = %d\n",
        mDrvFd,
        finfo.id,
        info.xres,
        info.yres,
        info.xres_virtual,
        info.yres_virtual,
        info.bits_per_pixel);

    MESON_LOGI("width        = %d mm \n"
        "height       = %d mm \n",
        info.width,
        info.height);

    if (finfo.smem_len <= 0)
        return -EBADF;

    mPlaneInfo.info = info;
    mPlaneInfo.finfo = finfo;
    MESON_LOGD("updatePlaneInfo: finfo.line_length is 0x%x,info.yres_virtual is 0x%x",
                    finfo.line_length, info.yres_virtual);
    mPlaneInfo.fbSize = HWC_ALIGN(finfo.line_length * info.yres_virtual, PAGE_SIZE);
    return 0;
}

int32_t CursorPlane::setCursorPosition(int32_t x, int32_t y) {
    fb_cursor cinfo;
    int32_t transform = mPlaneInfo.transform;
    if (mLastTransform != transform) {
        MESON_LOGD("setCursorPosition: mLastTransform: %d, transform: %d.",
                    mLastTransform, transform);
        int arg = 0;
        switch (transform) {
            case HAL_TRANSFORM_ROT_90:
                arg = 2;
            break;
            case HAL_TRANSFORM_ROT_180:
                arg = 1;
            break;
            case HAL_TRANSFORM_ROT_270:
                arg = 3;
            break;
            default:
                arg = 0;
            break;
        }
        if (ioctl(mDrvFd, FBIOPUT_OSD_REVERSE, arg) != 0)
            MESON_LOGE("set cursor reverse ioctl return(%d)", errno);
        mLastTransform = transform;
    }
    cinfo.hot.x = x;
    cinfo.hot.y = y;
    MESON_LOGI("setCursorPosition x_pos=%d, y_pos=%d", cinfo.hot.x, cinfo.hot.y);
    if (ioctl(mDrvFd, FBIOPUT_OSD_CURSOR, &cinfo) != 0)
        MESON_LOGE("set cursor position ioctl return(%d)", errno);

    return 0;
}

void CursorPlane::dump(String8 & dumpstr) {
    if (!mBlank) {
        dumpstr.appendFormat("  osd%1d | %3d |\n",
                 mId - 30,
                 mPlaneInfo.zorder);
    }
}

