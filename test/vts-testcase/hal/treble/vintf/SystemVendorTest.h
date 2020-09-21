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

#ifndef VTS_TREBLE_VINTF_TEST_SYSTEM_VENDOR_TEST_H_
#define VTS_TREBLE_VINTF_TEST_SYSTEM_VENDOR_TEST_H_
#include "VtsTrebleVintfTestBase.h"

#include <gtest/gtest.h>

namespace android {
namespace vintf {
namespace testing {

// Tests the compatibility / deprecation / etc. between framework and
// device manifest.
class SystemVendorTest : public VtsTrebleVintfTestBase {
 public:
  virtual ~SystemVendorTest() {}
  virtual void SetUp() override {
    VtsTrebleVintfTestBase::SetUp();

    vendor_manifest_ = VintfObject::GetDeviceHalManifest();
    ASSERT_NE(vendor_manifest_, nullptr)
        << "Failed to get vendor HAL manifest." << endl;

    fwk_manifest_ = VintfObject::GetFrameworkHalManifest();
    ASSERT_NE(fwk_manifest_, nullptr)
        << "Failed to get framework HAL manifest." << endl;
  }

  HalManifestPtr vendor_manifest_;
  HalManifestPtr fwk_manifest_;
};

}  // namespace testing
}  // namespace vintf
}  // namespace android

#endif  // VTS_TREBLE_VINTF_TEST_SYSTEM_VENDOR_TEST_H_
