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

#ifndef __VTS_PROTO_FUZZER_RUNNER_H_
#define __VTS_PROTO_FUZZER_RUNNER_H_

#include "ProtoFuzzerStats.h"
#include "ProtoFuzzerUtils.h"

#include <memory>

namespace android {
namespace vts {
namespace fuzzer {

// Describes a HIDL HAL interface.
struct IfaceDesc {
  // VTS spec of the interface.
  const CompSpec *comp_spec_;
  // Handle to an interface instance.
  std::shared_ptr<DriverBase> hal_;
};

using IfaceDescTbl = std::unordered_map<std::string, IfaceDesc>;

// Responsible for issuing function calls to the HAL and keeps track of
// HAL-related information, e.g. which interfaces has been opened so far.
class ProtoFuzzerRunner {
 public:
  ProtoFuzzerRunner(const std::vector<CompSpec> &comp_specs);

  // Initializes interface descriptor table by opening the root interface.
  void Init(const std::string &, bool);
  // Call every API from call sequence specified by the ExecSpec.
  void Execute(const ExecSpec &);
  // Execute the specified interface function call.
  void Execute(const FuncCall &);
  // Accessor to interface descriptor table containing currently opened
  // interfaces.
  const IfaceDescTbl &GetOpenedIfaces() const { return opened_ifaces_; }
  // Accessor to stats object.
  const ProtoFuzzerStats &GetStats() const { return stats_; }
  // Returns true iff there are opened interfaces that are untouched.
  bool UntouchedIfaces() const {
    return opened_ifaces_.size() > stats_.TouchedIfaces().size();
  }

 private:
  // Looks up interface spec by name.
  const CompSpec *FindCompSpec(std::string);
  // Processes return value from a function call.
  void ProcessReturnValue(const FuncSpec &result);
  // Loads the interface corresponding to the given VTS spec. Interface is
  // constructed with the given argument.
  DriverBase *LoadInterface(const CompSpec &, uint64_t);

  // Keeps track of opened interfaces.
  IfaceDescTbl opened_ifaces_;
  // All loaded VTS specs indexed by name.
  std::unordered_map<std::string, CompSpec> comp_specs_;
  // Handle to the driver library.
  void *driver_handle_;

  // Collects statistical information.
  ProtoFuzzerStats stats_;
};

}  // namespace fuzzer
}  // namespace vts
}  // namespace android

#endif  // __VTS_PROTO_FUZZER_RUNNER_H__
