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

#ifndef ANDROID_HARDWARE_WEAVER_WEAVER_H
#define ANDROID_HARDWARE_WEAVER_WEAVER_H

#include <android/hardware/weaver/1.0/IWeaver.h>

#include <Weaver.client.h>

namespace android {
namespace hardware {
namespace weaver {

using ::android::hardware::weaver::V1_0::IWeaver;
using ::android::hardware::weaver::V1_0::WeaverStatus;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

using WeaverClient = ::nugget::app::weaver::IWeaver;

struct Weaver : public IWeaver {
    Weaver(WeaverClient& weaver) : _weaver{weaver} {}
    ~Weaver() override = default;

    // Methods from ::android::hardware::weaver::V1_0::IWeaver follow.
    Return<void> getConfig(getConfig_cb _hidl_cb) override;
    Return<WeaverStatus> write(uint32_t slotId, const hidl_vec<uint8_t>& key,
                               const hidl_vec<uint8_t>& value) override;
    Return<void> read(uint32_t slotId, const hidl_vec<uint8_t>& key, read_cb _hidl_cb) override;

private:
    WeaverClient& _weaver;
};

} // namespace weaver
} // namespace hardware
} // namespace android

#endif // ANDROID_HARDWARE_WEAVER_WEAVER_H
