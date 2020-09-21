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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_PROFILER_PROFILERCODEGENBASE_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_PROFILER_PROFILERCODEGENBASE_H_

#include <android-base/macros.h>
#include <hidl-util/FQName.h>
#include <hidl-util/Formatter.h>
#include <iostream>
#include <string>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

namespace android {
namespace vts {
/**
 * Base class that generates the profiler code for HAL interfaces.
 * It takes the input of a vts proto (i.e. ComponentSpecificationMessage) and
 * generates the header and source file of the corresponding profiler.
 *
 * All the profiler generator for a particular HAL type (e.g. Hidl Hal,
 * Legacy Hal etc.) should derive from this class.
 */
class ProfilerCodeGenBase {
 public:
  ProfilerCodeGenBase(){};

  virtual ~ProfilerCodeGenBase(){};

  // Generates both the header and source file for profiler.
  void GenerateAll(Formatter& header_out, Formatter& source_out,
    const ComponentSpecificationMessage& message);

  // Generates the header file for profiler.
  virtual void GenerateHeaderFile(Formatter &out,
    const ComponentSpecificationMessage &message);

  // Generates the source file for profiler.
  virtual void GenerateSourceFile(Formatter &out,
    const ComponentSpecificationMessage &message);

 protected:
  // Generates the profiler code for scalar type.
  virtual void GenerateProfilerForScalarVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) = 0;

  // Generates the profiler code for string type.
  virtual void GenerateProfilerForStringVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) = 0;

  // Generates the profiler code for enum type.
  virtual void GenerateProfilerForEnumVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) = 0;

  // Generates the profiler code for vector type.
  virtual void GenerateProfilerForVectorVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) = 0;

  // Generates the profiler code for array type.
  virtual void GenerateProfilerForArrayVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) = 0;

  // Generates the profiler code for struct type.
  virtual void GenerateProfilerForStructVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) = 0;

  // Generates the profiler code for union type.
  virtual void GenerateProfilerForUnionVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) = 0;

  // Generates the profiler code for hidl callback type.
  virtual void GenerateProfilerForHidlCallbackVariable(Formatter& out,
      const VariableSpecificationMessage& val, const std::string& arg_name,
      const std::string& arg_value) = 0;

  // Generates the profiler code for hidl interface type.
  virtual void GenerateProfilerForHidlInterfaceVariable(Formatter& out,
      const VariableSpecificationMessage& val, const std::string& arg_name,
      const std::string& arg_value) = 0;

  // Generates the profiler code for mask type.
  virtual void GenerateProfilerForMaskVariable(
      Formatter& out, const VariableSpecificationMessage& val,
      const std::string& arg_name, const std::string& arg_value) = 0;

  // Generates the profiler code for handle type.
  virtual void GenerateProfilerForHandleVariable(
      Formatter& out, const VariableSpecificationMessage& val,
      const std::string& arg_name, const std::string& arg_value) = 0;

  // Generates the profiler code for hidl memory type.
  virtual void GenerateProfilerForHidlMemoryVariable(Formatter& out,
      const VariableSpecificationMessage& val, const std::string& arg_name,
      const std::string& arg_value) = 0;

  // Generates the profiler code for pointer type.
  virtual void GenerateProfilerForPointerVariable(Formatter& out,
      const VariableSpecificationMessage& val, const std::string& arg_name,
      const std::string& arg_value) = 0;

  // Generates the profiler code for fmq sync type.
  virtual void GenerateProfilerForFMQSyncVariable(Formatter& out,
      const VariableSpecificationMessage& val, const std::string& arg_name,
      const std::string& arg_value) = 0;

  // Generates the profiler code for fmq unsync type.
  virtual void GenerateProfilerForFMQUnsyncVariable(Formatter& out,
      const VariableSpecificationMessage& val, const std::string& arg_name,
      const std::string& arg_value) = 0;

  // Generates the profiler code for method.
  virtual void GenerateProfilerForMethod(Formatter& out,
    const FunctionSpecificationMessage& method) = 0;

  // Generates the necessary "#include" code for header file of profiler.
  virtual void GenerateHeaderIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message) = 0;
  // Generates the necessary "#include" code for source file of profiler.
  virtual void GenerateSourceIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message) = 0;
  // Generates the necessary "using" code for profiler.
  virtual void GenerateUsingDeclaration(Formatter& out,
    const ComponentSpecificationMessage& message) = 0;
  // Generates the necessary "#define" code for profiler.
  virtual void GenerateMacros(Formatter&,
      const ComponentSpecificationMessage&) {};
  // Generates sanity check for profiler. These codes will be generated at the
  // beginning of the main profiler function.
  virtual void GenerateProfilerSanityCheck(
      Formatter&, const ComponentSpecificationMessage&){};
  // Generate local variable definition. These codes will be generated after
  // the sanity check code.
  virtual void GenerateLocalVariableDefinition(Formatter&,
    const ComponentSpecificationMessage&) {};

  // Generates the profiler code for a typed variable.
  virtual void GenerateProfilerForTypedVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value);

  // Generates the profiler method declaration for a user defined type.
  // (e.g. attributes within an interface).
  // The method signature is:
  // void profile__UDTName(VariableSpecificationMessage* arg_name,
  //                       UDTName arg_val_name);
  virtual void GenerateProfilerMethodDeclForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute);

  // Generates the profiler method implementation for a user defined type.
  virtual void GenerateProfilerMethodImplForAttribute(Formatter& out,
    const VariableSpecificationMessage& attribute);

  //**********   Utility functions   *****************
  virtual void GenerateOpenNameSpaces(Formatter& out,
      const ComponentSpecificationMessage& message);
  virtual void GenerateCloseNameSpaces(Formatter& out,
      const ComponentSpecificationMessage& message);

  std::string input_vts_file_path_;
  DISALLOW_COPY_AND_ASSIGN (ProfilerCodeGenBase);
};

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_GEN_PROFILER_PROFILERCODEGENBASE_H_
