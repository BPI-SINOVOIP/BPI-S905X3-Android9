/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @date     2018/1/15
 *  @par function description:
 *  - 1 droidlogic tvserver daemon
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "main"

#include "CTvLog.h"
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <HidlTransportSupport.h>

#include "TvService.h"
#include "DroidTvServer.h"

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::vendor::amlogic::hardware::tvserver::V1_0::implementation::DroidTvServer;
using ::vendor::amlogic::hardware::tvserver::V1_0::implementation::ITvServer;
using ::vendor::amlogic::hardware::tvserver::V1_0::ITvServer;


int main(int argc __unused, char** argv __unused)
{
    android::ProcessState::initWithDriver("/dev/vndbinder");

    configureRpcThreadpool(4, false);
    sp<ProcessState> proc(ProcessState::self());

    sp<ITvServer> hidltvserver = new DroidTvServer();
    if (hidltvserver == nullptr) {
        LOGE("Cannot create ITvServer service");
    } else if (hidltvserver->registerAsService() != OK) {
        LOGE("Cannot register ITvServer service.");
    } else {
        LOGI("Treble ITvServer service created.");
    }

    /*
     * This thread is just going to process Binder transactions.
     */
    IPCThreadState::self()->joinThreadPool();
}
