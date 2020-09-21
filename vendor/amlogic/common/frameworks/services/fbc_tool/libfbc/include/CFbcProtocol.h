/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_FBC_PROTOCOL_H_
#define _C_FBC_PROTOCOL_H_

#include <sys/time.h>

#include "CFbcCommand.h"
#include "CSerialPort.h"
#include <utils/Thread.h>
#include "CVirtualInput.h"
#include "CFbcEpoll.h"

using namespace android;
typedef struct REQUEST_REPLY_CMD {
    Condition WaitReplyCondition;
    int WaitDevNo;
    int WaitCmd;
    int WaitTimeOut;
    unsigned char *replyData;
    int reDataLen;
} REQUEST_REPLY_S;

class CFbcProtocol: public Thread {
public:
    CFbcProtocol();
    ~CFbcProtocol();
    int start(unsigned int baud_rate);
    void testUart();
    int handleCmd(COMM_DEV_TYPE_E fromDev, int cmd[], int *pRetValue);
    int fbcGetBatchValue(COMM_DEV_TYPE_E toDev, unsigned char *cmd_buf, int count);
    int fbcSetBatchValue(COMM_DEV_TYPE_E fromDev, unsigned char *cmd_buf, int count);
    int sendDataOneway(int devno, unsigned char *pData, int dataLen, int flags);
    int sendDataAndWaitReply(int devno, int waitForDevno, int waitForCmd, unsigned char *pData, int dataLen, int timeout, unsigned char *pReData, int *reDataLen, int flags);
    int closeAll();
    int SetUpgradeFlag(int flag);
    int uartReadStream(unsigned char *retData, int rd_max_len, int timeout);
    unsigned int Calcrc32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len);

private:
    void showTime(struct timeval *_time);
    long getTime(void);
    void initCrc32Table();
    void sendAckCmd(bool isOk);
    unsigned int GetCrc32(unsigned char *InStr, unsigned int len);
    int processData(COMM_DEV_TYPE_E fromDev, unsigned char *PData, int dataLen);
    int uartReadData(unsigned char *retData, int *retLen);
    bool threadLoop();

    int mUpgradeFlag;
    CFile mFbcFile;
    CSerialPort mSerialPort;
    FbcEpoll mFbcEpoll;
    mutable Mutex mLock;
    REQUEST_REPLY_S mReplyList;
    //list
    epoll_event m_event;
    unsigned char *mpRevDataBuf;
    unsigned int mCrc32Table[256];
    bool mbSendKeyCode;
    CVirtualInput mCVirtualInput;
    int mbDownHaveSend;
    int mbFbcKeyEnterDown;
    nsecs_t mFbcEnterKeyDownTime;
};

extern CFbcProtocol *GetFbcProtocolInstance(unsigned int baud_rate = 115200);
extern void ResetFbcProtocolInstance();

#endif
