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

#ifndef __VTS_PROTO_FUZZER_STATS_H_
#define __VTS_PROTO_FUZZER_STATS_H_

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace android {
namespace vts {
namespace fuzzer {

// Holds stats about a fuzzer run.
class ProtoFuzzerStats {
 public:
  // Registers fuzzer touching an interface function.
  void RegisterTouch(std::string iface_name, std::string func_name);
  // Returns collected stats in string form.
  std::string StatsString() const;
  // Returns the set of touched interfaces.
  const std::unordered_set<std::string> &TouchedIfaces() const {
    return touched_ifaces_;
  }

 private:
  // Counts the number of times a function was touched.
  // This object will be written to often, so we use unordered_map.
  std::unordered_map<std::string, uint64_t> touch_count_;
  // The set of interfaces that have been touched to far.
  std::unordered_set<std::string> touched_ifaces_;
};

}  // namespace fuzzer
}  // namespace vts
}  // namespace android

#endif  // __VTS_PROTO_FUZZER_STATS__
