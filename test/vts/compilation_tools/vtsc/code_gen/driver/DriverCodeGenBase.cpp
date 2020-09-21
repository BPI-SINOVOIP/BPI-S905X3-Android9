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

#include "code_gen/driver/DriverCodeGenBase.h"

#include <iostream>
#include <string>

#include <hidl-util/Formatter.h>
#include <hidl-util/FQName.h>

#include "VtsCompilerUtils.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

using namespace std;

namespace android {
namespace vts {

void DriverCodeGenBase::GenerateAll(
    Formatter& header_out, Formatter& source_out,
    const ComponentSpecificationMessage& message) {
  GenerateHeaderFile(header_out, message);
  GenerateSourceFile(source_out, message);
}

void DriverCodeGenBase::GenerateHeaderFile(
    Formatter& out, const ComponentSpecificationMessage& message) {
  string component_name = GetComponentName(message);
  if (component_name.empty()) {
    cerr << __func__ << ":" << __LINE__ << " error component_name is empty"
         << "\n";
    exit(-1);
  }

  string component_name_token;
  if (message.component_class() == HAL_HIDL) {
    FQName component_fq_name = GetFQName(message);
    component_name_token = component_fq_name.tokenName();
  } else {
    string version = GetVersion(message, true);
    component_name_token =
        ComponentClassToString(message.component_class()) + "_" +
        ComponentTypeToString(message.component_type()) + "_" + version;
  }
  string fuzzer_extended_class_name;
  if (message.component_class() == HAL_HIDL) {
    fuzzer_extended_class_name = "FuzzerExtended_" + component_name_token;
  } else {
    fuzzer_extended_class_name = "FuzzerExtended_" + GetComponentName(message);
  }

  out << "#ifndef __VTS_DRIVER__" << component_name_token << "__\n";
  out << "#define __VTS_DRIVER__" << component_name_token << "__\n";
  out << "\n";

  out << "#undef LOG_TAG\n";
  out << "#define LOG_TAG \"" << fuzzer_extended_class_name << "\"\n";

  GenerateHeaderIncludeFiles(out, message, fuzzer_extended_class_name);

  GenerateOpenNameSpaces(out, message);
  GenerateClassHeader(out, message, fuzzer_extended_class_name);
  out << "\n\n";
  GenerateHeaderGlobalFunctionDeclarations(out, message);
  GenerateCloseNameSpaces(out, message);
  out << "#endif" << "\n";
}

void DriverCodeGenBase::GenerateSourceFile(
    Formatter& out, const ComponentSpecificationMessage& message) {
  string component_name = GetComponentName(message);
  if (component_name.empty()) {
    cerr << __func__ << ":" << __LINE__ << " error component_name is empty"
         << "\n";
    exit(-1);
  }
  FQName component_fq_name = GetFQName(message);
  string component_name_token = component_fq_name.tokenName();
  string fuzzer_extended_class_name;
  if (message.component_class() == HAL_HIDL) {
    fuzzer_extended_class_name = "FuzzerExtended_" + component_name_token;
  } else {
    fuzzer_extended_class_name = "FuzzerExtended_" + GetComponentName(message);
  }
  GenerateSourceIncludeFiles(out, message, fuzzer_extended_class_name);
  out << "\n\n";
  GenerateOpenNameSpaces(out, message);
  GenerateClassImpl(out, message, fuzzer_extended_class_name);
  GenerateCppBodyGlobalFunctions(out, message, fuzzer_extended_class_name);
  GenerateCloseNameSpaces(out, message);
}

void DriverCodeGenBase::GenerateClassHeader(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  GenerateHeaderInterfaceImpl(out, message);
  out << "class " << fuzzer_extended_class_name << " : public DriverBase {"
      << "\n";
  out << " public:" << "\n";

  out.indent();
  GenerateClassConstructionFunction(out, message, fuzzer_extended_class_name);
  GeneratePublicFunctionDeclarations(out, message);
  out.unindent();

  out << " protected:" << "\n";

  out.indent();
  out << "bool Fuzz(FunctionSpecificationMessage* func_msg, void** result, "
      << "const string& callback_socket_name);\n";
  out << "bool CallFunction(const FunctionSpecificationMessage& func_msg, "
      << "const string& callback_socket_name, "
      << "FunctionSpecificationMessage* result_msg);\n";
  out << "bool VerifyResults(const FunctionSpecificationMessage& expected_result, "
      << "const FunctionSpecificationMessage& actual_result);\n";
  out << "bool GetAttribute(FunctionSpecificationMessage* func_msg, "
      << "void** result);\n";

  // Produce Fuzz method(s) for sub_struct(s).
  for (auto const& sub_struct : message.interface().sub_struct()) {
    GenerateFuzzFunctionForSubStruct(out, sub_struct, "_");
  }
  // Generate additional function declarations if any.
  GenerateAdditionalFuctionDeclarations(out, message,
                                        fuzzer_extended_class_name);
  out.unindent();

  out << " private:" << "\n";

  out.indent();
  // Generate declarations of private members if any.
  GeneratePrivateMemberDeclarations(out, message);
  out.unindent();

  out << "};\n";
}

void DriverCodeGenBase::GenerateClassImpl(Formatter& out,
    const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  GenerateCppBodyInterfaceImpl(out, message, fuzzer_extended_class_name);
  GenerateCppBodyFuzzFunction(out, message, fuzzer_extended_class_name);
  GenerateCppBodyGetAttributeFunction(out, message, fuzzer_extended_class_name);
  GenerateDriverFunctionImpl(out, message, fuzzer_extended_class_name);
  GenerateVerificationFunctionImpl(out, message, fuzzer_extended_class_name);
}

void DriverCodeGenBase::GenerateHeaderIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message, const string&) {
  for (auto const& header : message.header()) {
    out << "#include " << header << "\n";
  }
  out << "\n";
  out << "#include <stdio.h>" << "\n";
  out << "#include <stdarg.h>" << "\n";
  out << "#include <stdlib.h>" << "\n";
  out << "#include <string.h>" << "\n";
  out << "#include <utils/Log.h>" << "\n";
  out << "\n";
  out << "#include <driver_base/DriverBase.h>"
      << "\n";
  out << "#include <driver_base/DriverCallbackBase.h>"
      << "\n";
  out << "\n";
  if (message.component_class() == HAL_HIDL) {
    out << "#include <VtsDriverCommUtil.h>" << "\n";
    out << "\n";
  }
}

void DriverCodeGenBase::GenerateSourceIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message, const string&) {
  if (message.component_class() != HAL_HIDL) {
    out << "#include \"" << input_vts_file_path_ << ".h\"\n";
    for (auto const& header : message.header()) {
      out << "#include " << header << "\n";
    }
    out << "#include \"vts_datatype.h\"" << "\n";
  } else {
    out << "#include \"" << GetPackagePath(message) << "/"
        << GetVersion(message) << "/" << GetComponentBaseName(message)
        << ".vts.h\"\n";
  }
  out << "#include \"vts_measurement.h\"" << "\n";
  out << "#include <android-base/logging.h>"
      << "\n";
}

