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

#include "VtsHalHidlTargetTestBase.h"

#include <sys/system_properties.h>

using namespace std;

namespace testing {

string VtsHalHidlTargetTestBase::PropertyGet(const char* name) {
  char value[PROP_VALUE_MAX] = {0};
  __system_property_get(name, value);
  return value;
}

bool VtsHalHidlTargetTestBase::VtsGetStub() {
  const char* env_value = getenv(VTS_HAL_HIDL_GET_STUB);
  if (env_value == NULL) {
    // VTS_HAL_HIDL_GET_STUB is not set in environment
    return false;
  }

  std::string value(env_value);
  if (value.compare("true") || value.compare("True") || value.compare("1")) {
    // Use passthrough mode
    return true;
  }

  return false;
}

string VtsHalHidlTargetTestBase::getTestCaseName() const {
  return UnitTest::GetInstance()->current_test_info()->test_case_name();
}

string VtsHalHidlTargetTestBase::getTestSuiteName() const {
  return UnitTest::GetInstance()->current_test_info()->name();
}

}  // namespace testing
