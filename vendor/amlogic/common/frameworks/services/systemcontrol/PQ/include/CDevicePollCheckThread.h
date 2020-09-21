/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef C_DEVICE_POLL_CHECK_H
#define C_DEVICE_POLL_CHECK_H

#include <CEpoll.h>
#include <CFile.h>
#include "CPQLog.h"
#include <utils/Thread.h>
#include <sys/prctl.h>
#include <fcntl.h>

#define VFRAME_MOUDLE_PATH    "/dev/amvideo_poll"
#define HDR_MOUDLE_PATH       "/dev/amvecm"
#define TX_MOUDLE_PATH        "/dev/display"

using namespace android;
class CDevicePollCheckThread: public Thread {
public:
    CDevicePollCheckThread();
    ~CDevicePollCheckThread();
    int StartCheck();

    class IDevicePollCheckObserver {
    public:
        IDevicePollCheckObserver() {};
        virtual ~IDevicePollCheckObserver() {};
        virtual void onVframeSizeChange() {};
        virtual void onHDRStatusChange() {};
        virtual void onTXStatusChange() {};
    };

    void setObserver ( IDevicePollCheckObserver *pOb ) {
        mpObserver = pOb;
    };

    int HDR_fd;

private:
    bool threadLoop();

    IDevicePollCheckObserver *mpObserver;
    CEpoll       mEpoll;
    mutable Mutex mLock;
    epoll_event m_event;
    CFile mVFrameSizeFile;
    CFile mHDRStatusFile;
    CFile mTXStatusFile;
};

#endif
