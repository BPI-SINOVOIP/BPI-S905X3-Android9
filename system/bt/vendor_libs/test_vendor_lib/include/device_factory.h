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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "device.h"

namespace test_vendor_lib {

// Encapsulate the details of supported devices to hide them from the
// Controller.
class DeviceFactory {
 public:
  DeviceFactory();
  virtual ~DeviceFactory() = default;

  // Call the constructor for the matching device type (arg[0]) and then call
  // the matching Initialize() function with args.
  static std::shared_ptr<Device> Create(const std::vector<std::string>& args);
};

}  // namespace test_vendor_lib
