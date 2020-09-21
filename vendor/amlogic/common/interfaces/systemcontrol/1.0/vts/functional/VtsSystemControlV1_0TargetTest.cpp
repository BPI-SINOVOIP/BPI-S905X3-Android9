/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "VtsSystemControlV1_0TargetTest"

#include <vendor/amlogic/hardware/systemcontrol/1.0/ISystemControl.h>
#include <vendor/amlogic/hardware/systemcontrol/1.0/types.h>
#include <log/log.h>
#include <VtsHalHidlTargetTestBase.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <inttypes.h>
#include <thread>

using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControl;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;


#define ASSERT_OK(ret) ASSERT_TRUE((ret).isOk())

class SystemControlTest : public ::testing::VtsHalHidlTargetTestBase {
public:
    ~SystemControlTest() {}

    virtual void SetUp() override {
        service = ::testing::VtsHalHidlTargetTestBase::getService<ISystemControl>();

        ASSERT_NE(service, nullptr);
    }

    virtual void TearDown() override {
    }

    sp<ISystemControl> service;
};

/**
 * No vsync events should happen unless you explicitly request one.
 */
TEST_F(SystemControlTest, getSupportDispModeList) {
    service->getSupportDispModeList([](const Result &ret, const hidl_vec<hidl_string>& modeList) {
        if (Result::OK == ret) {
        }

        hidl_vec<hidl_string> list = modeList;
        list = list;
    });
}

/**
 * Vsync rate respects count.
 */
TEST_F(SystemControlTest, getActiveDispMode) {
    service->getActiveDispMode([](const Result &ret, const hidl_string& mode) {
        if (Result::OK == ret) {
        }
        hidl_string m = mode;
        m = m;
    });

    ALOGE("SystemControlTest getActiveDispMode");
}

/**
 * Open/close should return proper error results.
 */
TEST_F(SystemControlTest, setActiveDispMode) {
    hidl_string mode("1080p60hz");
    Result ret = service->setActiveDispMode(mode);
    if (Result::OK == ret) {
    }

    ALOGE("SystemControlTest setActiveDispMode");
}

/**
 * Vsync must be given a value that is >= 0.
 */
TEST_F(SystemControlTest, isHDCPTxAuthSuccess) {
    Result ret = service->isHDCPTxAuthSuccess();
    if (Result::OK == ret) {
    }

    ALOGE("SystemControlTest isHDCPTxAuthSuccess");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test status = %d", status);
    return status;
}
