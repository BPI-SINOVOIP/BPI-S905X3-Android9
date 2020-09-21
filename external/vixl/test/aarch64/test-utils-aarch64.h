// Copyright 2014, VIXL authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VIXL_AARCH64_TEST_UTILS_AARCH64_H_
#define VIXL_AARCH64_TEST_UTILS_AARCH64_H_

#include "test-runner.h"

#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/macro-assembler-aarch64.h"
#include "aarch64/simulator-aarch64.h"

namespace vixl {
namespace aarch64 {

// Signalling and quiet NaNs in double format, constructed such that the bottom
// 32 bits look like a signalling or quiet NaN (as appropriate) when interpreted
// as a float. These values are not architecturally significant, but they're
// useful in tests for initialising registers.
extern const double kFP64SignallingNaN;
extern const double kFP64QuietNaN;

// Signalling and quiet NaNs in float format.
extern const float kFP32SignallingNaN;
extern const float kFP32QuietNaN;

// Structure representing Q registers in a RegisterDump.
struct vec128_t {
  uint64_t l;
  uint64_t h;
};

// RegisterDump: Object allowing integer, floating point and flags registers
// to be saved to itself for future reference.
class RegisterDump {
 public:
  RegisterDump() : completed_(false) {
    VIXL_ASSERT(sizeof(dump_.d_[0]) == kDRegSizeInBytes);
    VIXL_ASSERT(sizeof(dump_.s_[0]) == kSRegSizeInBytes);
    VIXL_ASSERT(sizeof(dump_.d_[0]) == kXRegSizeInBytes);
    VIXL_ASSERT(sizeof(dump_.s_[0]) == kWRegSizeInBytes);
    VIXL_ASSERT(sizeof(dump_.x_[0]) == kXRegSizeInBytes);
    VIXL_ASSERT(sizeof(dump_.w_[0]) == kWRegSizeInBytes);
    VIXL_ASSERT(sizeof(dump_.q_[0]) == kQRegSizeInBytes);
  }

  // The Dump method generates code to store a snapshot of the register values.
  // It needs to be able to use the stack temporarily, and requires that the
  // current stack pointer is sp, and is properly aligned.
  //
  // The dumping code is generated though the given MacroAssembler. No registers
  // are corrupted in the process, but the stack is used briefly. The flags will
  // be corrupted during this call.
  void Dump(MacroAssembler* assm);

  // Register accessors.
  inline int32_t wreg(unsigned code) const {
    if (code == kSPRegInternalCode) {
      return wspreg();
    }
    VIXL_ASSERT(RegAliasesMatch(code));
    return dump_.w_[code];
  }

  inline int64_t xreg(unsigned code) const {
    if (code == kSPRegInternalCode) {
      return spreg();
    }
    VIXL_ASSERT(RegAliasesMatch(code));
    return dump_.x_[code];
  }

  // FPRegister accessors.
  inline uint32_t sreg_bits(unsigned code) const {
    VIXL_ASSERT(FPRegAliasesMatch(code));
    return dump_.s_[code];
  }

  inline float sreg(unsigned code) const {
    return RawbitsToFloat(sreg_bits(code));
  }

  inline uint64_t dreg_bits(unsigned code) const {
    VIXL_ASSERT(FPRegAliasesMatch(code));
    return dump_.d_[code];
  }

  inline double dreg(unsigned code) const {
    return RawbitsToDouble(dreg_bits(code));
  }

  inline vec128_t qreg(unsigned code) const { return dump_.q_[code]; }

  // Stack pointer accessors.
  inline int64_t spreg() const {
    VIXL_ASSERT(SPRegAliasesMatch());
    return dump_.sp_;
  }

  inline int32_t wspreg() const {
    VIXL_ASSERT(SPRegAliasesMatch());
    return static_cast<int32_t>(dump_.wsp_);
  }

  // Flags accessors.
  inline uint32_t flags_nzcv() const {
    VIXL_ASSERT(IsComplete());
    VIXL_ASSERT((dump_.flags_ & ~Flags_mask) == 0);
    return dump_.flags_ & Flags_mask;
  }

  inline bool IsComplete() const { return completed_; }

 private:
  // Indicate whether the dump operation has been completed.
  bool completed_;

  // Check that the lower 32 bits of x<code> exactly match the 32 bits of
  // w<code>. A failure of this test most likely represents a failure in the
  // ::Dump method, or a failure in the simulator.
  bool RegAliasesMatch(unsigned code) const {
    VIXL_ASSERT(IsComplete());
    VIXL_ASSERT(code < kNumberOfRegisters);
    return ((dump_.x_[code] & kWRegMask) == dump_.w_[code]);
  }

