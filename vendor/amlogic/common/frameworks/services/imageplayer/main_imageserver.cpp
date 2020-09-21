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

#define LOG_TAG "imageserver"
#define LOG_NDEBUG 0

#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <utils/Log.h>
//#include "RegisterExtensions.h"
#include <HidlTransportSupport.h>
#include "ImagePlayerService.h"
#include "ImagePlayerHal.h"

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::vendor::amlogic::hardware::imageserver::V1_0::implementation::ImagePlayerHal;
using ::vendor::amlogic::hardware::imageserver::V1_0::IImageService;

int main(int argc, char** argv) {
    ALOGE("imageserver daemon starting");
    bool treble = property_get_bool("persist.imageserver.treble", true);

    if (treble) {
        android::ProcessState::initWithDriver("/dev/vndbinder");
    }

    ALOGI("imageserver daemon starting in %s mode", treble ? "treble" : "normal");
    configureRpcThreadpool(1, false);
    sp<ProcessState> proc(ProcessState::self());

    if (treble) {
        sp<IImageService> player = new ImagePlayerHal();

        if (player == nullptr) {
            ALOGE("Cannot create ISystemControl service");
        } else if (player->registerAsService() != OK) {
            ALOGE("Cannot register ISystemControl service.");
        } else {
            ALOGI("Treble ImagePlayerHal service created.");
        }
    } else {
        ImagePlayerService::instantiate();
    }

    IPCThreadState::self()->joinThreadPool();
}
