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

#ifndef ANDROID_ESED_WEAVER_H
#define ANDROID_ESED_WEAVER_H

#include <android/hardware/weaver/1.0/IWeaver.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <esecpp/EseInterface.h>

namespace android {
namespace esed {

using ::android::EseInterface;
using ::android::hardware::weaver::V1_0::IWeaver;
using ::android::hardware::weaver::V1_0::WeaverStatus;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

struct Weaver : public IWeaver {
    Weaver(EseInterface& ese) : mEse(ese) {};

    // Methods from ::android::hardware::weaver::V1_0::IWeaver follow.
    Return<void> getConfig(getConfig_cb _hidl_cb) override;
    Return<WeaverStatus> write(uint32_t slotId, const hidl_vec<uint8_t>& key,
                               const hidl_vec<uint8_t>& value) override;
    Return<void> read(uint32_t slotId, const hidl_vec<uint8_t>& key, read_cb _hidl_cb) override;

private:
    EseInterface& mEse;
};

}  // namespace esed
}  // namespace android

#endif  // ANDROID_ESED_WEAVER_H