void DriverCodeGenBase::GenerateHeaderGlobalFunctionDeclarations(
    Formatter& out, const ComponentSpecificationMessage& message,
    const bool print_extern_block) {
  string function_name_prefix = GetFunctionNamePrefix(message);

  if (print_extern_block) {
    out << "extern \"C\" {" << "\n";
  }
  out << "extern "
      << "android::vts::DriverBase* " << function_name_prefix << "();\n";
  if (print_extern_block) {
    out << "}" << "\n";
  }
}

void DriverCodeGenBase::GenerateCppBodyGlobalFunctions(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name, const bool print_extern_block) {
  string function_name_prefix = GetFunctionNamePrefix(message);

  if (print_extern_block) {
    out << "extern \"C\" {" << "\n";
  }
  out << "android::vts::DriverBase* " << function_name_prefix << "() {\n";
  out.indent();
  out << "return (android::vts::DriverBase*) "
      << "new android::vts::";
  out << fuzzer_extended_class_name << "();\n";
  out.unindent();
  out << "}\n\n";
  if (print_extern_block) {
    out << "}\n";
  }
}

void DriverCodeGenBase::GenerateFuzzFunctionForSubStruct(
    Formatter& out, const StructSpecificationMessage& message,
    const string& parent_path) {
  out << "bool Fuzz_" << parent_path << message.name()
      << "(FunctionSpecificationMessage* func_msg," << "\n";
  out << "    void** result, const string& callback_socket_name);"
      << "\n";

  out << "bool GetAttribute_" << parent_path << message.name()
      << "(FunctionSpecificationMessage* /*func_msg*/,"
      << "\n";
  out << "    void** /*result*/);"
      << "\n";

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateFuzzFunctionForSubStruct(out, sub_struct,
                                     parent_path + message.name() + "_");
  }
}

void DriverCodeGenBase::GenerateDriverFunctionImpl(Formatter& out,
    const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
  out << "bool " << fuzzer_extended_class_name
      << "::CallFunction(const FunctionSpecificationMessage&, const string&, "
      << "FunctionSpecificationMessage* ) {\n";
  out.indent();
  out << "/* No implementation yet. */\n";
  out << "return true;\n";
  out.unindent();
  out << "}\n";
}

void DriverCodeGenBase::GenerateVerificationFunctionImpl(Formatter& out,
    const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
  out << "bool " << fuzzer_extended_class_name
      << "::VerifyResults(const FunctionSpecificationMessage&, "
      << "const FunctionSpecificationMessage&) {\n";
  out.indent();
  out << "/* No implementation yet. */\n";
  out << "return true;\n";
  out.unindent();
  out << "}\n";
}

void DriverCodeGenBase::GenerateNamespaceName(
    Formatter& out, const ComponentSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    out << GetPackageNamespaceToken(message)
        << "::" << GetVersion(message, true);
  } else {
    cerr << __func__ << ":" << __LINE__ << " no namespace" << "\n";
    exit(-1);
  }
}

void DriverCodeGenBase::GenerateOpenNameSpaces(Formatter& out,
    const ComponentSpecificationMessage& message) {
  if (message.component_class() == HAL_HIDL && message.has_package()) {
    out << "using namespace ";
    GenerateNamespaceName(out, message);
    out << ";" << "\n";
  }

  out << "namespace android {" << "\n";
  out << "namespace vts {" << "\n";
}

void DriverCodeGenBase::GenerateCloseNameSpaces(Formatter& out,
    const ComponentSpecificationMessage& /*message*/) {
  out << "}  // namespace vts" << "\n";
  out << "}  // namespace android" << "\n";
}

void DriverCodeGenBase::GenerateCodeToStartMeasurement(Formatter& out) {
  out << "VtsMeasurement vts_measurement;" << "\n";
  out << "vts_measurement.Start();" << "\n";
}

void DriverCodeGenBase::GenerateCodeToStopMeasurement(Formatter& out) {
  out << "vector<float>* measured = vts_measurement.Stop();" << "\n";
  out << "LOG(INFO) << \"time \" << (*measured)[0];"
      << "\n";
}

}  // namespace vts
}  // namespace android
