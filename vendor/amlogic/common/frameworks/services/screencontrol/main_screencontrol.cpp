/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "screencontrol"
#define LOG_NDEBUG 0

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <HidlTransportSupport.h>
#include "ScreenControlService.h"
#include "ScreenControlHal.h"

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::vendor::amlogic::hardware::screencontrol::V1_0::implementation::ScreenControlHal;
using ::vendor::amlogic::hardware::screencontrol::V1_0::IScreenControl;

int main(int argc, char** argv)
{
    ALOGE("screen_control daemon starting");
    bool treble = property_get_bool("persist.screen_control.treble", true);
    if (treble) {
        ALOGE("screen_control daemon starting in treble mode");
        android::ProcessState::initWithDriver("/dev/vndbinder");
    }
    ALOGI("screen_control daemon starting in %s mode", treble?"treble":"normal");
    configureRpcThreadpool(4, false);
    sp<ProcessState> proc(ProcessState::self());

    if (treble) {
        sp<IScreenControl> screen = new ScreenControlHal();
        if (screen == nullptr) {
            ALOGE("Cannot create IScreenControl service");
        } else if (screen->registerAsService() != OK) {
            ALOGE("Cannot register IScreenControl service.");
        } else {
            ALOGI("Treble IScreenControl service created.");
        }
    }else {
        ScreenControlService::instantiate();
    }
    IPCThreadState::self()->joinThreadPool();
}
