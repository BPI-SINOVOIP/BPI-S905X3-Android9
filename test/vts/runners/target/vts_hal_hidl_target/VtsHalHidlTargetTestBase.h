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

#ifndef __VTS_HAL_HIDL_TARGET_TEST_BASE_H
#define __VTS_HAL_HIDL_TARGET_TEST_BASE_H

#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

#define VTS_HAL_HIDL_GET_STUB "VTS_HAL_HIDL_GET_STUB"

using namespace std;

namespace testing {

using ::android::sp;

// VTS target side test template
class VtsHalHidlTargetTestBase : public ::testing::Test {
 public:
  /*
   * Internal test class setup function.
   */
  virtual void SetUp() override {
    ALOGI("[Test Case] %s.%s BEGIN", getTestSuiteName().c_str(),
          getTestCaseName().c_str());
    string testCaseInfo = getTestCaseInfo();
    if (testCaseInfo.size()) {
      ALOGD("Test case info: %s", testCaseInfo.c_str());
    }

    HalHidlSetUp();
  }

  /*
   * Internal test class tear-down function.
   */
  virtual void TearDown() override {
    HalHidlTearDown();

    ALOGI("[Test Case] %s.%s END", getTestSuiteName().c_str(),
          getTestCaseName().c_str());
    string testCaseInfo = getTestCaseInfo();
    if (testCaseInfo.size()) {
      ALOGD("Test case info: %s", testCaseInfo.c_str());
    }
  }

  /*
   * HAL HIDL test class setup function.
   * Will be called in the end of SetUp() function.
   */
  virtual void HalHidlSetUp() {}

  /*
   * HAL HIDL test class tear-down function.
   * Will be called in the beginning of TearDown() function.
   */
  virtual void HalHidlTearDown() {}

  /*
   * Return test case info as string.
   */
  virtual string getTestCaseInfo() const { return ""; }

  /*
   * Get value of system property as string on target
   */
  static string PropertyGet(const char* name);

  /*
   * Call interface's getService and use passthrough mode if set from host.
   */
  template <class T>
  static sp<T> getService(const string& serviceName = "default",
                          bool getStub = false) {
    return T::getService(serviceName,
                         getStub || VtsHalHidlTargetTestBase::VtsGetStub());
  }

  /*
   * Call interface's getService with the service name stored in the test
   * environment and use passthrough mode if set from host.
   */
  template <class T>
  static sp <T> getService(VtsHalHidlTargetTestEnvBase* testEnv) {
    return T::getService(testEnv->getServiceName<T>(),
                         VtsHalHidlTargetTestBase::VtsGetStub());
  }

private:
  /*
   * Decide bool val for getStub option. Will read environment variable set
   * from host. If environment variable is not set, return will default to
   * false.
   */
  static bool VtsGetStub();

  /*
   * Return test suite name as string.
   */
  string getTestSuiteName() const;

  /*
   * Return test case name as string.
   */
  string getTestCaseName() const;
};

}  // namespace testing

#endif  // __VTS_HAL_HIDL_TARGET_TEST_BASE_H
