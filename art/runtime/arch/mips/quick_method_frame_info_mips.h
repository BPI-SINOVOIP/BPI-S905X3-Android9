/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef ART_RUNTIME_ARCH_MIPS_QUICK_METHOD_FRAME_INFO_MIPS_H_
#define ART_RUNTIME_ARCH_MIPS_QUICK_METHOD_FRAME_INFO_MIPS_H_

#include "arch/instruction_set.h"
#include "base/bit_utils.h"
#include "base/callee_save_type.h"
#include "base/enums.h"
#include "quick/quick_method_frame_info.h"
#include "registers_mips.h"

namespace art {
namespace mips {

static constexpr uint32_t kMipsCalleeSaveAlwaysSpills =
    (1u << art::mips::RA);
static constexpr uint32_t kMipsCalleeSaveRefSpills =
    (1 << art::mips::S2) | (1 << art::mips::S3) | (1 << art::mips::S4) | (1 << art::mips::S5) |
    (1 << art::mips::S6) | (1 << art::mips::S7) | (1 << art::mips::GP) | (1 << art::mips::FP);
static constexpr uint32_t kMipsCalleeSaveArgSpills =
    (1 << art::mips::A1) | (1 << art::mips::A2) | (1 << art::mips::A3) | (1 << art::mips::T0) |
    (1 << art::mips::T1);
// We want to save all floating point register pairs at addresses
// which are multiples of 8 so that we can eliminate use of the
// SDu/LDu macros by using sdc1/ldc1 to store/load floating
// register values using a single instruction. Because integer
// registers are stored at the top of the frame, to achieve having
// the floating point register pairs aligned on multiples of 8 the
// number of integer registers saved must be even. Previously, the
// only case in which we saved floating point registers beneath an
// odd number of integer registers was when "type" is
// CalleeSaveType::kSaveAllCalleeSaves. (There are other cases in
// which an odd number of integer registers are saved but those
// cases don't save any floating point registers. If no floating
// point registers are saved we don't care if the number of integer
// registers saved is odd or even). To save an even number of
// integer registers in this particular case we add the ZERO
// register to the list of registers which get saved.
static constexpr uint32_t kMipsCalleeSaveAllSpills =
    (1 << art::mips::ZERO) | (1 << art::mips::S0) | (1 << art::mips::S1);
static constexpr uint32_t kMipsCalleeSaveEverythingSpills =
    (1 << art::mips::AT) | (1 << art::mips::V0) | (1 << art::mips::V1) |
    (1 << art::mips::A0) | (1 << art::mips::A1) | (1 << art::mips::A2) | (1 << art::mips::A3) |
    (1 << art::mips::T0) | (1 << art::mips::T1) | (1 << art::mips::T2) | (1 << art::mips::T3) |
    (1 << art::mips::T4) | (1 << art::mips::T5) | (1 << art::mips::T6) | (1 << art::mips::T7) |
    (1 << art::mips::S0) | (1 << art::mips::S1) | (1 << art::mips::T8) | (1 << art::mips::T9);

static constexpr uint32_t kMipsCalleeSaveFpAlwaysSpills = 0;
static constexpr uint32_t kMipsCalleeSaveFpRefSpills = 0;
static constexpr uint32_t kMipsCalleeSaveFpArgSpills =
    (1 << art::mips::F8) | (1 << art::mips::F9) | (1 << art::mips::F10) | (1 << art::mips::F11) |
    (1 << art::mips::F12) | (1 << art::mips::F13) | (1 << art::mips::F14) | (1 << art::mips::F15) |
    (1 << art::mips::F16) | (1 << art::mips::F17) | (1 << art::mips::F18) | (1 << art::mips::F19);
static constexpr uint32_t kMipsCalleeSaveAllFPSpills =
    (1 << art::mips::F20) | (1 << art::mips::F21) | (1 << art::mips::F22) | (1 << art::mips::F23) |
    (1 << art::mips::F24) | (1 << art::mips::F25) | (1 << art::mips::F26) | (1 << art::mips::F27) |
    (1 << art::mips::F28) | (1 << art::mips::F29) | (1 << art::mips::F30) | (1u << art::mips::F31);
static constexpr uint32_t kMipsCalleeSaveFpEverythingSpills =
    (1 << art::mips::F0) | (1 << art::mips::F1) | (1 << art::mips::F2) | (1 << art::mips::F3) |
    (1 << art::mips::F4) | (1 << art::mips::F5) | (1 << art::mips::F6) | (1 << art::mips::F7) |
    (1 << art::mips::F8) | (1 << art::mips::F9) | (1 << art::mips::F10) | (1 << art::mips::F11) |
    (1 << art::mips::F12) | (1 << art::mips::F13) | (1 << art::mips::F14) | (1 << art::mips::F15) |
    (1 << art::mips::F16) | (1 << art::mips::F17) | (1 << art::mips::F18) | (1 << art::mips::F19) |
    (1 << art::mips::F20) | (1 << art::mips::F21) | (1 << art::mips::F22) | (1 << art::mips::F23) |
    (1 << art::mips::F24) | (1 << art::mips::F25) | (1 << art::mips::F26) | (1 << art::mips::F27) |
    (1 << art::mips::F28) | (1 << art::mips::F29) | (1 << art::mips::F30) | (1u << art::mips::F31);

constexpr uint32_t MipsCalleeSaveCoreSpills(CalleeSaveType type) {
  type = GetCanonicalCalleeSaveType(type);
  return kMipsCalleeSaveAlwaysSpills | kMipsCalleeSaveRefSpills |
      (type == CalleeSaveType::kSaveRefsAndArgs ? kMipsCalleeSaveArgSpills : 0) |
      (type == CalleeSaveType::kSaveAllCalleeSaves ? kMipsCalleeSaveAllSpills : 0) |
      (type == CalleeSaveType::kSaveEverything ? kMipsCalleeSaveEverythingSpills : 0);
}

constexpr uint32_t MipsCalleeSaveFPSpills(CalleeSaveType type) {
  type = GetCanonicalCalleeSaveType(type);
  return kMipsCalleeSaveFpAlwaysSpills | kMipsCalleeSaveFpRefSpills |
      (type == CalleeSaveType::kSaveRefsAndArgs ? kMipsCalleeSaveFpArgSpills : 0) |
      (type == CalleeSaveType::kSaveAllCalleeSaves ? kMipsCalleeSaveAllFPSpills : 0) |
      (type == CalleeSaveType::kSaveEverything ? kMipsCalleeSaveFpEverythingSpills : 0);
}

constexpr uint32_t MipsCalleeSaveFrameSize(CalleeSaveType type) {
  type = GetCanonicalCalleeSaveType(type);
  return RoundUp((POPCOUNT(MipsCalleeSaveCoreSpills(type)) /* gprs */ +
                  POPCOUNT(MipsCalleeSaveFPSpills(type))   /* fprs */ +
                  1 /* Method* */) * static_cast<size_t>(kMipsPointerSize), kStackAlignment);
}

constexpr QuickMethodFrameInfo MipsCalleeSaveMethodFrameInfo(CalleeSaveType type) {
  type = GetCanonicalCalleeSaveType(type);
  return QuickMethodFrameInfo(MipsCalleeSaveFrameSize(type),
                              MipsCalleeSaveCoreSpills(type),
                              MipsCalleeSaveFPSpills(type));
}

}  // namespace mips
}  // namespace art

#endif  // ART_RUNTIME_ARCH_MIPS_QUICK_METHOD_FRAME_INFO_MIPS_H_
