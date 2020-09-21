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

#ifndef CLEARKEY_CRYPTO_FACTORY_H_
#define CLEARKEY_CRYPTO_FACTORY_H_

#include <android/hardware/drm/1.0/ICryptoPlugin.h>
#include <android/hardware/drm/1.1/ICryptoFactory.h>

#include "ClearKeyTypes.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_1 {
namespace clearkey {

using ::android::hardware::drm::V1_1::ICryptoFactory;
using ::android::hardware::drm::V1_0::ICryptoPlugin;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;

struct CryptoFactory : public ICryptoFactory {
    CryptoFactory() {}
    virtual ~CryptoFactory() {}

    Return<bool> isCryptoSchemeSupported(const hidl_array<uint8_t, 16>& uuid)
            override;

    Return<void> createPlugin(
            const hidl_array<uint8_t, 16>& uuid,
            const hidl_vec<uint8_t>& initData,
            createPlugin_cb _hidl_cb) override;

private:
    CLEARKEY_DISALLOW_COPY_AND_ASSIGN(CryptoFactory);

};

} // namespace clearkey
} // namespace V1_1
} // namespace drm
} // namespace hardware
} // namespace android

#endif // CLEARKEY_CRYPTO_FACTORY_H_
