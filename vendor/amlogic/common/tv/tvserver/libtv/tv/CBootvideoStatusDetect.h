/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef C_BOOT_VIDEO_STATUS_DETECT_H
#define C_BOOT_VIDEO_STATUS_DETECT_H
#include <utils/Thread.h>

using namespace android;
class CBootvideoStatusDetect: public Thread {
public:
    CBootvideoStatusDetect();
    ~CBootvideoStatusDetect();
    int startDetect();
    int stopDetect();
    bool isBootvideoStopped();

    class IBootvideoStatusObserver {
    public:
        IBootvideoStatusObserver() {};
        virtual ~IBootvideoStatusObserver() {};
        virtual void onBootvideoRunning() {};
        virtual void onBootvideoStopped() {};
    };
    void setObserver (IBootvideoStatusObserver *pOb)
    {
        mpObserver = pOb;
    };

private:
    bool threadLoop();

    IBootvideoStatusObserver *mpObserver;
    bool mIsRunning;
};
#endif

