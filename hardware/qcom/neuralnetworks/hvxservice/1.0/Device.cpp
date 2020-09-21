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

#define LOG_TAG "android.hardware.neuralnetworks@1.0-impl-hvx"

#include "Device.h"
#include <android-base/logging.h>
#include <memory>
#include <mutex>
#include <thread>
#include "HexagonModel.h"
#include "HexagonUtils.h"
#include "PreparedModel.h"
#include "ValidateHal.h"

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace implementation {

Device::Device() : mCurrentStatus(DeviceStatus::AVAILABLE) {}

Device::~Device() {}

static std::once_flag configure_nnlib;
static void configureHexagon() {
    std::call_once(configure_nnlib, []() {
        hexagon::Controller::getInstance().config();
    });
}

Return<void> Device::getCapabilities(getCapabilities_cb _hidl_cb) {
    configureHexagon();

    // These numbers are approximations for this release.
    // TODO Change with the actual number.
    PerformanceInfo float32Performance = {
        .execTime = 30.0f, .powerUsage = 2.0f,
    };

    PerformanceInfo quantized8Performance = {
        .execTime = 0.7f, .powerUsage = 0.7f,
    };

    Capabilities capabilities = {
        .float32Performance = float32Performance, .quantized8Performance = quantized8Performance,
    };

    ErrorStatus status =
        hexagon::isHexagonAvailable() ? ErrorStatus::NONE : ErrorStatus::DEVICE_UNAVAILABLE;

    _hidl_cb(status, capabilities);
    return Void();
}

Return<void> Device::getSupportedOperations(const Model& model,
                                            getSupportedOperations_cb _hidl_cb) {
    configureHexagon();

    if (!nn::validateModel(model)) {
        _hidl_cb(ErrorStatus::INVALID_ARGUMENT, std::vector<bool>{});
        return Void();
    }
    if (!hexagon::isHexagonAvailable()) {
        _hidl_cb(ErrorStatus::DEVICE_UNAVAILABLE, std::vector<bool>{});
        return Void();
    }

    hexagon::Model hexagonModel(model);
    std::vector<bool> supported = hexagonModel.supportedOperations();

    _hidl_cb(ErrorStatus::NONE, supported);
    return Void();
}

static void asyncPrepare(const Model& model, const sp<IPreparedModelCallback>& callback) {
    std::shared_ptr<hexagon::Model> hexagonModel = std::make_shared<hexagon::Model>(model);

    Return<void> ret;
    if (hexagonModel->prepare()) {
        ret = callback->notify(ErrorStatus::NONE, new PreparedModel(model, hexagonModel));
    } else {
        ret = callback->notify(ErrorStatus::GENERAL_FAILURE, nullptr);
    }
    if (!ret.isOk()) {
        LOG(ERROR) << "Error in callback's return type: " << ret.description();
    }
}

Return<ErrorStatus> Device::prepareModel(const Model& model,
                                         const sp<IPreparedModelCallback>& callback) {
    configureHexagon();

    if (callback.get() == nullptr) {
        LOG(ERROR) << "invalid callback passed to prepareModel";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!nn::validateModel(model)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!hexagon::isHexagonAvailable()) {
        callback->notify(ErrorStatus::DEVICE_UNAVAILABLE, nullptr);
        return ErrorStatus::DEVICE_UNAVAILABLE;
    }

    // TODO: once nnlib hanging issue is resolved, make this function
    // asynchronous again
    asyncPrepare(model, callback);

    return ErrorStatus::NONE;
}

Return<DeviceStatus> Device::getStatus() {
    configureHexagon();
    mCurrentStatus =
        hexagon::isHexagonAvailable() ? DeviceStatus::AVAILABLE : DeviceStatus::OFFLINE;
    return mCurrentStatus;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
