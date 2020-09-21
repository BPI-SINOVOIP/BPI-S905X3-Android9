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
 *  - 1 process HDCP RX authenticate
 */

#ifndef _HDCP_RX_AUTHENTICATION_H
#define _HDCP_RX_AUTHENTICATION_H

#include "common.h"
#include "HDCPTxAuth.h"

class HDCPRxAuth {
public:
    HDCPRxAuth(HDCPTxAuth *txAuth);

    ~HDCPRxAuth();

private:
    HDCPTxAuth *pTxAuth;
    int mLastVideoState;

    void initKey();
    void startVer22();
    void stopVer22();
    void forceFlushVideoLayer();
    static void* RxUenventThreadLoop(void* data);
};

#endif // _HDCP_RX_AUTHENTICATION_H