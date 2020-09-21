/*
 * Copyright (C) 2017 The Android Open Source Project
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
 */

#include <chrono>
#include <string>
#include <thread>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/StrongPointer.h>

// Select the implementation
#include <esecpp/NxpPn80tNqNci.h>
using EseInterfaceImpl = android::NxpPn80tNqNci;

#include "Weaver.h"

using android::OK;
using android::sp;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

using namespace std::chrono_literals;

// HALs
using android::esed::Weaver;

int main(int /* argc */, char** /* argv */) {
    LOG(INFO) << "Waiting for property...";
    android::base::WaitForProperty("init.svc.vendor.ese_load", "stopped");
    LOG(INFO) << "Starting esed...";

    // Open connection to the eSE
    EseInterfaceImpl ese;
    uint32_t failCount = 0;
    while (true) {
        ese.init();
        if (ese.open() < 0) {
            std::string errMsg = "Failed to open connection to eSE";
            if (ese.error()) {
                errMsg += " (" + std::to_string(ese.error_code()) + "): " + ese.error_message();
            } else {
                errMsg += ": reason unknown";
            }
            LOG(ERROR) << errMsg;

            // FIXME: this loop with sleep is a hack to avoid the process repeatedly crashing
            ++failCount;
            std::this_thread::sleep_for(failCount * 5s);
            continue;
        }
        LOG(INFO) << "Opened connection to the eSE";
        break;
    }
    // Close it until use.
    ese.close();


    // This will be a single threaded daemon. This is important as libese is not
    // thread safe so we use binder to synchronize requests for us.
    constexpr bool thisThreadWillJoinPool = true;
    configureRpcThreadpool(1, thisThreadWillJoinPool);

    // Create Weaver HAL instance
    sp<Weaver> weaver = new Weaver{ese};
    const status_t status = weaver->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Failed to register Weaver as a service (status: " << status << ")";
    }


    joinRpcThreadpool();
    return -1; // Should never reach here
}
