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

#ifndef VTS_TREBLE_VINTF_TEST_BASE_H_
#define VTS_TREBLE_VINTF_TEST_BASE_H_

#include <string>
#include <vector>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <gtest/gtest.h>
#include <vintf/VintfObject.h>

#include "utils.h"

namespace android {
namespace vintf {
namespace testing {

using android::sp;
using android::hidl::base::V1_0::IBase;
using android::hidl::manager::V1_0::IServiceManager;

// Base class for many test suites. Provides some utility functions.
class VtsTrebleVintfTestBase : public ::testing::Test {
 public:
  virtual ~VtsTrebleVintfTestBase() {}
  virtual void SetUp() override;

  // Applies given function to each HAL instance in VINTF.
  static void ForEachHalInstance(const HalManifestPtr &, HalVerifyFn);
  // Retrieves an existing HAL service.
  static sp<IBase> GetHalService(const string &fq_name,
                                 const string &instance_name, Transport,
                                 bool log = true);
  static sp<IBase> GetHalService(const FQName &fq_name,
                                 const string &instance_name, Transport,
                                 bool log = true);

  static vector<string> GetInstanceNames(const sp<IServiceManager> &manager,
                                         const FQName &fq_name);

  static vector<string> GetInterfaceChain(const sp<IBase> &service);

  static set<string> GetPassthroughHals(HalManifestPtr manifest);
  static set<string> GetHwbinderHals(HalManifestPtr manifest);
  Partition GetPartition(sp<IBase> hal_service);

  // Default service manager.
  sp<IServiceManager> default_manager_;
};

}  // namespace testing
}  // namespace vintf
}  // namespace android

#endif  // VTS_TREBLE_VINTF_TEST_BASE_H_
