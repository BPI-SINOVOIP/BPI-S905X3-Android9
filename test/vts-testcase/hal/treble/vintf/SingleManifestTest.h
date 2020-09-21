/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef VTS_TREBLE_VINTF_TEST_SINGLE_MANIFEST_TEST_H_
#define VTS_TREBLE_VINTF_TEST_SINGLE_MANIFEST_TEST_H_
#include <gtest/gtest.h>
#include "VtsTrebleVintfTestBase.h"

namespace android {
namespace vintf {
namespace testing {

// A parameterized test for common tests on device and framework manifest.
class SingleManifestTest
    : public VtsTrebleVintfTestBase,
      public ::testing::WithParamInterface<HalManifestPtr> {
 public:
  virtual ~SingleManifestTest() {}
};

}  // namespace testing
}  // namespace vintf
}  // namespace android

#endif  // VTS_TREBLE_VINTF_TEST_SINGLE_MANIFEST_TEST_H_
