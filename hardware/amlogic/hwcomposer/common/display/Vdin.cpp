/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <Vdin.h>
#include <MesonLog.h>
#include <HwDisplayCrtc.h>
#include "AmVinfo.h"

ANDROID_SINGLETON_STATIC_INSTANCE(Vdin)

#define VDIN1_DEV "/dev/vdin1"
#define POLL_TIMEOUT_MS (20)

#define _TM_T 'T'
#define TVIN_IOC_S_VDIN_V4L2START  _IOW(_TM_T, 0x25, struct vdin_v4l2_param_s)
#define TVIN_IOC_S_VDIN_V4L2STOP   _IO(_TM_T, 0x26)

#define TVIN_IOC_S_CANVAS_ADDR  _IOW(_TM_T, 0x4f, struct vdin_set_canvas_s)
#define TVIN_IOC_S_CANVAS_RECOVERY  _IO(_TM_T, 0x0a)

Vdin::Vdin() {
    mDev = open(VDIN1_DEV, O_RDONLY);
    MESON_ASSERT(mDev >= 0, "Vdin device open fail.");
    mStatus = STREAMING_STOP;
    mCanvasCnt = 0;
}

Vdin::~Vdin() {
    stop();
    releaseCanvas();
    close(mDev);
}

int32_t Vdin::getStreamInfo(int & width, int & height, int & format) {
    /*read current */
    struct vinfo_base_s info;
    if (read_vout_info(CRTC_VOUT1, &info) != 0) {
        MESON_LOGE("getStreamInfo faild.");
        mCapParams.width = 1920;
        mCapParams.height = 1080;
        mCapParams.fps = 60;
        mDefFormat = HAL_PIXEL_FORMAT_RGB_888;
    } else {
        mCapParams.width = info.width;
        mCapParams.height = info.height;
        mCapParams.fps = (int)info.sync_duration_num/info.sync_duration_den;

        switch (info.viu_color_fmt) {
            case TVIN_RGB444:
                mDefFormat = HAL_PIXEL_FORMAT_RGB_888;
                break;
            case TVIN_NV21:
                mDefFormat = HAL_PIXEL_FORMAT_YCRCB_420_SP;
                break;
            case TVIN_YUV422:
                mDefFormat = HAL_PIXEL_FORMAT_YCBCR_422_SP;
                break;
            default:
                MESON_ASSERT(0,"Not supported format.");
        };
    }

    width = mCapParams.width;
    height = mCapParams.height;
    format = mDefFormat;
    return 0;
}

int32_t Vdin::setStreamInfo(int  format, int bufCnt) {
    UNUSED(format);

    if (mStatus != STREAMING_STOP) {
        MESON_LOGE("Cannot setStreamInfo when do streaming.");
        return -EINVAL;
    }

    releaseCanvas();
    createCanvas(bufCnt);
    return 0;
}

void Vdin::createCanvas(int bufCnt) {
    mCanvasCnt = bufCnt;
    for (int i = 0;i < VDIN_CANVAS_MAX_CNT; i++) {
        mCanvas[i].fd = -1;
        mCanvas[i].index = -1;
    }
}

void Vdin::releaseCanvas() {
    if (mCanvasCnt <= 0)
        return ;

    for (int i = 0;i < mCanvasCnt; i++) {
        if (mCanvas[i].fd >= 0)
            close(mCanvas[i].fd);
        mCanvas[i].fd = -1;
    }

    mCanvasCnt = 0;
}

int32_t Vdin::queueBuffer(std::shared_ptr<DrmFramebuffer> & fb, int idx) {
    MESON_ASSERT(idx < VDIN_CANVAS_MAX_CNT, "queueBuffer idx is invalid.");

    if (mStatus == STREAMING_STOP) {
        MESON_ASSERT(fb.get() != NULL, "init queue fb should not be null.");
        mCanvas[idx].index = idx;
        mCanvas[idx].fd = ::dup(am_gralloc_get_buffer_fd(fb->mBufferHandle));
        MESON_LOGD("Vdin::queue new Buffer %d - %d", idx, mCanvas[idx].fd);
    } else {
        /*TODO: cannot queue back specific buffer, just return the last buffer.*/
        ioctl(mDev, TVIN_IOC_S_CANVAS_RECOVERY, &idx);
    }
    return 0;
}

int32_t Vdin::dequeueBuffer(int & idx) {
    struct pollfd fds[1];
    fds[0].fd = mDev;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    int dequeuIdx = -1;
    int pollrtn = poll(fds, 1, POLL_TIMEOUT_MS);
    if (pollrtn > 0 && fds[0].revents == POLLIN) {
        if(read(mDev, &dequeuIdx, sizeof(int)) > 0) {
            idx = dequeuIdx;
        }
    }

    if (dequeuIdx < 0)
        return -EFAULT;

    return 0;
}

int32_t Vdin::start() {
    if (mStatus == STREAMING_START)
        return 0;

    MESON_ASSERT(mCanvas != NULL, "Canvas should set before streaming.");

    if (ioctl(mDev, TVIN_IOC_S_CANVAS_ADDR, mCanvas) < 0) {
        int rtn = -errno;
        MESON_ASSERT(0, "TVIN_IOC_S_CANVAS_ADDR failed (%d) ", rtn);
        return rtn;
    }

    /*all fd passed to drv, reset to -1.*/
    for (int i = 0;i < mCanvasCnt; i++)
        mCanvas[i].fd = -1;

    if (ioctl(mDev, TVIN_IOC_S_VDIN_V4L2START, &mCapParams) < 0) {
        int rtn = -errno;
        MESON_ASSERT(0, "Vdin start ioctl failed (%d) ", rtn);
        return rtn;
    }

    mStatus = STREAMING_START;
    return 0;
}

int32_t Vdin::pause() {
    if (ioctl(mDev, TVIN_IOC_S_VDIN_V4L2STOP, &mCapParams) < 0) {
        int rtn = -errno;
        MESON_ASSERT(0, "Vdin start ioctl failed (%d) ", rtn);
        return rtn;
    }

    mStatus = STREAMING_PAUSE;
    return 0;
}

int32_t Vdin::stop() {
    if (STREAMING_STOP == mStatus)
        return 0;

    if (mStatus == STREAMING_START) {
        pause();
    }

    releaseCanvas();
    mStatus = STREAMING_STOP;
    return 0;
}

