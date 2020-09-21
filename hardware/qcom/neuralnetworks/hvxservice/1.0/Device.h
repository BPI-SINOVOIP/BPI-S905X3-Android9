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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_0_DEVICE_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_0_DEVICE_H

#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <android/hardware/neuralnetworks/1.0/IPreparedModel.h>
#include <android/hardware/neuralnetworks/1.0/types.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <string>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct Device : public IDevice {
    Device();
    ~Device() override;

    // Methods from IDevice follow.
    Return<void> getCapabilities(getCapabilities_cb _hidl_cb) override;
    Return<void> getSupportedOperations(const Model& model,
                                        getSupportedOperations_cb _hidl_cb) override;
    Return<ErrorStatus> prepareModel(const Model& model,
                                     const sp<IPreparedModelCallback>& callback) override;
    Return<DeviceStatus> getStatus() override;

   private:
    DeviceStatus mCurrentStatus;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_0_DEVICE_H
