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
 */

#ifndef VENDOR_AMLOGIC_HARDWARE_DROIDVOLD_V1_0_DROIDVOLD_H
#define VENDOR_AMLOGIC_HARDWARE_DROIDVOLD_V1_0_DROIDVOLD_H

#include <vendor/amlogic/hardware/droidvold/1.0/IDroidVold.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <utils/Mutex.h>
#include <vector>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace droidvold {
namespace V1_0 {
namespace implementation {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::vendor::amlogic::hardware::droidvold::V1_0::IDroidVold;
using ::vendor::amlogic::hardware::droidvold::V1_0::IDroidVoldCallback;
using ::vendor::amlogic::hardware::droidvold::V1_0::Result;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

class DroidVold : public IDroidVold {
public:
    DroidVold();
    virtual ~DroidVold();

    // Methods from ::vendor::amlogic::hardware::droidvold::V1_0::IDroidVold follow.
    Return<void> setCallback(const sp<IDroidVoldCallback>& callback) override;
    Return<Result> reset() override;
    Return<Result> shutdown() override;
    Return<Result> mount(const hidl_string& id, uint32_t flag, uint32_t uid) override;
    Return<Result> unmount(const hidl_string& id) override;
    Return<Result> format(const hidl_string& id, const hidl_string& type) override;
    Return<int32_t> getDiskFlag(const hidl_string &path) override;

    static DroidVold *Instance();
    void sendBroadcast(int event, const std::string& message);

private:
    //std::vector<sp<IDroidVoldCallback>> mClients;
    sp<IDroidVoldCallback> mCallback;
    mutable android::Mutex mLock;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace droidvold
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor

#endif  // VENDOR_AMLOGIC_HARDWARE_DROIDVOLD_V1_0_DROIDVOLD_H
