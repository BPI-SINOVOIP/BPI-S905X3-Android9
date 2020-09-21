/*
 * Copyright 2017, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_KEYMASTER_KEYMASTER_DEVICE_H
#define ANDROID_HARDWARE_KEYMASTER_KEYMASTER_DEVICE_H

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>

#include <Keymaster.client.h>

#include <vector>

namespace android {
namespace hardware {
namespace keymaster {

using ::android::hardware::keymaster::V4_0::ErrorCode;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;
using ::android::hardware::keymaster::V4_0::HmacSharingParameters;
using ::android::hardware::keymaster::V4_0::IKeymasterDevice;
using ::android::hardware::keymaster::V4_0::KeyFormat;
using ::android::hardware::keymaster::V4_0::KeyParameter;
using ::android::hardware::keymaster::V4_0::KeyPurpose;
using ::android::hardware::keymaster::V4_0::VerificationToken;
using ::android::hardware::Return;
using ::android::hardware::hidl_vec;
using ::nugget::app::keymaster::BootColor;

#define KM_MAX_PROTO_FIELD_SIZE 2048

using KeymasterClient = ::nugget::app::keymaster::IKeymaster;

struct KeymasterDevice : public IKeymasterDevice {
    KeymasterDevice(KeymasterClient& keymaster);
    ~KeymasterDevice() override = default;

    // Methods from ::android::hardware::keymaster::V4_0::IKeymasterDevice follow.
    Return<void> getHardwareInfo(getHardwareInfo_cb _hidl_cb) override;
    Return<void> getHmacSharingParameters(
        getHmacSharingParameters_cb _hidl_cb) override;
    Return<void> computeSharedHmac(
        const hidl_vec<HmacSharingParameters>& params,
        computeSharedHmac_cb _hidl_cb) override;
    Return<void> verifyAuthorization(
        uint64_t operationHandle,
        const hidl_vec<KeyParameter>& parametersToVerify,
        const HardwareAuthToken& authToken,
        verifyAuthorization_cb _hidl_cb) override;
    Return<ErrorCode> addRngEntropy(const hidl_vec<uint8_t>& data) override;
    Return<void> generateKey(const hidl_vec<KeyParameter>& keyParams,
                             generateKey_cb _hidl_cb) override;
    Return<void> getKeyCharacteristics(
        const hidl_vec<uint8_t>& keyBlob,
        const hidl_vec<uint8_t>& clientId,
        const hidl_vec<uint8_t>& appData,
        getKeyCharacteristics_cb _hidl_cb) override;
    Return<void> importKey(
        const hidl_vec<KeyParameter>& params, KeyFormat keyFormat,
        const hidl_vec<uint8_t>& keyData, importKey_cb _hidl_cb) override;
    Return<void> importWrappedKey(const hidl_vec<uint8_t>& wrappedKeyData,
                                  const hidl_vec<uint8_t>& wrappingKeyBlob,
                                  const hidl_vec<uint8_t>& maskingKey,
                                  const hidl_vec<KeyParameter>& unwrappingParams,
                                  uint64_t passwordSid, uint64_t biometricSid,
                                  importWrappedKey_cb _hidl_cb) override;
    Return<void> exportKey(
        KeyFormat exportFormat, const hidl_vec<uint8_t>& keyBlob,
        const hidl_vec<uint8_t>& clientId, const hidl_vec<uint8_t>& appData,
        exportKey_cb _hidl_cb) override;
    Return<void> attestKey(const hidl_vec<uint8_t>& keyToAttest,
                           const hidl_vec<KeyParameter>& attestParams,
                           attestKey_cb _hidl_cb) override;
    Return<void> upgradeKey(const hidl_vec<uint8_t>& keyBlobToUpgrade,
                            const hidl_vec<KeyParameter>& upgradeParams,
                            upgradeKey_cb _hidl_cb) override;
    Return<ErrorCode> deleteKey(const hidl_vec<uint8_t>& keyBlob) override;
    Return<ErrorCode> deleteAllKeys() override;
    Return<ErrorCode> destroyAttestationIds() override;
    Return<void> begin(KeyPurpose purpose, const hidl_vec<uint8_t>& key,
                       const hidl_vec<KeyParameter>& inParams,
                       const HardwareAuthToken& authToken,
                       begin_cb _hidl_cb) override;
    Return<void> update(
        uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
        const hidl_vec<uint8_t>& input, const HardwareAuthToken& authToken,
        const VerificationToken& verificationToken,
        update_cb _hidl_cb) override;
    Return<void> finish(
        uint64_t operationHandle, const hidl_vec<KeyParameter>& inParams,
        const hidl_vec<uint8_t>& input, const hidl_vec<uint8_t>& signature,
        const HardwareAuthToken& authToken,
        const VerificationToken& verificationToken,
        finish_cb _hidl_cb) override;
    Return<ErrorCode> abort(uint64_t operationHandle) override;

private:
    KeymasterClient& _keymaster;
    // These come from GetProperty.
    uint32_t _os_version;
    uint32_t _os_patchlevel;
    uint32_t _vendor_patchlevel;

    // These come from the bootloader through Citadel.
    bool _is_unlocked;
    BootColor _boot_color;
    std::vector<uint8_t> _boot_key;
    std::vector<uint8_t> _boot_hash;

    Return<ErrorCode> SendSystemVersionInfo() const;
    Return<ErrorCode> GetBootInfo();
};

}  // namespace keymaster
}  // namespace hardware
}  // namespace android

#endif // ANDROID_HARDWARE_KEYMASTER_KEYMASTER_DEVICE_H
