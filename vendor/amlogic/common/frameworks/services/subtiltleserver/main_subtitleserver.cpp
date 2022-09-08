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

#define LOG_TAG "subtitleserver"
#define LOG_NDEBUG 0

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <hidl/HidlTransportSupport.h>

#include "SubtitleServer.h"

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::ISubtitleServer;
using ::vendor::amlogic::hardware::subtitleserver::V1_0::implementation::SubtitleServer;

int main(int argc __unused, char** argv __unused) {
    ALOGE("subtitleserver daemon starting");
    bool treble = true;//property_get_bool("persist.subtitle.treble", true);
    if (treble) {
        android::ProcessState::initWithDriver("/dev/vndbinder");
    }

    ALOGI("subtitle daemon starting in %s mode", treble ? "treble" : "normal");
    configureRpcThreadpool(4, false);
    sp<ProcessState> proc(ProcessState::self());

    if (treble) {
        sp<ISubtitleServer> subtitleServer = new SubtitleServer();

        if (subtitleServer == nullptr) {
            ALOGE("Cannot create ISubtitleServer service");
        } else if (subtitleServer->registerAsService() != OK) {
            ALOGE("Cannot register ISubtitleServer service.");
        } else {
            ALOGI("Treble SubtitleServerHal service created.--");
        }
    }
    ProcessState::self()->startThreadPool();
    ALOGI("Treble SubtitleServerHal service created end-2-!!");
    //IPCThreadState::self()->joinThreadPool();
    joinRpcThreadpool();
    ALOGI("Treble SubtitleServerHal service created end-3-!!");
}

