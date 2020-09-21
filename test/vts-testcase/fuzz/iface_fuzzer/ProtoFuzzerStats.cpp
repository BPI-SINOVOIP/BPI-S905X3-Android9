/*
 * Copyright 2017 The Android Open Source Project
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

#include "ProtoFuzzerStats.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

using std::endl;
using std::map;
using std::string;
using std::unordered_map;

namespace android {
namespace vts {
namespace fuzzer {

void ProtoFuzzerStats::RegisterTouch(string iface_name, string func_name) {
  // Update the touch count for the full function name.
  string key = iface_name + "::" + func_name;
  touch_count_[key]++;
  // Record that this interface has been touched.
  touched_ifaces_.insert(std::move(iface_name));
}

string ProtoFuzzerStats::StatsString() const {
  std::map<string, uint64_t> ordered_result{touch_count_.cbegin(),
                                            touch_count_.cend()};

  std::stringstream ss{};
  ss << "HAL api function touch count: " << endl;
  for (const auto &entry : ordered_result) {
    ss << std::left << std::setfill(' ') << std::setw(40) << entry.first
       << std::setw(40) << entry.second << endl;
  }
  ss << endl;
  return ss.str();
}

}  // namespace fuzzer
}  // namespace vts
}  // namespace android
