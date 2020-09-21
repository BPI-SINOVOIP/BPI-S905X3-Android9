/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CDevicePollCheck"

#include "CDevicePollCheckThread.h"

CDevicePollCheckThread::CDevicePollCheckThread()
{
    mpObserver = NULL;
    if (mEpoll.create() < 0) {
        SYS_LOGE("create epoll fail\n");
        return;
    }

    //VFrameSize change
    if (mVFrameSizeFile.openFile(VFRAME_MOUDLE_PATH) > 0) {
        m_event.data.fd = mVFrameSizeFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mVFrameSizeFile.getFd(), &m_event);
    }
    //HDR
    if (mHDRStatusFile.openFile(HDR_MOUDLE_PATH) > 0) {
        m_event.data.fd = mHDRStatusFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mHDRStatusFile.getFd(), &m_event);
        HDR_fd = mHDRStatusFile.getFd();
    }

    //TX
    if (mTXStatusFile.openFile(TX_MOUDLE_PATH) > 0) {
        m_event.data.fd = mTXStatusFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mTXStatusFile.getFd(), &m_event);
    }
}

CDevicePollCheckThread::~CDevicePollCheckThread()
{

}

int CDevicePollCheckThread::StartCheck()
{
    this->run("CDevicePollCheckThread");
    return 0;
}

bool CDevicePollCheckThread::threadLoop()
{
    if ( mpObserver == NULL ) {
        return false;
    }

    prctl(PR_SET_NAME, (unsigned long)"CDevicePollCheckThread thread loop");

    while (!exitPending()) { //requietexit() or requietexitWait() not call
        int num = mEpoll.wait();
        for (int i = 0; i < num; ++i) {
            int fd = (mEpoll)[i].data.fd;
            /**
             * EPOLLIN event
             */
            if ((mEpoll)[i].events & EPOLLIN) {
                if (fd == mVFrameSizeFile.getFd()) {//vframesize change
                    mpObserver->onVframeSizeChange();
                } else if (fd == mHDRStatusFile.getFd()) {//HDR
                    mpObserver->onHDRStatusChange();
                } else if (fd == mTXStatusFile.getFd()) {//TX
                    mpObserver->onTXStatusChange();
                }

                if ((mEpoll)[i].events & EPOLLOUT) {

                }
            }
        }
    }//exit

    SYS_LOGD("exit CDevicePollCheckThread!\n");
    return false;
}

