/*
 * Copyright (C) 2018 The Android Open Source Project
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
 *  @author   yongguang.hong
 *  @version  1.0
 *  @date     2018/03/19
 *  @par function description:
 *  - 1 bluetooth rc service
 */
#include <utils/Log.h>
#include "RemoteControl.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace remotecontrol {
namespace V1_0 {
namespace implementation {

RemoteControl::RemoteControl(Callback& callback)
    : mCallback(callback)
    , mDeathRecipient(new DeathRecipient(this)) {
}

RemoteControl::~RemoteControl() {
    mDeathRecipient = NULL;
}

// Methods from ::vendor::amlogic::hardware::remotecontrol::V1_0::IRemoteControl follow.
Return<void> RemoteControl::setMicEnable(int32_t flag) {
    // TODO implement
    mCallback.onSetMicEnable(flag);

    return Void();
}

Return<void> RemoteControl::onDeviceChanged(int32_t flag) {
    // TODO implement
    mCallback.onDeviceStatusChanged(flag);

    return Void();
}

void RemoteControl::handleServiceDeath(uint32_t type) {
    ALOGW("RemoteControl handleServiceDeath:%d", (int)type);
}

RemoteControl::DeathRecipient::DeathRecipient(sp<RemoteControl> rc)
        : mRemoteControl(rc) {
}

void RemoteControl::DeathRecipient::serviceDied(
        uint64_t cookie,
        const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    ALOGE("RemoteControl daemon a client died cookie:%d", (int)cookie);

    uint32_t type = static_cast<uint32_t>(cookie);
    mRemoteControl->handleServiceDeath(type);
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//IRemoteControl* HIDL_FETCH_IRemoteControl(const char* /* name */) {
//    return new RemoteControl();
//}

}  // namespace implementation
}  // namespace V1_0
}  // namespace remotecontrol
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor

