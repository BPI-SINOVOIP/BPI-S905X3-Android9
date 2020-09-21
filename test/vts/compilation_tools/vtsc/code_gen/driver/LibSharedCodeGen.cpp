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

#include "code_gen/driver/LibSharedCodeGen.h"

#include <iostream>
#include <string>

#include "VtsCompilerUtils.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const LibSharedCodeGen::kInstanceVariableName = "sharedlib_";

void LibSharedCodeGen::GenerateCppBodyFuzzFunction(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  out << "bool " << fuzzer_extended_class_name << "::Fuzz(" << "\n";
  out << "    FunctionSpecificationMessage* func_msg," << "\n";
  out << "    void** result, const string& callback_socket_name) {" << "\n";
  out.indent();
  out << "const char* func_name = func_msg->name().c_str();" << "\n";
  out << "LOG(INFO) << \"Function: \" << func_name;\n";

  for (auto const& api : message.interface().api()) {
    std::stringstream ss;

    out << "if (!strcmp(func_name, \"" << api.name() << "\")) {" << "\n";

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg_count == 0 && arg.type() == TYPE_PREDEFINED &&
          !strncmp(arg.predefined_type().c_str(),
                   message.original_data_structure_name().c_str(),
                   message.original_data_structure_name().length()) &&
          message.original_data_structure_name().length() > 0) {
        out << "    " << GetCppVariableType(arg) << " "
            << "arg" << arg_count << " = ";
        out << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
               << kInstanceVariableName << ")";
      } else if (arg.type() == TYPE_SCALAR) {
        if (arg.scalar_type() == "char_pointer" ||
            arg.scalar_type() == "uchar_pointer") {
          if (arg.scalar_type() == "char_pointer") {
            out << "    char ";
          } else {
            out << "    unsigned char ";
          }
          out << "arg" << arg_count
                 << "[func_msg->arg(" << arg_count
                 << ").string_value().length() + 1];" << "\n";
          out << "    if (func_msg->arg(" << arg_count
                 << ").type() == TYPE_SCALAR && "
                 << "func_msg->arg(" << arg_count
                 << ").string_value().has_message()) {" << "\n";
          out << "      strcpy(arg" << arg_count << ", "
                 << "func_msg->arg(" << arg_count << ").string_value()"
                 << ".message().c_str());" << "\n";
          out << "    } else {" << "\n";
          out << "   strcpy(arg" << arg_count << ", "
                 << GetCppInstanceType(arg) << ");" << "\n";
          out << "    }" << "\n";
        } else {
          out << "    " << GetCppVariableType(arg) << " "
                 << "arg" << arg_count << " = ";
          out << "(func_msg->arg(" << arg_count
                 << ").type() == TYPE_SCALAR && "
                 << "func_msg->arg(" << arg_count
                 << ").scalar_value().has_" << arg.scalar_type() << "()) ? ";
          if (arg.scalar_type() == "void_pointer") {
            out << "reinterpret_cast<" << GetCppVariableType(arg) << ">(";
          }
          out << "func_msg->arg(" << arg_count << ").scalar_value()."
                 << arg.scalar_type() << "()";
          if (arg.scalar_type() == "void_pointer") {
            out << ")";
          }
          out << " : " << GetCppInstanceType(arg);
        }
      } else {
        out << "    " << GetCppVariableType(arg) << " "
               << "arg" << arg_count << " = ";
        out << GetCppInstanceType(arg);
      }
      out << ";" << "\n";
      out << "LOG(INFO) << \"arg" << arg_count << " = \" << arg" << arg_count
          << ";\n";
      arg_count++;
    }

    out << "    ";
    out << "typedef void* (*";
    out << "func_type_" << api.name() << ")(...";
    out << ");" << "\n";

    // actual function call
    if (!api.has_return_type() || api.return_type().type() == TYPE_VOID) {
      out << "*result = NULL;" << "\n";
    } else {
      out << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    out << "    ";
    out << "((func_type_" << api.name() << ") "
           << "target_loader_.GetLoaderFunction(\"" << api.name() << "\"))(";
    // out << "reinterpret_cast<" << message.original_data_structure_name()
    //    << "*>(" << kInstanceVariableName << ")->" << api.name() << "(";

    if (arg_count > 0) out << "\n";

    for (int index = 0; index < arg_count; index++) {
      out << "      arg" << index;
      if (index != (arg_count - 1)) {
        out << "," << "\n";
      }
    }
    if (api.has_return_type() || api.return_type().type() != TYPE_VOID) {
      out << "))";
    }
    out << ");" << "\n";
    out << "    return true;" << "\n";
    out << "  }" << "\n";
  }
  // TODO: if there were pointers, free them.
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";
}

void LibSharedCodeGen::GenerateCppBodyGetAttributeFunction(
    Formatter& out,
    const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
  out << "bool " << fuzzer_extended_class_name << "::GetAttribute(" << "\n";
  out << "    FunctionSpecificationMessage* func_msg," << "\n";
  out << "    void** result) {" << "\n";
  out.indent();
  out << "const char* func_name = func_msg->name().c_str();" << "\n";
  out << "LOG(INFO) << \" '\" << func_name << \"'\";\n";
  out << "LOG(ERROR) << \"attribute not supported for shared lib yet.\";"
      << "\n";
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";
}

void LibSharedCodeGen::GenerateClassConstructionFunction(Formatter& out,
    const ComponentSpecificationMessage& /*message*/,
    const string& fuzzer_extended_class_name) {
  out << fuzzer_extended_class_name << "() : DriverBase(LIB_SHARED) {}\n";
}

}  // namespace vts
}  // namespace android
