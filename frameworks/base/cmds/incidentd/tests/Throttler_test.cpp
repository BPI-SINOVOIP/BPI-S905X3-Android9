// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#define DEBUG false
#include "Log.h"

#include "Throttler.h"

#include <android-base/test_utils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace android::os::incidentd;

TEST(ThrottlerTest, DataSizeExceeded) {
    Throttler t(100, 100000);
    EXPECT_FALSE(t.shouldThrottle());
    t.addReportSize(200);
    EXPECT_TRUE(t.shouldThrottle());
}

TEST(ThrottlerTest, TimeReset) {
    Throttler t(100, 500);
    EXPECT_FALSE(t.shouldThrottle());
    t.addReportSize(200);
    EXPECT_TRUE(t.shouldThrottle());
    sleep(1);  // sleep for 1 second to make sure throttler resets
    EXPECT_FALSE(t.shouldThrottle());
}
