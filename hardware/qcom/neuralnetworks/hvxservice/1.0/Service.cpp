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

#define LOG_TAG "android.hardware.neuralnetworks@1.0-service-hvx"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <hidl/HidlTransportSupport.h>
#include "Device.h"

// Generated HIDL files
using android::hardware::neuralnetworks::V1_0::IDevice;
using android::hardware::neuralnetworks::V1_0::implementation::Device;

int main() {
    android::sp<IDevice> device = new Device();
    android::hardware::configureRpcThreadpool(4, true /* will join */);
    if (device->registerAsService("hvx") != android::OK) {
        LOG(ERROR) << "Could not register service";
        return 1;
    }
    android::hardware::joinRpcThreadpool();
    LOG(ERROR) << "Hvx service exited!";
    return 1;
}
