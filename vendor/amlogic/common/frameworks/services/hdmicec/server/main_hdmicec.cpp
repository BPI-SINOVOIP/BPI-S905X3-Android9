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
 *  @date     2017/9/25
 *  @par function description:
 *  - 1 droidlogic hdmi cec daemon
 */

#define LOG_TAG "hdmicecd"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <HidlTransportSupport.h>

#include "HdmiCecService.h"
#include "DroidHdmiCec.h"

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::vendor::amlogic::hardware::hdmicec::V1_0::implementation::DroidHdmiCec;
using ::vendor::amlogic::hardware::hdmicec::V1_0::IDroidHdmiCEC;


int main(int argc __unused, char** argv __unused)
{
    //bool treble = property_get_bool("persist.hdmi_cec.treble", true);
    bool treble = true;
    if (treble) {
        android::ProcessState::initWithDriver("/dev/vndbinder");
    }

    ALOGI("hdmi cec daemon starting in %s mode", treble?"treble":"normal");
    configureRpcThreadpool(4, false);
    sp<ProcessState> proc(ProcessState::self());

    if (treble) {
        sp<IDroidHdmiCEC>hidlHdmiCec = new DroidHdmiCec();
        if (hidlHdmiCec == nullptr) {
            ALOGE("Cannot create IDroidHdmiCEC service");
        } else if (hidlHdmiCec->registerAsService() != OK) {
            ALOGE("Cannot register IDroidHdmiCEC service.");
        } else {
            ALOGI("Treble IDroidHdmiCEC service created.");
        }
    }
    else {
        //HdmiCecService::instantiate();
    }

    /*
     * This thread is just going to process Binder transactions.
     */
    IPCThreadState::self()->joinThreadPool();
}

