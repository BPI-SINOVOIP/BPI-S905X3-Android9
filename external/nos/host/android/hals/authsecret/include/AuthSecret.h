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
 */

#ifndef ANDROID_HARDWARE_AUTHSECRET_AUTHSECRET_H
#define ANDROID_HARDWARE_AUTHSECRET_AUTHSECRET_H

#include <android/hardware/authsecret/1.0/IAuthSecret.h>

#include <nos/NuggetClientInterface.h>

namespace android {
namespace hardware {
namespace authsecret {

using ::android::hardware::authsecret::V1_0::IAuthSecret;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

using ::nos::NuggetClientInterface;

struct AuthSecret : public IAuthSecret {
    AuthSecret(NuggetClientInterface& client) : _client(client) {}
    ~AuthSecret() override = default;

    // Methods from ::android::hardware::authsecret::V1_0::IAuthSecret follow.
    Return<void> primaryUserCredential(const hidl_vec<uint8_t>& secret) override;

private:
    NuggetClientInterface& _client;
};

} // namespace authsecret
} // namespace hardware
} // namespace android

#endif // ANDROID_HARDWARE_AUTHSECRET_AUTHSECRET_H
