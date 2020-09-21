/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef AMLV4L_HEAD_A__
#define AMLV4L_HEAD_A__
#include <amvideo.h>

typedef struct amlv4l_dev {
    int v4l_fd;
    unsigned int bufferNum;
    vframebuf_t *vframe;
    int type;
    int width;
    int height;
    int pixformat;
    int memory_mode;
} amlv4l_dev_t;
amvideo_dev_t *new_amlv4l(void);
int amlv4l_release(amvideo_dev_t *dev);

#endif//AMLV4L_HEAD_A__