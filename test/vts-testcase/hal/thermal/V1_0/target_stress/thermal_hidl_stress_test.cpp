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

#define LOG_TAG "thermal_hidl_target_stress_test"

#include <android-base/logging.h>
#include <android/hardware/thermal/1.0/IThermal.h>
#include <android/hardware/thermal/1.0/types.h>

#include <gtest/gtest.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

using ::android::hardware::hidl_vec;
using ::android::hardware::thermal::V1_0::CoolingDevice;
using ::android::hardware::thermal::V1_0::CpuUsage;
using ::android::hardware::thermal::V1_0::IThermal;
using ::android::hardware::thermal::V1_0::Temperature;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::sp;

#define NUMBER_LOOPS 100
#define TIMEOUT_PERIOD 1

class ThermalHidlStressTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    thermal_ = IThermal::getService();
    ASSERT_NE(thermal_, nullptr);
    count_ = 0;
  }

  /* Test code calls this function to wait for callback */
  inline std::cv_status wait() {
    std::unique_lock<std::mutex> lock(mtx_);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count_ == 0) {
      status = cv_.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
      if (status == std::cv_status::timeout) return status;
    }
    count_--;
    return status;
  }

 protected:
  sp<IThermal> thermal_;
  std::mutex mtx_;
  std::condition_variable cv_;
  int count_;
};

/* Stress test for Thermal::getTemperatures(). */
TEST_F(ThermalHidlStressTest, stressTemperatures) {
  for (int loops = 0; loops < NUMBER_LOOPS; loops++) {
    thermal_->getTemperatures(
        [&](ThermalStatus status, hidl_vec<Temperature> /* temperatures */) {
          EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
          /* Inform the test about callback. */
          std::unique_lock<std::mutex> lock(mtx_);
          count_++;
          cv_.notify_one();
        });
    EXPECT_EQ(std::cv_status::no_timeout, wait());
  }
}

/* Stress test for Thermal::getCpuUsages(). */
TEST_F(ThermalHidlStressTest, stressCpuUsages) {
  for (int loops = 0; loops < NUMBER_LOOPS; loops++) {
    thermal_->getCpuUsages(
        [&](ThermalStatus status, hidl_vec<CpuUsage> /* cpuUsages */) {
          EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
          /* Inform the test about callback. */
          std::unique_lock<std::mutex> lock(mtx_);
          count_++;
          cv_.notify_one();
        });
    EXPECT_EQ(std::cv_status::no_timeout, wait());
  }
}

/* Stress test for Thermal::getCoolingDevices(). */
TEST_F(ThermalHidlStressTest, stressCoolingDevices) {
  for (int loops = 0; loops < NUMBER_LOOPS; loops++) {
    thermal_->getCoolingDevices(
        [&](ThermalStatus status, hidl_vec<CoolingDevice> /* coolingDevice */) {
          EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
          /* Inform the test about callback. */
          std::unique_lock<std::mutex> lock(mtx_);
          count_++;
          cv_.notify_one();
        });
    EXPECT_EQ(std::cv_status::no_timeout, wait());
  }
}
