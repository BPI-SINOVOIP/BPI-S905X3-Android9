/*
 * Copyright 2017 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_KEYMASTER_IMPORT_WRAPPED_KEY_H
#define ANDROID_HARDWARE_KEYMASTER_IMPORT_WRAPPED_KEY_H

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>

#include <Keymaster.client.h>

namespace android {
namespace hardware {
namespace keymaster {

// HAL
using ::android::hardware::keymaster::V4_0::ErrorCode;
using ::android::hardware::keymaster::V4_0::KeyFormat;
using ::android::hardware::keymaster::V4_0::KeyParameter;

// Keymaster app
using ::nugget::app::keymaster::ImportWrappedKeyRequest;

#define KM_WRAPPER_MASKING_KEY_SIZE  32

ErrorCode import_wrapped_key_request(const hidl_vec<uint8_t>& wrappedKeyData,
                                     const hidl_vec<uint8_t>& wrappingKeyBlob,
                                     const hidl_vec<uint8_t>& maskingKey,
                                     ImportWrappedKeyRequest *request);

}  // namespace keymaster
}  // hardware
}  // android

#endif  // ANDROID_HARDWARE_KEYMASTER_IMPORT_WRAPPED_KEY_H
