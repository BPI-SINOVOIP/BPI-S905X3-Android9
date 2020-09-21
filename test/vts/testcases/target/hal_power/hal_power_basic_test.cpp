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

#include <hardware/hardware.h>
#include <hardware/power.h>
#include <utils/Log.h>

#include <iostream>

using namespace std;

namespace {

// VTS structural testcase for HAL Power basic functionalities.
class VtsStructuralTestHalPowerTest : public ::testing::Test {
 protected:
  VtsStructuralTestHalPowerTest() {
    int rc =
        hw_get_module(POWER_HARDWARE_MODULE_ID, (hw_module_t const **)&module_);
    if (rc || !module_) {
      cerr << "could not find any power HAL module." << endl;
      module_ = NULL;
      return;
    }
    if (module_->init) module_->init(module_);
  }

  virtual ~VtsStructuralTestHalPowerTest() {}

  virtual void SetUp() {
    // define operations to execute before running each testcase.
    ASSERT_TRUE(module_->get_number_of_platform_modes)
        << "get_number_of_platform_modes is NULL";
    ASSERT_TRUE(module_->get_voter_list) << "get_voter_list is NULL";
    ASSERT_TRUE(module_->get_number_of_platform_modes)
        << "get_number_of_platform_modes is NULL";
    num_modes_ = module_->get_number_of_platform_modes(module_);
    ASSERT_GT(num_modes_, 0) << "num_modes_ should be at least 1";
  }

  virtual void TearDown() {
    // define operations to execute after running each testcase.
  }

  struct power_module *module_;

  int num_modes_;
};

// get_number_of_platform_modes should always return the same value
// irrespective of how many times invoked.
TEST_F(VtsStructuralTestHalPowerTest,
       get_number_of_platform_modes_returns_same_value) {
  // NOTE: We can repeat this for few more times in a loop if the test can
  // run longer.
  sleep(1);
  int rc2 = module_->get_number_of_platform_modes(module_);
  EXPECT_EQ(num_modes_, rc2)
      << "get_number_of_platform_modes returned unequal values";
}

// Check if residency duration and count decreases between successive
// invocations.
TEST_F(VtsStructuralTestHalPowerTest, check_if_the_values_decrease) {
  power_state_platform_sleep_state_t *list1 =
      (power_state_platform_sleep_state_t *)calloc(
          num_modes_, sizeof(power_state_platform_sleep_state_t));
  ASSERT_TRUE(list1) << "power_state_platform_sleep_state_t* alloc failed";

  power_state_platform_sleep_state_t *list2 =
      (power_state_platform_sleep_state_t *)calloc(
          num_modes_, sizeof(power_state_platform_sleep_state_t));
  ASSERT_TRUE(list2) << "power_state_platform_sleep_state_t* alloc failed";

  size_t *voter_list = (size_t *)calloc(num_modes_, sizeof(*voter_list));
  ASSERT_TRUE(voter_list) << "voter_list* alloc failed";

  module_->get_voter_list(module_, voter_list);

  for (int i = 0; i < num_modes_; i++) {
    list1[i].voters = (power_state_voter_t *)calloc(
        voter_list[i], sizeof(power_state_voter_t));
    ASSERT_TRUE(list1[i].voters) << "voter_t allocation failed";

    list2[i].voters = (power_state_voter_t *)calloc(
        voter_list[i], sizeof(power_state_voter_t));

    ASSERT_TRUE(list2[i].voters) << "voter_t allocation failed";
  }

  ASSERT_EQ(0, module_->get_platform_low_power_stats(module_, list1))
      << "get_platform_low_power_stats failed";
  sleep(2);
  ASSERT_EQ(0, module_->get_platform_low_power_stats(module_, list2))
      << "get_platform_low_power_stats failed";

  for (int i = 0; i < num_modes_; i++) {
    EXPECT_LE(list1[i].residency_in_msec_since_boot,
              list2[i].residency_in_msec_since_boot)
        << "residency_in_msec_since_boot decreased";
    EXPECT_LE(list1[i].total_transitions, list2[i].total_transitions)
        << "total_transitions decreased";

    for (unsigned int j = 0; j < list1[i].number_of_voters; j++) {
      EXPECT_LE(list1[i].voters[j].total_time_in_msec_voted_for_since_boot,
                list2[i].voters[j].total_time_in_msec_voted_for_since_boot)
          << "total_time_in_msec_voted_for_since_boot decreased";
      EXPECT_LE(list1[i].voters[j].total_number_of_times_voted_since_boot,
                list2[i].voters[j].total_number_of_times_voted_since_boot)
          << "total_number_of_times_voted_since_boot decreased";
    }
  }
  for (int i = 0; i < num_modes_; i++) {
    free(list1[i].voters);
    free(list2[i].voters);
  }
  free(list1);
  free(list2);
  free(voter_list);
}

// Check if get_platform_low_power_stats succeeds.
TEST_F(VtsStructuralTestHalPowerTest,
       check_if_get_platform_low_power_stats_returns_success) {
  power_state_platform_sleep_state_t *list1 =
      (power_state_platform_sleep_state_t *)calloc(
          num_modes_, sizeof(power_state_platform_sleep_state_t));
  ASSERT_TRUE(list1) << "power_state_platform_sleep_state_t* alloc failed";

  size_t *voter_list = (size_t *)calloc(num_modes_, sizeof(*voter_list));
  ASSERT_TRUE(voter_list) << "voter_t allocation failed";

  module_->get_voter_list(module_, voter_list);
  for (int i = 0; i < num_modes_; i++) {
    list1[i].voters = (power_state_voter_t *)calloc(
        voter_list[i], sizeof(power_state_voter_t));
    ASSERT_TRUE(list1[i].voters) << "voter_t allocation failed";
  }
  EXPECT_EQ(0, module_->get_platform_low_power_stats(module_, list1))
      << "get_platform_low_power_stats failed";
  for (int i = 0; i < num_modes_; i++) {
    free(list1[i].voters);
  }
  free(list1);
  free(voter_list);
}
}  // namespace
