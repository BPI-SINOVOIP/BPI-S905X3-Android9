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

#include <gtest/gtest.h>

#include <stdio.h>
#include <stdlib.h>

#include <hardware/hardware.h>
#include <hardware/lights.h>

#include <iostream>

using namespace std;

namespace {

// VTS structural testcase for HAL Lights basic functionalities.
class VtsStructuralTestHalLightsBasicTest : public ::testing::Test {
 protected:
  VtsStructuralTestHalLightsBasicTest() {
    int rc = hw_get_module_by_class(LIGHTS_HARDWARE_MODULE_ID, NULL, &module_);
    if (rc || !module_) {
      cerr << "could not find any lights HAL module." << endl;
      module_ = NULL;
      return;
    }

    rc = module_->methods->open(
        module_, LIGHT_ID_NOTIFICATIONS,
        reinterpret_cast<struct hw_device_t**>(&device_));
    if (rc || !device_) {
      cerr << "could not open a lights HAL device." << endl;
      module_ = NULL;
      return;
    }
  }

  virtual ~VtsStructuralTestHalLightsBasicTest() {}

  virtual void SetUp() {
    // define operations to execute before running each testcase.
  }

  virtual void TearDown() {
    // define operations to execute after running each testcase.
  }

  // a light device which is open.
  struct light_device_t* device_;

 private:
  const struct hw_module_t* module_;
};

TEST_F(VtsStructuralTestHalLightsBasicTest, example) {
  ASSERT_TRUE(device_);
  struct light_state_t* arg =
      (struct light_state_t*)malloc(sizeof(struct light_state_t));
  arg->color = 0x80ff8000;
  arg->flashMode = LIGHT_FLASH_NONE;
  arg->flashOnMS = 0;
  arg->flashOffMS = 0;
  arg->brightnessMode = BRIGHTNESS_MODE_USER;
  EXPECT_EQ(0, device_->set_light(device_, arg));
}

}  // namespace
