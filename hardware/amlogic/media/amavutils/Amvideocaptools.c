/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#define LOG_TAG "AmAvutls"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <unistd.h>
#include <cutils/log.h>
#include <sys/ioctl.h>

#include "Amvideocaptools.h"


#define VIDEOCAPDEV "/dev/amvideocap0"
int amvideocap_capframe(char *buf, int size, int *w, int *h, int fmt_ignored __unused, int at_end, int* ret_size, int fmt)
{
    int fd = open(VIDEOCAPDEV, O_RDWR);
    int ret = 0;
    if (fd < 0) {
        ALOGI("amvideocap_capframe open %s failed\n", VIDEOCAPDEV);
        return -1;
    }
    if (w != NULL && *w > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH, *w);
    }
    if (h != NULL && *h > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, *h);
    }
    if (fmt) {
        ret = ioctl(fd, AMVIDEOCAP_IOW_SET_WANTFRAME_FORMAT, fmt);
    }
    if (at_end) {
        ret = ioctl(fd, AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS, CAP_FLAG_AT_END);
    }
    *ret_size = read(fd, buf, size);
    if (w != NULL) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_GET_FRAME_WIDTH, w);
    }
    if (h != NULL) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_GET_FRAME_HEIGHT, h);
    }
    close(fd);
    return ret;
}

int amvideocap_capframe_with_rect(char *buf, int size, int src_rect_x, int src_rect_y, int *w, int *h, int fmt_ignored __unused, int at_end, int* ret_size)
{
    int fd = open(VIDEOCAPDEV, O_RDWR);
    int ret = 0;
    if (fd < 0) {
        ALOGI("amvideocap_capframe_with_rect open %s failed\n", VIDEOCAPDEV);
        return -1;
    }
    ALOGI("amvideocap_capframe_with_rect open %d, %d\n", *w, *h);
    if (src_rect_x > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_SET_SRC_X, src_rect_x);
    }
    if (src_rect_y > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_SET_SRC_Y, src_rect_y);
    }
    if (*w > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_SET_SRC_WIDTH, *w);
    }
    if (*h > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_SET_SRC_HEIGHT, *h);
    }
    if (*w > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH, *w);
    }
    if (*h > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, *h);
    }
    if (at_end) {
        ret = ioctl(fd, AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS, CAP_FLAG_AT_END);
    }
    *ret_size = read(fd, buf, size);

    if (*w > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_GET_FRAME_WIDTH, w);
    }

    if (*h > 0) {
        ret = ioctl(fd, AMVIDEOCAP_IOR_GET_FRAME_HEIGHT, h);
    }
    close(fd);
    return ret;
}

