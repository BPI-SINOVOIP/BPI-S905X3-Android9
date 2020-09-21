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

#include <MockKeymaster.client.h>
#include <KeymasterDevice.h>

#include <gtest/gtest.h>

#include <string>

using ::std::string;

using ::testing::Eq;
using ::testing::Return;

// Hardware
using ::android::hardware::keymaster::KeymasterDevice;
using ::android::hardware::keymaster::KeymasterDevice;

// HAL
using ::android::hardware::keymaster::V4_0::SecurityLevel;

// App
using ::nugget::app::keymaster::MockKeymaster;

// GetHardwareInfo

TEST(KeymasterHalTest, getHardwareInfoReturnsDefaults) {
    MockKeymaster mockService;
    KeymasterDevice hal{mockService};
    hal.getHardwareInfo([&](SecurityLevel securityLevel,
                            string keymasterName,
                            string keymasterAuthorName) {
        EXPECT_EQ(securityLevel, SecurityLevel::STRONGBOX);
        EXPECT_THAT(keymasterName, Eq("CitadelKeymaster"));
        EXPECT_THAT(keymasterAuthorName, Eq("Google"));
    });
}
