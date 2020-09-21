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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_HALHIDLCODEGEN_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_HALHIDLCODEGEN_H_

#include <string>

#include "code_gen/driver/DriverCodeGenBase.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class HalHidlCodeGen : public DriverCodeGenBase {
 public:
  HalHidlCodeGen(const char* input_vts_file_path)
      : DriverCodeGenBase(input_vts_file_path) {}

 protected:
  void GenerateClassHeader(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateClassImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateCppBodyFuzzFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  virtual void GenerateDriverFunctionImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateVerificationFunctionImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateCppBodyGetAttributeFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateHeaderInterfaceImpl(
      Formatter& out, const ComponentSpecificationMessage& message) override;

  void GenerateCppBodyInterfaceImpl(
      Formatter& out, const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateClassConstructionFunction(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateHeaderGlobalFunctionDeclarations(Formatter& out,
      const ComponentSpecificationMessage& message,
      const bool print_extern_block = true) override;

  void GenerateCppBodyGlobalFunctions(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name,
      const bool print_extern_block = true) override;

  void GenerateHeaderIncludeFiles(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateSourceIncludeFiles(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GenerateAdditionalFuctionDeclarations(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name) override;

  void GeneratePublicFunctionDeclarations(
      Formatter& out, const ComponentSpecificationMessage& message) override;

  void GeneratePrivateMemberDeclarations(Formatter& out,
      const ComponentSpecificationMessage& message) override;

 private:
  void GenerateCppBodyFuzzFunction(Formatter& out,
      const StructSpecificationMessage& message,
      const string& fuzzer_extended_class_name,
      const string& original_data_structure_name, const string& parent_path);

  // Generates the code to declar the impl class for an interface.
  void GenerateClassDecalrationForInterface(
      Formatter& out, const ComponentSpecificationMessage& message);

  // Generates a scalar type in C/C++.
  void GenerateScalarTypeInC(Formatter& out, const string& type);

  // Generates the driver function implementation for hidl reserved methods.
  void GenerateDriverImplForReservedMethods(Formatter& out);

  // Generates the driver function implementation for a method.
  void GenerateDriverImplForMethod(Formatter& out,
      const FunctionSpecificationMessage& func_msg);

  // Generates the code to perform a Hal function call.
  void GenerateHalFunctionCall(Formatter& out,
      const FunctionSpecificationMessage& func_msg);

  // Generates the implementation of a callback passed to the Hal function call.
  void GenerateSyncCallbackFunctionImpl(Formatter& out,
      const FunctionSpecificationMessage& func_msg);

  // Generates the driver function declaration for attributes defined within
  // an interface or in a types.hal.
  void GenerateDriverDeclForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the driver function implementation for attributes defined within
  // an interface or in a types.hal.
  void GenerateDriverImplForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the driver code for a typed variable.
  void GenerateDriverImplForTypedVariable(Formatter& out,
      const VariableSpecificationMessage& val, const string& arg_name,
      const string& arg_value_name);

  // Generates the verification function declarations for attributes defined
  // within an interface or in a types.hal.
  void GenerateVerificationDeclForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the verification function implementation for attributes defined
  // within an interface or in a types.hal.
  void GenerateVerificationImplForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the verification code for a typed variable.
  void GenerateVerificationCodeForTypedVariable(Formatter& out,
      const VariableSpecificationMessage& val, const string& result_value,
      const string& expected_result);

  // Generates the SetResult function declarations for attributes defined
  // within an interface or in a types.hal.
  void GenerateSetResultDeclForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the SetResult function implementation for attributes defined
  // within an interface or in a types.hal.
  void GenerateSetResultImplForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the SetResult code for a typed variable.
  void GenerateSetResultCodeForTypedVariable(Formatter& out,
      const VariableSpecificationMessage& val, const string& result_msg,
      const string& result_val);

  // Generates the random function declaration for attributes defined within
  // an interface or in a types.hal.
  void GenerateRandomFunctionDeclForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the default return value for a typed variable.
  void GenerateDefaultReturnValForTypedVariable(
      Formatter& out, const VariableSpecificationMessage& val);

  // Generates the random function implementation for attributes defined within
  // an interface or in a types.hal.
  void GenerateRandomFunctionImplForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates the getService function implementation for an interface.
  void GenerateGetServiceImpl(Formatter& out,
      const ComponentSpecificationMessage& message,
      const string& fuzzer_extended_class_name);

  // Generates all function declaration for an attributed.
  void GenerateAllFunctionDeclForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Generates all function implementation for an attributed.
  void GenerateAllFunctionImplForAttribute(Formatter& out,
      const VariableSpecificationMessage& attribute);

  // Returns true if we could omit the callback function and return result
  // directly.
  bool CanElideCallback(const FunctionSpecificationMessage& func_msg);

  // instance variable name (e.g., device_);
  static const char* const kInstanceVariableName;
};

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_DRIVER_HALHIDLCODEGEN_H_
