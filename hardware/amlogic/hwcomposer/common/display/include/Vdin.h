/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef VDIN_H
#define VDIN_H

#include <DrmTypes.h>
#include <BasicTypes.h>
#include <utils/threads.h>
#include <DrmFramebuffer.h>

#define VDIN_CANVAS_MAX_CNT	 9

struct vdin_v4l2_param_s {
    int width;
    int height;
    int fps;
};

struct vdin_set_canvas_s {
    int fd;
    int index;
};

/*Vdin1 used to capture data from vout0.*/
class Vdin
    :   public android::Singleton<Vdin> {
public:
    Vdin();
    ~Vdin();

    /*read back size is defined acorrding to the displaymode*/
    int32_t getStreamInfo(int & width, int & height, int & format);
    int32_t setStreamInfo(int  format, int bufCnt);

    int32_t queueBuffer(std::shared_ptr<DrmFramebuffer> & fb, int idx);
    int32_t dequeueBuffer(int & idx);

    /*should call queueBuffer() before start.*/
    int32_t start();
    /*only stop capture, and you can call start to resume it again.*/
    int32_t pause();
    /*stop capture, and clear info. Need setStreamInfo again before start.*/
    int32_t stop();

protected:
    void createCanvas(int bufCnt);
    void releaseCanvas();

protected:
    enum {
        STREAMING_START = 0,
        STREAMING_PAUSE,
        STREAMING_STOP,
    };

protected:
    int32_t mDev;
    int mStatus;

    int mDefFormat;
    vdin_v4l2_param_s mCapParams;

    int mCanvasCnt;
    struct vdin_set_canvas_s mCanvas[VDIN_CANVAS_MAX_CNT];
};

#endif
