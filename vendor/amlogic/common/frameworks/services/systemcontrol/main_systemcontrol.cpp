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
 *  @version  2.0
 *  @date     2014/11/04
 *  @par function description:
 *  - 1 control system sysfs proc env & property
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <HidlTransportSupport.h>

//#include "SystemControl.h"
#include "SystemControlService.h"
#include "SystemControlHal.h"


using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::vendor::amlogic::hardware::systemcontrol::V1_1::implementation::SystemControlHal;
using ::vendor::amlogic::hardware::systemcontrol::V1_1::ISystemControl;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;

int main(int argc, char** argv)
{
    //char value[PROPERTY_VALUE_MAX];
    const char* path = NULL;
    if(argc >= 2){
        path = argv[1];
    }

    bool treble = property_get_bool("persist.vendor.system_control.treble", true);
    if (treble) {
        android::ProcessState::initWithDriver("/dev/vndbinder");
    }

    ALOGI("systemcontrol starting in %s mode", treble?"treble":"normal");
    configureRpcThreadpool(16, false);
    sp<ProcessState> proc(ProcessState::self());

    if (treble) {
        SystemControlService *controlIntf = SystemControlService::instantiate(path);
        sp<ISystemControl> controlHal = new SystemControlHal(controlIntf);
        if (controlHal == nullptr) {
            ALOGE("Cannot create ISystemControl service");
        } else if (controlHal->registerAsService() != OK) {
            ALOGE("Cannot register ISystemControl service.");
        } else {
            ALOGI("Treble ISystemControl service created.");
        }
    }
    else {
        //SystemControl *control = SystemControl::instantiate(path);
    }

    /*
     * This thread is just going to process Binder transactions.
     */
    IPCThreadState::self()->joinThreadPool();
}
