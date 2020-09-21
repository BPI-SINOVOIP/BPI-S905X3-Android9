/*
 * Copyright 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_KEYMASTER_BUFFER_H
#define ANDROID_HARDWARE_KEYMASTER_BUFFER_H

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>

#include <Keymaster.client.h>

using ::nugget::app::keymaster::BeginOperationResponse;

namespace android {
namespace hardware {
namespace keymaster {

// HAL
using ::android::hardware::keymaster::V4_0::Algorithm;
using ::android::hardware::keymaster::V4_0::ErrorCode;

size_t buffer_remaining(uint64_t handle);
ErrorCode buffer_begin(uint64_t handle, Algorithm algorithm);
ErrorCode buffer_append(uint64_t handle,
                        const hidl_vec<uint8_t>& input,
                        uint32_t *consumed);
ErrorCode buffer_peek(uint64_t handle,
                      hidl_vec<uint8_t> *data);
ErrorCode buffer_advance(uint64_t handle, size_t count);
ErrorCode buffer_final(uint64_t handle,
                       hidl_vec<uint8_t> *data);

}  // namespace keymaster
}  // hardware
}  // android

#endif  // ANDROID_HARDWARE_KEYMASTER_IMPORT_KEY_H
