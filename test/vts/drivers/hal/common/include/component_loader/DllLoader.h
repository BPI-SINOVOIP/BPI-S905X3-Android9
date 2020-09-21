/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef __VTS_SYSFUZZER_COMMON_COMPONENTLOADER_DLLLOADER_H__
#define __VTS_SYSFUZZER_COMMON_COMPONENTLOADER_DLLLOADER_H__

#include "hardware/hardware.h"

namespace android {
namespace vts {

class DriverBase;

// Pointer type for a function in a loaded component.
typedef DriverBase* (*loader_function)();
typedef DriverBase* (*loader_function_with_arg)(uint64_t arg);
typedef void (*writeout_fn)();
typedef void (*flush_fn)();

// Component loader implementation for a DLL file.
class DllLoader {
 public:
  DllLoader();
  virtual ~DllLoader();

  // Loads a DLL file.
  // Returns a handle (void *) if successful; NULL otherwise.
  void* Load(const char* file_path);

  // Finds and returns a requested function defined in the loaded file.
  // Returns NULL if not found.
  loader_function GetLoaderFunction(const char* function_name) const;
  loader_function_with_arg GetLoaderFunctionWithArg(
      const char* function_name) const;

  // (for sancov) Reset coverage data.
  bool SancovResetCoverage();

  // (for gcov) initialize.
  bool GcovInit(writeout_fn wfn, flush_fn ffn);

  // (for gcov) flush to file(s).
  bool GcovFlush();

 private:
  // pointer to a handle of the loaded DLL file.
  void* handle_;

  // Loads a symbol and prints error message.
  // Returns the symbol value if successful; NULL otherwise.
  void* LoadSymbol(const char* symbol_name) const;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMMON_COMPONENTLOADER_DLLLOADER_H__
