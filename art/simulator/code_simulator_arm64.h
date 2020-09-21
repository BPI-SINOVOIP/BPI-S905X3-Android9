/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef ART_SIMULATOR_CODE_SIMULATOR_ARM64_H_
#define ART_SIMULATOR_CODE_SIMULATOR_ARM64_H_

#include "memory"

// TODO(VIXL): Make VIXL compile with -Wshadow.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "aarch64/simulator-aarch64.h"
#pragma GCC diagnostic pop

#include "arch/instruction_set.h"
#include "code_simulator.h"

namespace art {
namespace arm64 {

class CodeSimulatorArm64 : public CodeSimulator {
 public:
  static CodeSimulatorArm64* CreateCodeSimulatorArm64();
  virtual ~CodeSimulatorArm64();

  void RunFrom(intptr_t code_buffer) OVERRIDE;

  bool GetCReturnBool() const OVERRIDE;
  int32_t GetCReturnInt32() const OVERRIDE;
  int64_t GetCReturnInt64() const OVERRIDE;

 private:
  CodeSimulatorArm64();

  vixl::aarch64::Decoder* decoder_;
  vixl::aarch64::Simulator* simulator_;

  // TODO: Enable CodeSimulatorArm64 for more host ISAs once Simulator supports them.
  static constexpr bool kCanSimulate = (kRuntimeISA == InstructionSet::kX86_64);

  DISALLOW_COPY_AND_ASSIGN(CodeSimulatorArm64);
};

}  // namespace arm64
}  // namespace art

#endif  // ART_SIMULATOR_CODE_SIMULATOR_ARM64_H_
