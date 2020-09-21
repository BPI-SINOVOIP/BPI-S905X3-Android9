/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <DeviceInfo.h>

#include <gtest/gtest.h>
#include "tests/common/TestUtils.h"

using namespace android;
using namespace android::uirenderer;

OPENGL_PIPELINE_TEST(DeviceInfo, basic) {
    // can't assert state before init - another test may have initialized the singleton
    DeviceInfo::initialize();
    const DeviceInfo* di = DeviceInfo::get();
    ASSERT_NE(nullptr, di) << "DeviceInfo initialization failed";
    EXPECT_EQ(2048, di->maxTextureSize()) << "Max texture size didn't match";
}
