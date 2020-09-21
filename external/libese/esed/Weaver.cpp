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

#include "Weaver.h"

#include <algorithm>
#include <tuple>

#include <android-base/logging.h>

#include "../apps/weaver/include/ese/app/weaver.h"
#include "ScopedEseConnection.h"

namespace android {
namespace esed {

// libhidl
using ::android::hardware::Void;

// HAL
using ::android::hardware::weaver::V1_0::WeaverConfig;
using ::android::hardware::weaver::V1_0::WeaverReadResponse;
using ::android::hardware::weaver::V1_0::WeaverReadStatus;

// Methods from ::android::hardware::weaver::V1_0::IWeaver follow.
Return<void> Weaver::getConfig(getConfig_cb _hidl_cb) {
    LOG(VERBOSE) << "Running Weaver::getNumSlots";
    // Open SE session for applet
    ScopedEseConnection ese{mEse};
    ese.init();
    EseWeaverSession ws;
    ese_weaver_session_init(&ws);
    EseAppResult res = ese_weaver_session_open(mEse.ese_interface(), &ws);
    if (EseAppResultValue(res) == ESE_APP_RESULT_ERROR_OS) {
        switch (EseAppResultAppValue(res)) {
        case 0x6999: // SW_APPLET_SELECT_FAILED
        case 0x6A82: // SW_FILE_NOT_FOUND
            // No applet means no Weaver storage. Report no slots to prompt
            // fallback to software mode.
            _hidl_cb(WeaverStatus::OK, WeaverConfig{0, 0, 0});
            return Void();
        }
    } else if (res != ESE_APP_RESULT_OK) {
        // Transient error
        _hidl_cb(WeaverStatus::FAILED, WeaverConfig{});
        return Void();
    }

    // Call the applet
    uint32_t numSlots;
    if (ese_weaver_get_num_slots(&ws, &numSlots) != ESE_APP_RESULT_OK) {
        _hidl_cb(WeaverStatus::FAILED, WeaverConfig{});
        return Void();
    }

    // Try and close the session
    if (ese_weaver_session_close(&ws) != ESE_APP_RESULT_OK) {
        LOG(WARNING) << "Failed to close Weaver session";
    }

    _hidl_cb(WeaverStatus::OK, WeaverConfig{numSlots, kEseWeaverKeySize, kEseWeaverValueSize});
    return Void();
}

Return<WeaverStatus> Weaver::write(uint32_t slotId, const hidl_vec<uint8_t>& key,
                           const hidl_vec<uint8_t>& value) {
    LOG(INFO) << "Running Weaver::write on slot " << slotId;
    ScopedEseConnection ese{mEse};
    ese.init();
    // Validate the key and value sizes
    if (key.size() != kEseWeaverKeySize) {
        LOG(ERROR) << "Key size must be " << kEseWeaverKeySize << ", not" << key.size() << " bytes";
        return WeaverStatus::FAILED;
    }
    if (value.size() != kEseWeaverValueSize) {
        LOG(ERROR) << "Value size must be " << kEseWeaverValueSize << ", not" << value.size()
                   << " bytes";
        return WeaverStatus::FAILED;
    }

    // Open SE session for applet
    EseWeaverSession ws;
    ese_weaver_session_init(&ws);
    if (ese_weaver_session_open(mEse.ese_interface(), &ws) != ESE_APP_RESULT_OK) {
        return WeaverStatus::FAILED;
    }

    // Call the applet
    if (ese_weaver_write(&ws, slotId, key.data(), value.data()) != ESE_APP_RESULT_OK) {
        return WeaverStatus::FAILED;
    }

    // Try and close the session
    if (ese_weaver_session_close(&ws) != ESE_APP_RESULT_OK) {
        LOG(WARNING) << "Failed to close Weaver session";
    }

    return WeaverStatus::OK;
}

Return<void> Weaver::read(uint32_t slotId, const hidl_vec<uint8_t>& key, read_cb _hidl_cb) {
    LOG(VERBOSE) << "Running Weaver::read on slot " << slotId;

    // Validate the key size
    if (key.size() != kEseWeaverKeySize) {
        LOG(ERROR) << "Key size must be " << kEseWeaverKeySize << ", not" << key.size() << " bytes";
        _hidl_cb(WeaverReadStatus::FAILED, WeaverReadResponse{});
        return Void();
    }

    // Open SE session for applet
    ScopedEseConnection ese{mEse};
    ese.init();
    EseWeaverSession ws;
    ese_weaver_session_init(&ws);
    if (ese_weaver_session_open(mEse.ese_interface(), &ws) != ESE_APP_RESULT_OK) {
        _hidl_cb(WeaverReadStatus::FAILED, WeaverReadResponse{});
        return Void();
    }

    // Call the applet
    hidl_vec<uint8_t> value;
    value.resize(kEseWeaverValueSize);
    uint32_t timeout;
    const int res = ese_weaver_read(&ws, slotId, key.data(), value.data(), &timeout);
    WeaverReadStatus status;
    switch (res) {
        case ESE_APP_RESULT_OK:
            status = WeaverReadStatus::OK;
            timeout = 0;
            break;
        case ESE_WEAVER_READ_WRONG_KEY:
            status = WeaverReadStatus::INCORRECT_KEY;
            value.resize(0);
            break;
        case ESE_WEAVER_READ_TIMEOUT:
            status = WeaverReadStatus::THROTTLE;
            value.resize(0);
            break;
        default:
            status = WeaverReadStatus::FAILED;
            timeout = 0;
            value.resize(0);
            break;
    }

    // Try and close the session
    if (ese_weaver_session_close(&ws) != ESE_APP_RESULT_OK) {
        LOG(WARNING) << "Failed to close Weaver session";
    }

    _hidl_cb(status, WeaverReadResponse{timeout, value});
    return Void();
}

}  // namespace esed
}  // namespace android
