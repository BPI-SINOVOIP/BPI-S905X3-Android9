/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef ART_RUNTIME_ARCH_CONTEXT_H_
#define ART_RUNTIME_ARCH_CONTEXT_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/mutex.h"

namespace art {

class QuickMethodFrameInfo;

// Representation of a thread's context on the executing machine, used to implement long jumps in
// the quick stack frame layout.
class Context {
 public:
  // Creates a context for the running architecture
  static Context* Create();

  virtual ~Context() {}

  // Re-initializes the registers for context re-use.
  virtual void Reset() = 0;

  static uintptr_t* CalleeSaveAddress(uint8_t* frame, int num, size_t frame_size) {
    // Callee saves are held at the top of the frame
    uint8_t* save_addr = frame + frame_size - ((num + 1) * sizeof(void*));
#if defined(__i386__) || defined(__x86_64__)
    save_addr -= sizeof(void*);  // account for return address
#endif
    return reinterpret_cast<uintptr_t*>(save_addr);
  }

  // Reads values from callee saves in the given frame. The frame also holds
  // the method that holds the layout.
  virtual void FillCalleeSaves(uint8_t* frame, const QuickMethodFrameInfo& fr) = 0;

  // Sets the stack pointer value.
  virtual void SetSP(uintptr_t new_sp) = 0;

  // Sets the program counter value.
  virtual void SetPC(uintptr_t new_pc) = 0;

  // Sets the first argument register.
  virtual void SetArg0(uintptr_t new_arg0_value) = 0;

  // Returns whether the given GPR is accessible (read or write).
  virtual bool IsAccessibleGPR(uint32_t reg) = 0;

  // Gets the given GPRs address.
  virtual uintptr_t* GetGPRAddress(uint32_t reg) = 0;

  // Reads the given GPR. The caller is responsible for checking the register
  // is accessible with IsAccessibleGPR.
  virtual uintptr_t GetGPR(uint32_t reg) = 0;

  // Sets the given GPR. The caller is responsible for checking the register
  // is accessible with IsAccessibleGPR.
  virtual void SetGPR(uint32_t reg, uintptr_t value) = 0;

  // Returns whether the given FPR is accessible (read or write).
  virtual bool IsAccessibleFPR(uint32_t reg) = 0;

  // Reads the given FPR. The caller is responsible for checking the register
  // is accessible with IsAccessibleFPR.
  virtual uintptr_t GetFPR(uint32_t reg) = 0;

  // Sets the given FPR. The caller is responsible for checking the register
  // is accessible with IsAccessibleFPR.
  virtual void SetFPR(uint32_t reg, uintptr_t value) = 0;

  // Smashes the caller save registers. If we're throwing, we don't want to return bogus values.
  virtual void SmashCallerSaves() = 0;

  // Switches execution of the executing context to this context
  NO_RETURN virtual void DoLongJump() = 0;

  enum {
    kBadGprBase = 0xebad6070,
    kBadFprBase = 0xebad8070,
  };
};

}  // namespace art

#endif  // ART_RUNTIME_ARCH_CONTEXT_H_
