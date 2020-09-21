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

#include "OemLock.h"

#include <memory>

#include <android-base/endian.h>
#include <android-base/logging.h>

#include <avb.h>

namespace android {
namespace hardware {
namespace oemlock {

// libhidl
using ::android::hardware::Void;

// AVB app
using ::nugget::app::avb::CarrierUnlock;
using ::nugget::app::avb::CarrierUnlockRequest;
using ::nugget::app::avb::CarrierUnlockResponse;
using ::nugget::app::avb::GetLockRequest;
using ::nugget::app::avb::GetLockResponse;
using ::nugget::app::avb::LockIndex;
using ::nugget::app::avb::SetDeviceLockRequest;
using ::nugget::app::avb::SetDeviceLockResponse;

// Methods from ::android::hardware::oemlock::V1_0::IOemLock follow.
Return<void> OemLock::getName(getName_cb _hidl_cb) {
    LOG(VERBOSE) << "Running OemLock::getName";
    _hidl_cb(OemLockStatus::OK, {"01"});
    return Void();
}

Return<OemLockSecureStatus> OemLock::setOemUnlockAllowedByCarrier(
    bool allowed, const hidl_vec<uint8_t>& signature) {
    LOG(INFO) << "Running OemLock::setOemUnlockAllowedByCarrier: " << allowed;

    if (!allowed) {
        // Locking is only performed in the factory
        LOG(ERROR) << "Cannot carrier lock the device";
        return OemLockSecureStatus::FAILED;
    }

    auto unlock = std::make_unique<CarrierUnlock>();
    if (!carrierUnlockFromSignature(signature, unlock.get())) {
        return OemLockSecureStatus::INVALID_SIGNATURE;
    }

    CarrierUnlockRequest request;
    request.set_allocated_token(unlock.release());
    CarrierUnlockResponse response;
    const uint32_t appStatus =_avbApp.CarrierUnlock(request, &response);

    if (appStatus == APP_ERROR_AVB_AUTHORIZATION) {
        LOG(WARNING) << "Carrier unlock signature rejected by app";
        return OemLockSecureStatus::INVALID_SIGNATURE;
    }

    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "App CarrierUnlock request failed with status " << appStatus;
        return OemLockSecureStatus::FAILED;
    }

    return OemLockSecureStatus::OK;
}

Return<void> OemLock::isOemUnlockAllowedByCarrier(isOemUnlockAllowedByCarrier_cb _hidl_cb) {
    LOG(VERBOSE) << "Running OemLock::isOemUnlockAllowedByCarrier";

    GetLockRequest request;
    request.set_lock(LockIndex::CARRIER);
    GetLockResponse response;
    const uint32_t appStatus = _avbApp.GetLock(request, &response);

    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "App GetLock request for CARRIER failed with status " << appStatus;
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }

    const bool allowed = response.locked() == 0;
    _hidl_cb(OemLockStatus::OK, allowed);
    return Void();
}

Return<OemLockStatus> OemLock::setOemUnlockAllowedByDevice(bool allowed) {
    LOG(INFO) << "Running OemLock::setOemUnlockAllowedByDevice: " << allowed;

    SetDeviceLockRequest request;
    request.set_locked(allowed ? 0 : 1);
    SetDeviceLockResponse response;
    const uint32_t appStatus = _avbApp.SetDeviceLock(request, &response);

    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "App SetDeviceLock failed with status " << appStatus;
        return OemLockStatus::FAILED;
    }

    return OemLockStatus::OK;
}

Return<void> OemLock::isOemUnlockAllowedByDevice(isOemUnlockAllowedByDevice_cb _hidl_cb) {
    LOG(VERBOSE) << "Running OemLock::isOemUnlockAllowedByDevice";

    GetLockRequest request;
    request.set_lock(LockIndex::DEVICE);
    GetLockResponse response;
    const uint32_t appStatus = _avbApp.GetLock(request, &response);

    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "App GetLock request for DEVICE failed with status " << appStatus;
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }

    const bool allowed = response.locked() == 0;
    _hidl_cb(OemLockStatus::OK, allowed);
    return Void();
}

/**
 * Decompose the signature into:
 *    1. 64-bit version
 *    2. 64-bit nonce
 *    3. unlock token bytes
 *
 * @param signature The signature to parse.
 * @param unlock The message to populate with the parsed data.
 * @return whether the signature could be parsed.
 */
bool OemLock::carrierUnlockFromSignature(const hidl_vec<uint8_t>& signature,
                                         CarrierUnlock* unlock) {
    CHECK(unlock != nullptr);

    // Ensure there is enough data to decode. Not checking details of the token
    // in the HAL.
    if (signature.size() < (sizeof(uint64_t) * 2)) {
        return false;
    }

    auto it = signature.begin();

    // Get the version
    uint64_t version;
    memcpy(&version, &(*it), sizeof(uint64_t));
    it += sizeof(uint64_t);
    unlock->set_version(letoh64(version));

    // Get the nonce
    uint64_t nonce;
    memcpy(&nonce, &(*it), sizeof(uint64_t));
    it += sizeof(uint64_t);
    unlock->set_nonce(letoh64(nonce));

    // Remaining is the unlock token
    unlock->set_signature(&(*it), std::distance(it, signature.end()));

    return true;
}

} // namespace oemlock
} // namespace hardware
} // namespace android
