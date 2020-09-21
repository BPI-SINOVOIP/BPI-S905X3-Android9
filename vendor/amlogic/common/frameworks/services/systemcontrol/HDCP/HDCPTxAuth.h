/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process HDCP TX authenticate
 */

#ifndef _HDCP_TX_AUTHENTICATION_H
#define _HDCP_TX_AUTHENTICATION_H

#include <map>
#include <cmath>
#include <string>
#include <pthread.h>
#include <semaphore.h>

#include "../common.h"
#include "../SysWrite.h"
#include "../FrameRateAutoAdaption.h"
enum {
    REPEATER_RX_VERSION_NONE        = 0,
    REPEATER_RX_VERSION_14          = 1,
    REPEATER_RX_VERSION_22          = 2
};

class HDCPTxAuth {
public:
    class TxUevntCallbak {
    public:
        TxUevntCallbak() {};
        virtual ~TxUevntCallbak() {};
        virtual void onTxEvent (char* switchName, char* hpdstate, int outputState) = 0;
    };

    HDCPTxAuth();

    ~HDCPTxAuth();

    void setBootAnimFinished(bool finished);
    void setRepeaterRxVersion(int ver);
    void setUevntCallback (TxUevntCallbak *ob);
    void setFRAutoAdpt (FrameRateAutoAdaption *mFRAutoAdpt);
    int start();
    int stop();
    void stopVerAll();
    void isAuthSuccess(int *status);

    #ifndef RECOVERY_MODE
    void sfRepaintEverything();
    #endif


private:
    bool authInit(bool *pHdcp22, bool *pHdcp14);
    bool authLoop(bool useHdcp22, bool useHdcp14);
    void startVer22();
    void startVer14();
    void mute(bool mute);

    static void* authThread(void* data);
    static void* TxUenventThreadLoop(void* data);

    SysWrite mSysWrite;
    int mRepeaterRxVer;
    TxUevntCallbak *mpCallback;
    FrameRateAutoAdaption *mFRAutoAdpt;

    bool mMute;
    bool mBootAnimFinished;

    pthread_mutex_t pthreadTxMutex;
    sem_t pthreadTxSem;
    pthread_t pthreadIdHdcpTx;
    bool mExitHdcpTxThread;
};

#endif // _HDCP_TX_AUTHENTICATION_H
