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

#include <vector>

#include <android-base/logging.h>
#include "../apps/boot/include/ese/app/boot.h"
#include "ScopedEseConnection.h"

namespace android {
namespace esed {

// libhidl
using ::android::hardware::Void;

// Methods from ::android::hardware::oemlock::V1_0::IOemLock follow.
Return<void> OemLock::getName(getName_cb _hidl_cb) {
  _hidl_cb(OemLockStatus::OK, {"01"});
  return Void();
}

Return<OemLockSecureStatus> OemLock::setOemUnlockAllowedByCarrier(
        bool allowed, const hidl_vec<uint8_t>& signature) {
    LOG(INFO) << "Running OemLock::setOemUnlockAllowedByCarrier: " << allowed;
    ScopedEseConnection ese{mEse};
    ese.init();
    // In general, setting the carrier lock to locked is only done in factory,
    // but there is no reason the HAL could not be used in factory to do it.
    // As such, the signature would actually be a specially formatted string of
    // identifiers.  Unlocking requires a signature packaged in a simple format
    // as well.
    //
    // See ../apps/boot/README.md for details.
    std::vector<uint8_t> data(signature);
    // "allowed" == unlocked == 0.
    uint8_t lock_byte = allowed ? 0 : 1;
    // xset expects the lock value as the first byte.
    data.insert(data.cbegin(), lock_byte);

   // Open SE session for applet
    EseBootSession session;
    ese_boot_session_init(&session);
    EseAppResult res = ese_boot_session_open(mEse.ese_interface(), &session);
    if (res != ESE_APP_RESULT_OK) {
        LOG(ERROR) << "Failed to open a boot session: " << res;
        return OemLockSecureStatus::FAILED;
    }
    res = ese_boot_lock_xset(&session, kEseBootLockIdCarrier,
                             data.data(), data.size());
    if (res != ESE_APP_RESULT_OK) {
        LOG(ERROR) << "Failed to change lock state (allowed="
                   << allowed << "): " << res;
    }

    // Try and close the session without perturbing our result value.
    if (ese_boot_session_close(&session) != ESE_APP_RESULT_OK) {
        LOG(WARNING) << "Failed to close boot session";
    }

    if (EseAppResultValue(res) == ESE_APP_RESULT_ERROR_APPLET) {
        // 0004 and 0005 are invalid signature and invalid nonce respectively.
        return OemLockSecureStatus::INVALID_SIGNATURE;
    } else if (res != ESE_APP_RESULT_OK) {
        return OemLockSecureStatus::FAILED;
    }
    return OemLockSecureStatus::OK;
}

Return<void> OemLock::isOemUnlockAllowedByCarrier(isOemUnlockAllowedByCarrier_cb _hidl_cb) {
    LOG(VERBOSE) << "Running OemLock::isOemUnlockAllowedByCarrier";
    ScopedEseConnection ese{mEse};
    ese.init();
    // Open SE session for applet
    EseBootSession session;
    ese_boot_session_init(&session);
    EseAppResult res = ese_boot_session_open(mEse.ese_interface(), &session);
    if (res != ESE_APP_RESULT_OK) {
        LOG(ERROR) << "Failed to open a boot session: " << res;
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }
    std::vector<uint8_t> data;
    data.resize(1024);
    uint16_t actualData = 0;
    res = ese_boot_lock_xget(&session, kEseBootLockIdCarrier,
                             &data[0], data.size(),
                             &actualData);
    if (res != ESE_APP_RESULT_OK || actualData == 0) {
        LOG(ERROR) << "Failed to get lock state: " << res;
    }

    // Try and close the session without perturbing our result value.
    if (ese_boot_session_close(&session) != ESE_APP_RESULT_OK) {
        LOG(WARNING) << "Failed to close boot session";
    }

    if (res != ESE_APP_RESULT_OK) {
        // Fail closed.
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }
    // if data[0] == 1, lock == true, so allowed == false.
    _hidl_cb(OemLockStatus::OK, data[0] != 0 ? false : true);
    return Void();
}

Return<OemLockStatus> OemLock::setOemUnlockAllowedByDevice(bool allowed) {
    LOG(INFO) << "Running OemLock::setOemUnlockAllowedByDevice: " << allowed;
    ScopedEseConnection ese{mEse};
    ese.init();
    // "allowed" == unlocked == 0.
    uint8_t lock_byte = allowed ? 0 : 1;

   // Open SE session for applet
    EseBootSession session;
    ese_boot_session_init(&session);
    EseAppResult res = ese_boot_session_open(mEse.ese_interface(), &session);
    if (res != ESE_APP_RESULT_OK) {
        LOG(ERROR) << "Failed to open a boot session: " << res;
        return OemLockStatus::FAILED;
    }
    res = ese_boot_lock_set(&session, kEseBootLockIdDevice, lock_byte);
    if (res != ESE_APP_RESULT_OK) {
        LOG(ERROR) << "Failed to change device lock state (allowed="
                   << allowed << "): " << res;
    }

    // Try and close the session without perturbing our result value.
    if (ese_boot_session_close(&session) != ESE_APP_RESULT_OK) {
        LOG(WARNING) << "Failed to close boot session";
    }

    if (res != ESE_APP_RESULT_OK) {
        return OemLockStatus::FAILED;
    }
    return OemLockStatus::OK;
}

Return<void> OemLock::isOemUnlockAllowedByDevice(isOemUnlockAllowedByDevice_cb _hidl_cb) {
    LOG(VERBOSE) << "Running OemLock::isOemUnlockAllowedByDevice";
    ScopedEseConnection ese{mEse};
    ese.init();
    // Open SE session for applet
    EseBootSession session;
    ese_boot_session_init(&session);
    EseAppResult res = ese_boot_session_open(mEse.ese_interface(), &session);
    if (res != ESE_APP_RESULT_OK) {
        LOG(ERROR) << "Failed to open a boot session: " << res;
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }
    uint8_t lock_byte = 0;
    res = ese_boot_lock_get(&session, kEseBootLockIdDevice, &lock_byte);
    if (res != ESE_APP_RESULT_OK) {
        LOG(ERROR) << "Failed to get device lock state: " << res;
    }

    // Try and close the session without perturbing our result value.
    if (ese_boot_session_close(&session) != ESE_APP_RESULT_OK) {
        LOG(WARNING) << "Failed to close boot session";
    }

    if (res != ESE_APP_RESULT_OK) {
        // Fail closed.
        _hidl_cb(OemLockStatus::FAILED, false);
        return Void();
    }
    // if data[0] == 1, lock == true, so allowed == false.
    _hidl_cb(OemLockStatus::OK, lock_byte != 0 ? false : true);
    return Void();
}

}  // namespace esed
}  // namespace android
