/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#include <utils/Thread.h>
#include <utils/Vector.h>
using namespace android;
#if !defined(_C_MSG_QUEUE_H)
#define _C_MSG_QUEUE_H

class CMessage {
public:
    CMessage();
    ~CMessage();
    nsecs_t mDelayMs;//delay times , MS
    nsecs_t mWhenMs;//when, the msg will handle
    int mType;
    void *mpData;
    unsigned char mpPara[5120];
};

class CMsgQueueThread: public Thread {
public:
    CMsgQueueThread();
    virtual ~CMsgQueueThread();
    int startMsgQueue();
    void sendMsg(CMessage &msg);
    void removeMsg(CMessage &msg);
    void clearMsg();
private:
    bool  threadLoop();
    nsecs_t getNowMs();//get system time , MS
    virtual void handleMessage(CMessage &msg) = 0;

    //
    Vector<CMessage> m_v_msg;
    Condition mGetMsgCondition;
    mutable Mutex mLockQueue;
};

/*class CHandler
{
    pubulic:
    CHandler(CMsgQueueThread& msgQueue);
    ~CHandler();
    void sendMsg();
    void removeMsg();
    private:
    virtual void handleMessage(CMessage &msg);
};*/

#endif
