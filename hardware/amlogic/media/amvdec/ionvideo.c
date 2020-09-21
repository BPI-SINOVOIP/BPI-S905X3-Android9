/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ionvideo.h>
#include "ionvdec_priv.h"
#include "ionv4l.h"

ionvideo_dev_t *new_ionvideo(int flags)
{
    ionvideo_dev_t *dev = NULL;
    if (flags & FLAGS_V4L_MODE) {
        dev = new_ionv4l();
        if (dev) {
            dev->mode = FLAGS_V4L_MODE;
        }
    }
    return dev;
}
int ionvideo_setparameters(ionvideo_dev_t *dev, int cmd, void * parameters)
{
    return 0;
}
int ionvideo_getparameters(ionvideo_dev_t *dev, int *width, int *height, int *pixelformat)
{
    struct v4l2_format v4lfmt;
    int ret = 0;

    v4lfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (dev->ops.getparameters) {
        ret = dev->ops.getparameters(dev, &v4lfmt);
        if (ret) {
            return ret;
        }
        *width = v4lfmt.fmt.pix.width;
        *height = v4lfmt.fmt.pix.height;
        *pixelformat = v4lfmt.fmt.pix.pixelformat;
    }
    return 0;
}
int ionvideo_init(ionvideo_dev_t *dev, int flags, int width, int height, int fmt, int buffernum)
{
    int ret = -1;
    if (dev->ops.init) {
        ret = dev->ops.init(dev, O_RDWR | O_NONBLOCK, width, height, fmt, buffernum);
    }
    return ret;
}
int ionvideo_start(ionvideo_dev_t *dev)
{
    if (dev->ops.start) {
        return dev->ops.start(dev);
    }
    return 0;
}
int ionvideo_stop(ionvideo_dev_t *dev)
{
    if (dev->ops.stop) {
        return dev->ops.stop(dev);
    }
    return 0;
}
int ionvideo_release(ionvideo_dev_t *dev)
{
    if (dev->mode == FLAGS_V4L_MODE) {
        ionv4l_release(dev);
    }
    return 0;
}
int ionv4l_dequeuebuf(ionvideo_dev_t *dev, vframebuf_t*vf)
{
    if (dev->ops.dequeuebuf) {
        return dev->ops.dequeuebuf(dev, vf);
    }
    return -1;
}
int ionv4l_queuebuf(ionvideo_dev_t *dev, vframebuf_t*vf)
{
    if (dev->ops.queuebuf) {
        return dev->ops.queuebuf(dev, vf);
    }
    return 0;
}
