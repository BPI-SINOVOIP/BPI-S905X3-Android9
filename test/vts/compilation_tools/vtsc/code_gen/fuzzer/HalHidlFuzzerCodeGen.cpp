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

#include "HalHidlFuzzerCodeGen.h"
#include "VtsCompilerUtils.h"
#include "code_gen/common/HalHidlCodeGenUtils.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

using std::cerr;
using std::cout;
using std::endl;
using std::vector;

namespace android {
namespace vts {

void HalHidlFuzzerCodeGen::GenerateSourceIncludeFiles(Formatter &out) {
  out << "#include <iostream>\n\n";
  out << "#include \"FuncFuzzerUtils.h\"\n";
  out << "#include <" << GetPackagePath(comp_spec_) << "/"
      << GetVersion(comp_spec_) << "/" << GetComponentName(comp_spec_)
      << ".h>\n";
  out << "\n";
}

void HalHidlFuzzerCodeGen::GenerateUsingDeclaration(Formatter &out) {
  out << "using std::cerr;\n";
  out << "using std::endl;\n";
  out << "using std::string;\n\n";

  string package_path = comp_spec_.package();
  ReplaceSubString(package_path, ".", "::");
  string comp_version = GetVersion(comp_spec_, true);

  out << "using namespace ::" << package_path << "::" << comp_version << ";\n";
  out << "using namespace ::android::hardware;\n";
  out << "\n";
}

void HalHidlFuzzerCodeGen::GenerateGlobalVars(Formatter &out) {
  out << "static string target_func;\n\n";
}

void HalHidlFuzzerCodeGen::GenerateLLVMFuzzerInitialize(Formatter &out) {
  out << "extern \"C\" int LLVMFuzzerInitialize(int *argc, char ***argv) "
         "{\n";
  out.indent();
  out << "FuncFuzzerParams params{ExtractFuncFuzzerParams(*argc, *argv)};\n";
  out << "target_func = params.target_func_;\n";
  out << "return 0;\n";
  out.unindent();
  out << "}\n\n";
}

void HalHidlFuzzerCodeGen::GenerateLLVMFuzzerTestOneInput(Formatter &out) {
  out << "extern \"C\" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t "
         "size) {\n";
  out.indent();
  out << "static ::android::sp<" << comp_spec_.component_name() << "> "
      << GetHalPointerName() << " = " << comp_spec_.component_name()
      << "::getService(true);\n";
  out << "if (" << GetHalPointerName() << " == nullptr) {\n";
  out.indent();
  out << "cerr << \"" << comp_spec_.component_name()
      << "::getService() failed\" << endl;\n";
  out << "exit(1);\n";
  out.unindent();
  out << "}\n\n";
  for (const auto &func_spec : comp_spec_.interface().api()) {
    GenerateHalFunctionCall(out, func_spec);
  }
  out << "{\n";
  out.indent();
  out << "cerr << \"No such function: \" << target_func << endl;\n";
  out << "exit(1);\n";
  out.unindent();
  out << "}\n";

  out.unindent();
  out << "}\n\n";
}

string HalHidlFuzzerCodeGen::GetHalPointerName() {
  string prefix = "android.hardware.";
  string hal_pointer_name = comp_spec_.package().substr(prefix.size());
  ReplaceSubString(hal_pointer_name, ".", "_");
  return hal_pointer_name;
}

void HalHidlFuzzerCodeGen::GenerateReturnCallback(
    Formatter &out, const FunctionSpecificationMessage &func_spec) {
  if (CanElideCallback(func_spec)) {
    return;
  }
  out << "// No-op. Only need this to make HAL function call.\n";
  out << "auto " << return_cb_name << " = [](";
  size_t num_cb_arg = func_spec.return_type_hidl_size();
  for (size_t i = 0; i < num_cb_arg; ++i) {
    const auto &return_val = func_spec.return_type_hidl(i);
    out << GetCppVariableType(return_val, IsConstType(return_val.type()));
    out << " arg" << i << ((i != num_cb_arg - 1) ? ", " : "");
  }
  out << "){};\n\n";
}

void HalHidlFuzzerCodeGen::GenerateHalFunctionCall(
    Formatter &out, const FunctionSpecificationMessage &func_spec) {
  string func_name = func_spec.name();
  out << "if (target_func == \"" << func_name << "\") {\n";
  out.indent();

  GenerateReturnCallback(out, func_spec);
  vector<string> types{GetFuncArgTypes(func_spec)};
  for (size_t i = 0; i < types.size(); ++i) {
    out << "size_t type_size" << i << " = sizeof(" << types[i] << ");\n";
    out << "if (size < type_size" << i << ") { return 0; }\n";
    out << "size -= type_size" << i << ";\n";
    out << types[i] << " arg" << i << ";\n";
    out << "memcpy(&arg" << i << ", data, type_size" << i << ");\n";
    out << "data += type_size" << i << ";\n\n";
  }

  out << GetHalPointerName() << "->" << func_spec.name() << "(";
  for (size_t i = 0; i < types.size(); ++i) {
    out << "arg" << i << ((i != types.size() - 1) ? ", " : "");
  }
  if (!CanElideCallback(func_spec)) {
    if (func_spec.arg_size() > 0) {
      out << ", ";
    }
    out << return_cb_name;
  }
  out << ");\n";
  out << "return 0;\n";

  out.unindent();
  out << "} else ";
}

bool HalHidlFuzzerCodeGen::CanElideCallback(
    const FunctionSpecificationMessage &func_spec) {
  if (func_spec.return_type_hidl_size() == 0) {
    return true;
  }
  // Can't elide callback for void or tuple-returning methods
  if (func_spec.return_type_hidl_size() != 1) {
    return false;
  }
  const VariableType &type = func_spec.return_type_hidl(0).type();
  if (type == TYPE_ARRAY || type == TYPE_VECTOR || type == TYPE_REF) {
    return false;
  }
  return IsElidableType(type);
}

vector<string> HalHidlFuzzerCodeGen::GetFuncArgTypes(
    const FunctionSpecificationMessage &func_spec) {
  vector<string> types{};
  for (const auto &var_spec : func_spec.arg()) {
    string type = GetCppVariableType(var_spec);
    types.emplace_back(GetCppVariableType(var_spec));
  }
  return types;
}

}  // namespace vts
}  // namespace android
