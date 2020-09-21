/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "ionv4l.h"
#include "ionvdec_priv.h"
#define V4LDEVICE_NAME  "/dev/video13"
#define CLEAR(s) memset(&s, 0, sizeof(s))

static int ionv4l_unmapbufs(ionvideo_dev_t *dev);
static int ionv4l_mapbufs(ionvideo_dev_t *dev);
int ionv4l_setfmt(ionvideo_dev_t *dev, struct v4l2_format *fmt);
int ionv4l_stop(ionvideo_dev_t *dev);
int ionv4l_init(ionvideo_dev_t *dev, int type, int width, int height, int fmt, int buffernum)
{
    int ret;
    ionv4l_dev_t *v4l = dev->devpriv;
    struct v4l2_format v4lfmt;
    v4l->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l->width = width;
    v4l->height = height;
    v4l->pixformat = fmt;
    v4l->buffer_num = buffernum;
    v4lfmt.type = v4l->type;
    v4lfmt.fmt.pix.width = v4l->width;
    v4lfmt.fmt.pix.height = v4l->height;
    v4lfmt.fmt.pix.pixelformat = v4l->pixformat;
    ret = ionv4l_setfmt(dev, &v4lfmt);
    if (ret != 0) {
        goto error_out;
    }
    ret = ionv4l_mapbufs(dev);
error_out:
    return ret;
}

static int ionv4l_ioctl(ionvideo_dev_t *dev, int request, void *arg)
{
    int ret;
    ionv4l_dev_t *v4l = dev->devpriv;
    ret = ioctl(v4l->v4l_fd, request, arg);
    if (ret == -1 && errno) {
        ret = -errno;
    }
    return ret;
}

int ionv4l_release(ionvideo_dev_t *dev)
{
    int ret = -1;
    ionv4l_dev_t *v4l = dev->devpriv;
    if (v4l->v4l_fd < 0) {
        return 0;
    }
    ionv4l_stop(dev);
    ionv4l_unmapbufs(dev);
    if (v4l->v4l_fd >= 0) {
        ret = close(v4l->v4l_fd);
    }
    v4l->v4l_fd = -1;
    free(dev);
    if (ret == -1 && errno) {
        ret = -errno;
    }

    return ret;
}

int ionv4l_dequeue_buf(ionvideo_dev_t *dev, vframebuf_t *vf)
{
    struct v4l2_buffer vbuf;
    CLEAR(vbuf);
    int ret;
    ionv4l_dev_t *v4l = dev->devpriv;
    vbuf.type = v4l->type;
    vbuf.memory = v4l->memory_mode;
    vbuf.length = vf->length;
    ret = ionv4l_ioctl(dev, VIDIOC_DQBUF, &vbuf);
    if (!ret && vbuf.index < v4l->buffer_num) {
        vf->pts = vbuf.timestamp.tv_sec & 0xFFFFFFFF;
        vf->pts <<= 32;
        vf->pts += vbuf.timestamp.tv_usec & 0xFFFFFFFF;
        vf->fd = vbuf.m.fd;
        vf->index = vbuf.index;
    }

    return ret;
}

int ionv4l_queue_buf(ionvideo_dev_t *dev, vframebuf_t *vf)
{
    struct v4l2_buffer vbuf;
    CLEAR(vbuf);
    int ret;
    ionv4l_dev_t *v4l = dev->devpriv;
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_DMABUF;
    vbuf.index = vf->index;
    vbuf.m.fd = vf->fd;
    vbuf.length = vf->length;
    return ionv4l_ioctl(dev, VIDIOC_QBUF, &vbuf);
}

int ionv4l_start(ionvideo_dev_t *dev)
{
    int type;
    ionv4l_dev_t *v4l = dev->devpriv;
    type = v4l->type;
    return ionv4l_ioctl(dev, VIDIOC_STREAMON, &type);
}

int ionv4l_stop(ionvideo_dev_t *dev)
{
    int type;
    ionv4l_dev_t *v4l = dev->devpriv;
    type = v4l->type;
    return ionv4l_ioctl(dev, VIDIOC_STREAMOFF, &type);
}

int ionv4l_setfmt(ionvideo_dev_t *dev, struct v4l2_format *fmt)
{
    return ionv4l_ioctl(dev, VIDIOC_S_FMT, fmt);
}

int ionv4l_getfmt(ionvideo_dev_t *dev, struct v4l2_format *fmt)
{
    return ionv4l_ioctl(dev, VIDIOC_G_FMT, fmt);
}

static int ionv4l_unmapbufs(ionvideo_dev_t *dev)
{
    return 0;
}

static int ionv4l_mapbufs(ionvideo_dev_t *dev)
{
    int ret;
    struct v4l2_requestbuffers rb;
    CLEAR(rb);
    ionv4l_dev_t *v4l = dev->devpriv;
    rb.count = v4l->buffer_num;
    rb.type = v4l->type;
    rb.memory = v4l->memory_mode;
    return ionv4l_ioctl(dev, VIDIOC_REQBUFS, &rb);
}

ionvideo_dev_t *new_ionv4l(void)
{
    ionvideo_dev_t *dev;
    ionv4l_dev_t *v4l;
    dev = malloc(sizeof(ionvideo_dev_t) + sizeof(ionv4l_dev_t));
    memset(dev, 0, sizeof(ionvideo_dev_t) + sizeof(ionv4l_dev_t));
    dev->devpriv = (void *)((long)(&dev->devpriv) + 4);
    v4l = dev->devpriv;
    v4l->memory_mode = V4L2_MEMORY_DMABUF;
    dev->ops.init = ionv4l_init;
    dev->ops.release = ionv4l_release;
    dev->ops.dequeuebuf = ionv4l_dequeue_buf;
    dev->ops.queuebuf = ionv4l_queue_buf;
    dev->ops.start = ionv4l_start;
    dev->ops.stop = ionv4l_stop;
    dev->ops.getparameters = ionv4l_getfmt;
    v4l->v4l_fd = open(V4LDEVICE_NAME, O_RDWR | O_NONBLOCK);
    if (v4l->v4l_fd < 0) {
        free(dev);
        LOGE("v4l device opend failed!,%s(%d)\n", strerror(errno), errno);
        return NULL;
    }
    return dev;
}

