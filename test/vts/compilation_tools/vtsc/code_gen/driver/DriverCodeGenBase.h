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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_CODEGENBASE_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_CODEGENBASE_H_

#include <string>

#include <hidl-util/Formatter.h>
#include <hidl-util/FQName.h>

#include "code_gen/CodeGenBase.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class DriverCodeGenBase : public CodeGenBase {
 public:
  explicit DriverCodeGenBase(const char* input_vts_file_path)
      : CodeGenBase(input_vts_file_path) {}

  // Generate both a C/C++ file and its header file.
  virtual void GenerateAll(Formatter& header_out, Formatter& source_out,
                           const ComponentSpecificationMessage& message);

  // Generates source file.
  virtual void GenerateSourceFile(
      Formatter& out, const ComponentSpecificationMessage& message);

  // Generates header file.
  virtual void GenerateHeaderFile(
      Formatter& out, const ComponentSpecificationMessage& message);

 protected:

  // Generates header code for a specific class.
  virtual void GenerateClassHeader(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // Generates source code for a specific class.
  virtual void GenerateClassImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // Generates code for Fuzz(...) function body.
  virtual void GenerateCppBodyFuzzFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) = 0;

  // Generates code for GetAttribute(...) function body.
  virtual void GenerateCppBodyGetAttributeFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) = 0;

  // Generates code for CallFuction(...) function body.
  virtual void GenerateDriverFunctionImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // Generates code for VerifyResults(...) function body.
  virtual void GenerateVerificationFunctionImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // Generates C/C++ code for interface implemetation class.
  virtual void GenerateCppBodyInterfaceImpl(
      Formatter& /*out*/, const ComponentSpecificationMessage& /*message*/,
      const string& /*fuzzer_extended_class_name*/) {};

  // Generates header code for interface impl class.
  virtual void GenerateHeaderInterfaceImpl(
      Formatter& /*out*/, const ComponentSpecificationMessage& /*message*/) {};

  // Generates header code for construction function.
  virtual void GenerateClassConstructionFunction(Formatter& /*out*/,
      const ComponentSpecificationMessage& /*message*/,
      const string& /*fuzzer_extended_class_name*/) {};

  // Generates header code for additional function declarations if any.
  virtual void GenerateAdditionalFuctionDeclarations(Formatter& /*out*/,
      const ComponentSpecificationMessage& /*message*/,
      const string& /*fuzzer_extended_class_name*/) {};

  // Generates header code to declare the C/C++ global functions.
  virtual void GenerateHeaderGlobalFunctionDeclarations(Formatter& out,
      const ComponentSpecificationMessage& message,
      const bool print_extern_block = true);

  // Generates code for the bodies of the C/C++ global functions.
  virtual void GenerateCppBodyGlobalFunctions(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name,
      const bool print_extern_block = true);

  // Generates header code for include declarations.
  virtual void GenerateHeaderIncludeFiles(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // Generates source code for include declarations.
  virtual void GenerateSourceIncludeFiles(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // Generates header code for public function declarations if any.
  virtual void GeneratePublicFunctionDeclarations(
      Formatter& /*out*/, const ComponentSpecificationMessage& /*message*/) {};

  // Generates header code for private member declarations if any.
  virtual void GeneratePrivateMemberDeclarations(Formatter& /*out*/,
      const ComponentSpecificationMessage& /*message*/) {};

  //**********   Utility functions   *****************
  // Generates the namespace name of a HIDL component, crashes otherwise.
  void GenerateNamespaceName(
      Formatter& out, const ComponentSpecificationMessage& message);

  // Generates code that opens the default namespaces.
  void GenerateOpenNameSpaces(
      Formatter& out, const ComponentSpecificationMessage& message);

  // Generates code that closes the default namespaces.
  void GenerateCloseNameSpaces(Formatter& out,
      const ComponentSpecificationMessage& message);

  // Generates code that starts the measurement.
  void GenerateCodeToStartMeasurement(Formatter& out);

  // Generates code that stops the measurement.
  void GenerateCodeToStopMeasurement(Formatter& out);

  void GenerateFuzzFunctionForSubStruct(
      Formatter& out, const StructSpecificationMessage& message,
      const string& parent_path);
};

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_CODEGENBASE_H_
