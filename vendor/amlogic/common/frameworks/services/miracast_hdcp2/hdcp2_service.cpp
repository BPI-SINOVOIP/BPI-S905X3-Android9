/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "vendor.amlogic.miracast_hdcp2@1.0-service"

#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPService.h>
#include <hidl/HidlTransportSupport.h>

#include "HDCPService.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using vendor::amlogic::hardware::miracast_hdcp2::V1_0::IHDCPService;
using vendor::amlogic::hardware::miracast_hdcp2::V1_0::implementation::HDCPService;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(1, true);

    sp<IHDCPService> hdcpService = new HDCPService;
    status_t status = hdcpService->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register IHDCPService");

    joinRpcThreadpool();
    return 0;
}
