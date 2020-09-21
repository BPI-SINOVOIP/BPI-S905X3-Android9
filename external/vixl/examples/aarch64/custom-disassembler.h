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

#ifndef VIXL_EXAMPLES_CUSTOM_DISASSEMBLER_H_
#define VIXL_EXAMPLES_CUSTOM_DISASSEMBLER_H_

#include "aarch64/disasm-aarch64.h"

using namespace vixl::aarch64;

void TestCustomDisassembler();

// We want to change three things in the disassembly:
// - Add comments to some add/sub instructions.
// - Use aliases for register names.
// - Add descriptions for code addresses.
class CustomDisassembler : public Disassembler {
 public:
  CustomDisassembler() : Disassembler() {}
  virtual ~CustomDisassembler() {}

  virtual void VisitAddSubShifted(const Instruction* instr) VIXL_OVERRIDE;

 protected:
  virtual void AppendRegisterNameToOutput(const Instruction* instr,
                                          const CPURegister& reg) VIXL_OVERRIDE;

  virtual void AppendCodeRelativeCodeAddressToOutput(
      const Instruction* instr, const void* addr) VIXL_OVERRIDE;
};


#endif
