/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "device"

#include <vector>

#include "device.h"

using std::vector;

namespace test_vendor_lib {

std::string Device::ToString() const {
  std::string dev = "(" + GetTypeString() + ")" + "@" + address_.ToString();

  return dev;
}

bool Device::IsAdvertisementAvailable(
    std::chrono::milliseconds scan_time) const {
  if (advertising_interval_ms_ == std::chrono::milliseconds(0)) return false;

  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

  std::chrono::steady_clock::time_point last_interval =
      ((now - time_stamp_) / advertising_interval_ms_) *
          advertising_interval_ms_ +
      time_stamp_;

  std::chrono::steady_clock::time_point next_interval =
      last_interval + advertising_interval_ms_;

  return ((now + scan_time) >= next_interval);
}

bool Device::IsPageScanAvailable() const { return true; }

}  // namespace test_vendor_lib
