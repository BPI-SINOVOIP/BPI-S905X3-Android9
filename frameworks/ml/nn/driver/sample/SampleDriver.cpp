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

#define LOG_TAG "SampleDriver"

#include "SampleDriver.h"

#include "CpuExecutor.h"
#include "HalInterfaces.h"
#include "ValidateHal.h"

#include <android-base/logging.h>
#include <hidl/LegacySupport.h>
#include <thread>

namespace android {
namespace nn {
namespace sample_driver {

Return<void> SampleDriver::getCapabilities(getCapabilities_cb cb) {
    return getCapabilities_1_1(
        [&](ErrorStatus error, const V1_1::Capabilities& capabilities) {
            // TODO(dgross): Do we need to check compliantWithV1_0(capabilities)?
            cb(error, convertToV1_0(capabilities));
        });
}

Return<void> SampleDriver::getSupportedOperations(const V1_0::Model& model,
                                                  getSupportedOperations_cb cb) {
    if (!validateModel(model)) {
        VLOG(DRIVER) << "getSupportedOperations";
        std::vector<bool> supported;
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }
    return getSupportedOperations_1_1(convertToV1_1(model), cb);
}

Return<ErrorStatus> SampleDriver::prepareModel(const V1_0::Model& model,
                                               const sp<IPreparedModelCallback>& callback) {
    if (callback.get() == nullptr) {
        VLOG(DRIVER) << "prepareModel";
        LOG(ERROR) << "invalid callback passed to prepareModel";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model)) {
        VLOG(DRIVER) << "prepareModel";
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }
    return prepareModel_1_1(convertToV1_1(model), ExecutionPreference::FAST_SINGLE_ANSWER,
                            callback);
}

Return<ErrorStatus> SampleDriver::prepareModel_1_1(const V1_1::Model& model,
                                                   ExecutionPreference preference,
                                                   const sp<IPreparedModelCallback>& callback) {
    if (VLOG_IS_ON(DRIVER)) {
        VLOG(DRIVER) << "prepareModel_1_1";
        logModelToInfo(model);
    }
    if (callback.get() == nullptr) {
        LOG(ERROR) << "invalid callback passed to prepareModel";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model) || !validateExecutionPreference(preference)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    // TODO: make asynchronous later
    sp<SamplePreparedModel> preparedModel = new SamplePreparedModel(model);
    if (!preparedModel->initialize()) {
       callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
       return ErrorStatus::INVALID_ARGUMENT;
    }
    callback->notify(ErrorStatus::NONE, preparedModel);
    return ErrorStatus::NONE;
}

Return<DeviceStatus> SampleDriver::getStatus() {
    VLOG(DRIVER) << "getStatus()";
    return DeviceStatus::AVAILABLE;
}

int SampleDriver::run() {
    android::hardware::configureRpcThreadpool(4, true);
    if (registerAsService(mName) != android::OK) {
        LOG(ERROR) << "Could not register service";
        return 1;
    }
    android::hardware::joinRpcThreadpool();
    LOG(ERROR) << "Service exited!";
    return 1;
}

bool SamplePreparedModel::initialize() {
    return setRunTimePoolInfosFromHidlMemories(&mPoolInfos, mModel.pools);
}

void SamplePreparedModel::asyncExecute(const Request& request,
                                       const sp<IExecutionCallback>& callback) {
    std::vector<RunTimePoolInfo> requestPoolInfos;
    if (!setRunTimePoolInfosFromHidlMemories(&requestPoolInfos, request.pools)) {
        callback->notify(ErrorStatus::GENERAL_FAILURE);
        return;
    }

    CpuExecutor executor;
    int n = executor.run(mModel, request, mPoolInfos, requestPoolInfos);
    VLOG(DRIVER) << "executor.run returned " << n;
    ErrorStatus executionStatus =
            n == ANEURALNETWORKS_NO_ERROR ? ErrorStatus::NONE : ErrorStatus::GENERAL_FAILURE;
    Return<void> returned = callback->notify(executionStatus);
    if (!returned.isOk()) {
        LOG(ERROR) << " hidl callback failed to return properly: " << returned.description();
    }
}

Return<ErrorStatus> SamplePreparedModel::execute(const Request& request,
                                                 const sp<IExecutionCallback>& callback) {
    VLOG(DRIVER) << "execute(" << SHOW_IF_DEBUG(toString(request)) << ")";
    if (callback.get() == nullptr) {
        LOG(ERROR) << "invalid callback passed to execute";
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateRequest(request, mModel)) {
        callback->notify(ErrorStatus::INVALID_ARGUMENT);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    // This thread is intentionally detached because the sample driver service
    // is expected to live forever.
    std::thread([this, request, callback]{ asyncExecute(request, callback); }).detach();

    return ErrorStatus::NONE;
}

} // namespace sample_driver
} // namespace nn
} // namespace android
