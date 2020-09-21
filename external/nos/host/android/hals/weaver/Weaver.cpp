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

#include <android-base/logging.h>

#include <Weaver.h>
#include <Weaver.client.h>

namespace android {
namespace hardware {
namespace weaver {

// libhidl
using ::android::hardware::Void;

// HAL
using ::android::hardware::weaver::V1_0::WeaverConfig;
using ::android::hardware::weaver::V1_0::WeaverReadResponse;
using ::android::hardware::weaver::V1_0::WeaverReadStatus;

// Weaver app
using ::nugget::app::weaver::GetConfigRequest;
using ::nugget::app::weaver::GetConfigResponse;
using ::nugget::app::weaver::ReadRequest;
using ::nugget::app::weaver::ReadResponse;
using ::nugget::app::weaver::WriteRequest;
using ::nugget::app::weaver::WriteResponse;

// Methods from ::android::hardware::weaver::V1_0::IWeaver follow.
Return<void> Weaver::getConfig(getConfig_cb _hidl_cb) {
    LOG(VERBOSE) << "Running Weaver::getNumSlots";

    GetConfigRequest request;
    GetConfigResponse response;
    const uint32_t appStatus = _weaver.GetConfig(request, &response);

    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "App GetConfig request failed with status " << appStatus;
        _hidl_cb(WeaverStatus::FAILED, WeaverConfig{});
        return Void();
    }

    _hidl_cb(WeaverStatus::OK,
             WeaverConfig{response.number_of_slots(), response.key_size(), response.value_size()});
    return Void();
}

Return<WeaverStatus> Weaver::write(uint32_t slotId, const hidl_vec<uint8_t>& key,
                           const hidl_vec<uint8_t>& value) {
    LOG(INFO) << "Running Weaver::write on slot " << slotId;

    WriteRequest request;
    request.set_slot(slotId);
    request.set_key(key.data(), key.size());
    request.set_value(value.data(), value.size());
    WriteResponse response;
    const uint32_t appStatus = _weaver.Write(request, &response);

    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "App Write request failed with status " << appStatus;
        return WeaverStatus::FAILED;
    }

    return WeaverStatus::OK;
}

Return<void> Weaver::read(uint32_t slotId, const hidl_vec<uint8_t>& key, read_cb _hidl_cb) {
    LOG(VERBOSE) << "Running Weaver::read on slot " << slotId;

    ReadRequest request;
    request.set_slot(slotId);
    request.set_key(key.data(), key.size());
    ReadResponse response;
    const uint32_t appStatus = _weaver.Read(request, &response);

    if (appStatus != APP_SUCCESS) {
        LOG(ERROR) << "App Read request failed with status " << appStatus;
        _hidl_cb(WeaverReadStatus::FAILED, WeaverReadResponse{});
        return Void();
    }

    WeaverReadStatus status;
    hidl_vec<uint8_t> value;
    uint32_t timeout = 0;
    switch (response.error()) {
    case ReadResponse::NONE:
        status = WeaverReadStatus::OK;
        value.setToExternal(reinterpret_cast<uint8_t*>(const_cast<char*>(response.value().data())),
                            response.value().size(), false);
        break;
    case ReadResponse::WRONG_KEY:
        LOG(WARNING) << "Wrong key used when reading slot " << slotId;
        status = WeaverReadStatus::INCORRECT_KEY;
        timeout = response.throttle_msec();
        break;
    case ReadResponse::THROTTLE:
        LOG(WARNING) << "Attempted to read slot " << slotId << " when throttling is active";
        status = WeaverReadStatus::THROTTLE;
        timeout = response.throttle_msec();
        break;
    default:
        LOG(ERROR) << "Unexpected error code from app (" << response.error() << ")";
        status = WeaverReadStatus::FAILED;
        break;
    }

    _hidl_cb(status, WeaverReadResponse{timeout, value});
    return Void();
}

} // namespace weaver
} // namespace hardware
} // namespace android
