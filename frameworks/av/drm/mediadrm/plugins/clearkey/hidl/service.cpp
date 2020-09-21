/*
 * Copyright 2018 The Android Open Source Project
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
#define LOG_TAG "android.hardware.drm@1.1-service.clearkey"

#include <CryptoFactory.h>
#include <DrmFactory.h>

#include <android-base/logging.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>

using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::sp;

using android::hardware::drm::V1_1::ICryptoFactory;
using android::hardware::drm::V1_1::IDrmFactory;
using android::hardware::drm::V1_1::clearkey::CryptoFactory;
using android::hardware::drm::V1_1::clearkey::DrmFactory;


int main(int /* argc */, char** /* argv */) {
    ALOGD("android.hardware.drm@1.1-service.clearkey starting...");

    // The DRM HAL may communicate to other vendor components via
    // /dev/vndbinder
    android::ProcessState::initWithDriver("/dev/vndbinder");

    sp<IDrmFactory> drmFactory = new DrmFactory;
    sp<ICryptoFactory> cryptoFactory = new CryptoFactory;

    configureRpcThreadpool(8, true /* callerWillJoin */);

    // Setup hwbinder service
    CHECK_EQ(drmFactory->registerAsService("clearkey"), android::NO_ERROR)
        << "Failed to register Clearkey Factory HAL";
    CHECK_EQ(cryptoFactory->registerAsService("clearkey"), android::NO_ERROR)
        << "Failed to register Clearkey Crypto  HAL";

    joinRpcThreadpool();
}
