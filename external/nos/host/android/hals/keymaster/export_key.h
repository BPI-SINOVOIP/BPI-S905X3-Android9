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

#ifndef ANDROID_HARDWARE_KEYMASTER_EXPORT_KEY_H
#define ANDROID_HARDWARE_KEYMASTER_EXPORT_KEY_H

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>

#include <Keymaster.client.h>

namespace android {
namespace hardware {
namespace keymaster {

// HAL
using ::android::hardware::keymaster::V4_0::ErrorCode;

// Keymaster app
using ::nugget::app::keymaster::ExportKeyResponse;

ErrorCode export_key_der(const ExportKeyResponse& response,
                          hidl_vec<uint8_t> *der);

}  // namespace keymaster
}  // hardware
}  // android

#endif  // ANDROID_HARDWARE_KEYMASTER_EXPORT_KEY_H
