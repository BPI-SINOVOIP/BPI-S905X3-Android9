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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_CODEGENBASE_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_CODEGENBASE_H_

#include <hidl-util/Formatter.h>
#include <iostream>
#include <sstream>
#include <string>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

enum VtsCompileMode {
  kDriver = 0,
  kProfiler,
  kFuzzer
};

// Specifies what kinds of files to generate.
enum VtsCompileFileType {
  kBoth = 0,
  kHeader,
  kSource,
};

class CodeGenBase {
 public:
  explicit CodeGenBase(const char* input_vts_file_path);
  virtual ~CodeGenBase();

  // Generate both a C/C++ file and its header file.
  virtual void GenerateAll(Formatter& header_out, Formatter& source_out,
                           const ComponentSpecificationMessage& message) = 0;

  // Generates source file.
  virtual void GenerateSourceFile(
      Formatter& out, const ComponentSpecificationMessage& message) = 0;

  // Generates header file.
  virtual void GenerateHeaderFile(
      Formatter& out, const ComponentSpecificationMessage& message) = 0;

  const char* input_vts_file_path() const {
    return input_vts_file_path_;
  }

 protected:
  const char* input_vts_file_path_;
};

// TODO(zhuoyao): move these methods to util files.
// Translates the VTS proto file to C/C++ code and header files.
void Translate(VtsCompileMode mode,
               const char* input_vts_file_path,
               const char* output_header_dir_path,
               const char* output_cpp_file_path);


// Translates the VTS proto file to a C/C++ source or header file.
void TranslateToFile(VtsCompileMode mode,
                     const char* input_vts_file_path,
                     const char* output_file_path,
                     VtsCompileFileType file_type);

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_CODEGENBASE_H_
