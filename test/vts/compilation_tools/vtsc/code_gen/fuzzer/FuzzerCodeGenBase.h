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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_FUZZER_FUZZERCODEGENBASE_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_FUZZER_FUZZERCODEGENBASE_H_

#include <hidl-util/Formatter.h>
#include <iostream>
#include <string>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

namespace android {
namespace vts {

// Base class generates LLVM libfuzzer code for HAL interfaces.
// Takes VTS spec in the form of a ComponentSpecificationMessage and generates
// one source file per interface function in the spec.
// All fuzzer code generators should derive from this class.
class FuzzerCodeGenBase {
 public:
  FuzzerCodeGenBase(const ComponentSpecificationMessage &comp_spec)
      : comp_spec_(comp_spec) {}

  virtual ~FuzzerCodeGenBase(){};

  // Generates all files.
  void GenerateAll(Formatter &header_out, Formatter &source_out);
  // Generates fuzzer header file.
  void GenerateHeaderFile(Formatter &out);
  // Generates fuzzer source file.
  void GenerateSourceFile(Formatter &out);

 protected:
  // Generates "#include" declarations.
  virtual void GenerateSourceIncludeFiles(Formatter &out) = 0;
  // Generates "using" declarations.
  virtual void GenerateUsingDeclaration(Formatter &out) = 0;
  // Generates global variable declarations.
  virtual void GenerateGlobalVars(Formatter &out) = 0;
  // Generates definition of LLVMFuzzerInitialize function.
  virtual void GenerateLLVMFuzzerInitialize(Formatter &out) = 0;
  // Generates definition of LLVMFuzzerTestOneInput function.
  virtual void GenerateLLVMFuzzerTestOneInput(Formatter &out) = 0;
  virtual void GenerateOpenNameSpaces(Formatter &out);
  virtual void GenerateCloseNameSpaces(Formatter &out);

  // Generates warning that file was auto-generated.
  virtual void GenerateWarningComment(Formatter &out);

  // Contains all information about the component.
  const ComponentSpecificationMessage &comp_spec_;
};

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_FUZZER_FUZZERCODEGENBASE_H_
