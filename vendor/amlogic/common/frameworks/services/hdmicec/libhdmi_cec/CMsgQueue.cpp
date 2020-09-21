/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "hdmicecd"
#define LOG_TV_TAG "CMessage"

#include "CMsgQueue.h"
#include <HdmiCecBase.h>
#include <utils/Timers.h>

CMessage::CMessage()
{
    mDelayMs = 0;
    mWhenMs = 0;
}

CMessage::~CMessage()
{
}

CMsgQueueThread::CMsgQueueThread()
{
}

CMsgQueueThread::~CMsgQueueThread()
{
    requestExitAndWait();
}

nsecs_t CMsgQueueThread::getNowMs()
{
    return systemTime(SYSTEM_TIME_MONOTONIC) / 1000000;
}

void CMsgQueueThread::sendMsg(CMessage &msg)
{
    AutoMutex _l(mLockQueue);
    msg.mWhenMs = getNowMs() + msg.mDelayMs;//
    int i = 0;
    while (i < (int)m_v_msg.size() && m_v_msg[i].mWhenMs <= msg.mWhenMs) i++; //find the index that will insert(i)
    m_v_msg.insertAt(msg, i);//insert at index i
    CMessage msg1 = m_v_msg[0];
    LOGD("sendmsg now = %lld msg[0] when = %lld", getNowMs(), msg1.mWhenMs);
    //
    //if(i == 0)// is empty or new whenMS  is  at  index 0, low all ms in list.  so ,need to  wakeup loop,  to get new delay time.
    mGetMsgCondition.signal();
}

void CMsgQueueThread::removeMsg(CMessage &msg)
{
    AutoMutex _l(mLockQueue);
    int beforeSize = (int)m_v_msg.size();
    for (int i = 0; i < (int)m_v_msg.size(); i++) {
        const CMessage &_msg = m_v_msg.itemAt(i);
        if (_msg.mType == msg.mType) {
            m_v_msg.removeAt(i);
        }
    }
    //some msg removeed
    if (beforeSize > (int)m_v_msg.size())
        mGetMsgCondition.signal();
}

void CMsgQueueThread::clearMsg()
{
    AutoMutex _l(mLockQueue);
    m_v_msg.clear();
}

int CMsgQueueThread::startMsgQueue()
{
    AutoMutex _l(mLockQueue);
    this->run("CMsgQueueThread start to run");
    return 0;
}

bool CMsgQueueThread::threadLoop()
{
    int sleeptime = 100;//ms
    while (!exitPending()) { //requietexit() or requietexitWait() not call
        mLockQueue.lock();
        while (m_v_msg.size() == 0) { //msg queue is empty
            mGetMsgCondition.wait(mLockQueue);//first unlock,when return,lock again,so need,call unlock
        }
        mLockQueue.unlock();
        //get delay time
        CMessage msg;
        nsecs_t delayMs = 0, nowMS = 0;
        do { //wait ,until , the lowest time msg's whentime is low nowtime, to go on
            if (m_v_msg.size() <= 0) {
                LOGD("msg size is 0, break");
                break;
            }
            mLockQueue.lock();//get msg ,first lock.
            msg = m_v_msg[0];//get first
            mLockQueue.unlock();

            delayMs = msg.mWhenMs - getNowMs();
            if (delayMs > 0) {
                mLockQueue.lock();//get msg ,first lock.
                int ret = mGetMsgCondition.waitRelative(mLockQueue, delayMs);
                mLockQueue.unlock();
            } else {
                break;
            }
        } while (true); //msg[0], timeout

        if (m_v_msg.size() > 0) {
            mLockQueue.lock();//
            msg = m_v_msg[0];
            m_v_msg.removeAt(0);
            mLockQueue.unlock();//
            //handle it
            handleMessage(msg);
        }

        //usleep(sleeptime * 1000);
    }
    //exit
    //return true, run again, return false,not run.
    return false;
}