  // As RegAliasesMatch, but for the stack pointer.
  bool SPRegAliasesMatch() const {
    VIXL_ASSERT(IsComplete());
    return ((dump_.sp_ & kWRegMask) == dump_.wsp_);
  }

  // As RegAliasesMatch, but for floating-point registers.
  bool FPRegAliasesMatch(unsigned code) const {
    VIXL_ASSERT(IsComplete());
    VIXL_ASSERT(code < kNumberOfFPRegisters);
    return (dump_.d_[code] & kSRegMask) == dump_.s_[code];
  }

  // Store all the dumped elements in a simple struct so the implementation can
  // use offsetof to quickly find the correct field.
  struct dump_t {
    // Core registers.
    uint64_t x_[kNumberOfRegisters];
    uint32_t w_[kNumberOfRegisters];

    // Floating-point registers, as raw bits.
    uint64_t d_[kNumberOfFPRegisters];
    uint32_t s_[kNumberOfFPRegisters];

    // Vector registers.
    vec128_t q_[kNumberOfVRegisters];

    // The stack pointer.
    uint64_t sp_;
    uint64_t wsp_;

    // NZCV flags, stored in bits 28 to 31.
    // bit[31] : Negative
    // bit[30] : Zero
    // bit[29] : Carry
    // bit[28] : oVerflow
    uint64_t flags_;
  } dump_;
};

// Some of these methods don't use the RegisterDump argument, but they have to
// accept them so that they can overload those that take register arguments.
bool Equal32(uint32_t expected, const RegisterDump*, uint32_t result);
bool Equal64(uint64_t expected, const RegisterDump*, uint64_t result);

bool EqualFP32(float expected, const RegisterDump*, float result);
bool EqualFP64(double expected, const RegisterDump*, double result);

bool Equal32(uint32_t expected, const RegisterDump* core, const Register& reg);
bool Equal64(uint64_t expected, const RegisterDump* core, const Register& reg);
bool Equal64(uint64_t expected,
             const RegisterDump* core,
             const VRegister& vreg);

bool EqualFP32(float expected,
               const RegisterDump* core,
               const FPRegister& fpreg);
bool EqualFP64(double expected,
               const RegisterDump* core,
               const FPRegister& fpreg);

bool Equal64(const Register& reg0,
             const RegisterDump* core,
             const Register& reg1);
bool Equal128(uint64_t expected_h,
              uint64_t expected_l,
              const RegisterDump* core,
              const VRegister& reg);

bool EqualNzcv(uint32_t expected, uint32_t result);

bool EqualRegisters(const RegisterDump* a, const RegisterDump* b);

// Populate the w, x and r arrays with registers from the 'allowed' mask. The
// r array will be populated with <reg_size>-sized registers,
//
// This allows for tests which use large, parameterized blocks of registers
// (such as the push and pop tests), but where certain registers must be
// avoided as they are used for other purposes.
//
// Any of w, x, or r can be NULL if they are not required.
//
// The return value is a RegList indicating which registers were allocated.
RegList PopulateRegisterArray(Register* w,
                              Register* x,
                              Register* r,
                              int reg_size,
                              int reg_count,
                              RegList allowed);

// As PopulateRegisterArray, but for floating-point registers.
RegList PopulateFPRegisterArray(FPRegister* s,
                                FPRegister* d,
                                FPRegister* v,
                                int reg_size,
                                int reg_count,
                                RegList allowed);

// Ovewrite the contents of the specified registers. This enables tests to
// check that register contents are written in cases where it's likely that the
// correct outcome could already be stored in the register.
//
// This always overwrites X-sized registers. If tests are operating on W
// registers, a subsequent write into an aliased W register should clear the
// top word anyway, so clobbering the full X registers should make tests more
// rigorous.
void Clobber(MacroAssembler* masm,
             RegList reg_list,
             uint64_t const value = 0xfedcba9876543210);

// As Clobber, but for FP registers.
void ClobberFP(MacroAssembler* masm,
               RegList reg_list,
               double const value = kFP64SignallingNaN);

// As Clobber, but for a CPURegList with either FP or integer registers. When
// using this method, the clobber value is always the default for the basic
// Clobber or ClobberFP functions.
void Clobber(MacroAssembler* masm, CPURegList reg_list);

}  // namespace aarch64
}  // namespace vixl

#endif  // VIXL_AARCH64_TEST_UTILS_AARCH64_H_
