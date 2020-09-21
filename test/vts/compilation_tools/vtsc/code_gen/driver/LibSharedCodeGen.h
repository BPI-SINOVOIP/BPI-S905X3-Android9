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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_LIBSHARED_CODEGEN_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_LIBSHARED_CODEGEN_H_

#include <string>

#include "code_gen/driver/HalCodeGen.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class LibSharedCodeGen : public HalCodeGen {
 public:
  LibSharedCodeGen(const char* input_vts_file_path)
      : HalCodeGen(input_vts_file_path) {}

 protected:
  void GenerateCppBodyFuzzFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateCppBodyGetAttributeFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateClassConstructionFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  // instance variable name (e.g., submodule_);
  static const char* const kInstanceVariableName;
};

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_LIBSHARED_CODEGEN_H_
