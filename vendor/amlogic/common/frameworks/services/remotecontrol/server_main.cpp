/*
 * Copyright (C) 2018 The Android Open Source Project
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
 *  @author   yongguang.hong
 *  @version  1.0
 *  @date     2018/03/19
 *  @par function description:
 *  - 1 rc server bin
 */

#define LOG_TAG "rc_server"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "RemoteControlServer.h"

using namespace android;

static void __setMicEnableCallback(int flag) {
    ALOGD("setMicEnableCallback flag=%d", flag);
}

static void __onDeviceStatusCallback(int flag) {
    ALOGD("onDeviceStatusCallback flag=%d", flag);
}

rc_callbacks_t cb =  {
    __setMicEnableCallback,
    __onDeviceStatusCallback
};

int main(int argc __unused, char** argv __unused)
{
    //start remote control service
    ProcessState::initWithDriver("/dev/vndbinder");
    RemoteControlServer* rcServer = RemoteControlServer::getInstance();
    rcServer->setCallback(&cb);

    /*
     * This thread is just going to process Binder transactions.
     */
    IPCThreadState::self()->joinThreadPool();
}

