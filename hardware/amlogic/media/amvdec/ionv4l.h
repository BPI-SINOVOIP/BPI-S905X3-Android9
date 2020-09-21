/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef IONV4L_HEAD_A__
#define IONV4L_HEAD_A__
#include <ionvideo.h>

typedef struct ionv4l_dev {
    int v4l_fd;
    unsigned int buffer_num;
    vframebuf_t *vframe;
    int type;
    int width;
    int height;
    int pixformat;
    int memory_mode;
} ionv4l_dev_t;
ionvideo_dev_t *new_ionv4l(void);
int ionv4l_release(ionvideo_dev_t *dev);

#endif//IONV4L_HEAD_A__
