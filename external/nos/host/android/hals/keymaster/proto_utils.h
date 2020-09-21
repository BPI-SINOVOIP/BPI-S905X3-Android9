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

#ifndef ANDROID_HARDWARE_KEYMASTER_PROTO_UTILS_H
#define ANDROID_HARDWARE_KEYMASTER_PROTO_UTILS_H

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>

#include <Keymaster.client.h>

#include <map>
#include <vector>

namespace android {
namespace hardware {
namespace keymaster {

// HAL
using ::android::hardware::keymaster::V4_0::Algorithm;
using ::android::hardware::keymaster::V4_0::EcCurve;
using ::android::hardware::keymaster::V4_0::ErrorCode;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::keymaster::V4_0::KeyParameter;
using ::android::hardware::keymaster::V4_0::SecurityLevel;
using ::android::hardware::keymaster::V4_0::Tag;
using ::android::hardware::keymaster::V4_0::VerificationToken;
using ::android::hardware::hidl_vec;

// std
using std::map;
using std::vector;

// App
namespace nosapp = nugget::app::keymaster;

typedef map<Tag, vector<KeyParameter>> tag_map_t;

ErrorCode key_parameter_to_pb(const KeyParameter& param,
                              nosapp::KeyParameter *pb);
ErrorCode hidl_params_to_pb(const hidl_vec<KeyParameter>& params,
                            nosapp::KeyParameters *pbParams);
ErrorCode hidl_params_to_map(const hidl_vec<KeyParameter>& params,
                             tag_map_t *tag_map);
ErrorCode map_params_to_pb(const tag_map_t& params,
                           nosapp::KeyParameters *pbParams);
ErrorCode pb_to_hidl_params(const nosapp::KeyParameters& pbParams,
                            hidl_vec<KeyParameter> *params);
ErrorCode translate_algorithm(nosapp::Algorithm algorithm,
                              Algorithm *out);
ErrorCode translate_error_code(nosapp::ErrorCode error_code);
ErrorCode translate_ec_curve(nosapp::EcCurve ec_curve, EcCurve *out);
ErrorCode translate_auth_token(const HardwareAuthToken& auth_token,
                          nosapp::HardwareAuthToken *out);
void translate_verification_token(
    const VerificationToken& verification_token,
    nosapp::VerificationToken *out);

}  // namespace keymaster
}  // hardware
}  // android

#endif  // ANDROID_HARDWARE_KEYMASTER_PROTO_UTILS_H
