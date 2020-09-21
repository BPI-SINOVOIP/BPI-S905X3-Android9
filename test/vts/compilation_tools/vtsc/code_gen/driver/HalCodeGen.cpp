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

#include "code_gen/driver/HalCodeGen.h"

#include <iostream>
#include <string>

#include "VtsCompilerUtils.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

using namespace std;
using namespace android;

namespace android {
namespace vts {

const char* const HalCodeGen::kInstanceVariableName = "device_";

void HalCodeGen::GenerateCppBodyInterfaceImpl(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  bool first_callback = true;

  for (int i = 0; i < message.interface().attribute_size(); i++) {
    const VariableSpecificationMessage& attribute = message.interface().attribute(i);
    if (attribute.type() != TYPE_FUNCTION_POINTER || !attribute.is_callback()) {
      continue;
    }
    string name =
        "vts_callback_" + fuzzer_extended_class_name + "_" + attribute.name();
    if (first_callback) {
      out << "static string callback_socket_name_;" << "\n";
      first_callback = false;
    }
    out << "\n";
    out << "class " << name << " : public DriverCallbackBase {"
        << "\n";
    out << " public:" << "\n";
    out.indent();
    out << name << "(const string& callback_socket_name) {" << "\n";
    out.indent();
    out << "callback_socket_name_ = callback_socket_name;" << "\n";
    out.unindent();
    out << "}" << "\n";

    int primitive_format_index = 0;
    for (const FunctionPointerSpecificationMessage& func_pt_spec :
         attribute.function_pointer()) {
      const string& callback_name = func_pt_spec.function_name();
      // TODO: callback's return value is assumed to be 'void'.
      out << "\n";
      out << "static ";
      bool has_return_value = false;
      if (!func_pt_spec.has_return_type() ||
          !func_pt_spec.return_type().has_type() ||
          func_pt_spec.return_type().type() == TYPE_VOID) {
        out << "void" << "\n";
      } else if (func_pt_spec.return_type().type() == TYPE_PREDEFINED) {
        out << func_pt_spec.return_type().predefined_type();
        has_return_value = true;
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR unknown type "
             << func_pt_spec.return_type().type() << "\n";
        exit(-1);
      }
      out << " " << callback_name << "(";
      int primitive_type_index;
      primitive_type_index = 0;
      for (const auto& arg : func_pt_spec.arg()) {
        if (primitive_type_index != 0) {
          out << ", ";
        }
        if (arg.is_const()) {
          out << "const ";
        }
        if (arg.type() == TYPE_SCALAR) {
          /*
          if (arg.scalar_type() == "pointer") {
            out << definition.aggregate_value(
                primitive_format_index).primitive_name(primitive_type_index)
                    << " ";
          } */
          if (arg.scalar_type() == "char_pointer") {
            out << "char* ";
          } else if (arg.scalar_type() == "uchar_pointer") {
            out << "unsigned char* ";
          } else if (arg.scalar_type() == "bool_t") {
            out << "bool ";
          } else if (arg.scalar_type() == "int8_t" ||
                     arg.scalar_type() == "uint8_t" ||
                     arg.scalar_type() == "int16_t" ||
                     arg.scalar_type() == "uint16_t" ||
                     arg.scalar_type() == "int32_t" ||
                     arg.scalar_type() == "uint32_t" ||
                     arg.scalar_type() == "size_t" ||
                     arg.scalar_type() == "int64_t" ||
                     arg.scalar_type() == "uint64_t") {
            out << arg.scalar_type() << " ";
          } else if (arg.scalar_type() == "void_pointer") {
            out << "void*";
          } else {
            cerr << __func__ << " unsupported scalar type " << arg.scalar_type()
                 << "\n";
            exit(-1);
          }
        } else if (arg.type() == TYPE_PREDEFINED) {
          out << arg.predefined_type() << " ";
        } else {
          cerr << __func__ << " unsupported type" << "\n";
          exit(-1);
        }
        out << "arg" << primitive_type_index;
        primitive_type_index++;
      }
      out << ") {" << "\n";
      out.indent();
#if USE_VAARGS
      out << "    const char fmt[] = \""
             << definition.primitive_format(primitive_format_index) << "\";"
             << "\n";
      out << "    va_list argp;" << "\n";
      out << "    const char* p;" << "\n";
      out << "    int i;" << "\n";
      out << "    char* s;" << "\n";
      out << "    char fmtbuf[256];" << "\n";
      out << "\n";
      out << "    va_start(argp, fmt);" << "\n";
      out << "\n";
      out << "    for (p = fmt; *p != '\\0'; p++) {" << "\n";
      out << "      if (*p != '%') {" << "\n";
      out << "        putchar(*p);" << "\n";
      out << "        continue;" << "\n";
      out << "      }" << "\n";
      out << "      switch (*++p) {" << "\n";
      out << "        case 'c':" << "\n";
      out << "          i = va_arg(argp, int);" << "\n";
      out << "          putchar(i);" << "\n";
      out << "          break;" << "\n";
      out << "        case 'd':" << "\n";
      out << "          i = va_arg(argp, int);" << "\n";
      out << "          s = itoa(i, fmtbuf, 10);" << "\n";
      out << "          fputs(s, stdout);" << "\n";
      out << "          break;" << "\n";
      out << "        case 's':" << "\n";
      out << "          s = va_arg(argp, char *);" << "\n";
      out << "          fputs(s, stdout);" << "\n";
      out << "          break;" << "\n";
      // out << "        case 'p':
      out << "        case '%':" << "\n";
      out << "          putchar('%');" << "\n";
      out << "          break;" << "\n";
      out << "      }" << "\n";
      out << "    }" << "\n";
      out << "    va_end(argp);" << "\n";
#endif
      // TODO: check whether bytes is set and handle properly if not.
      out << "AndroidSystemCallbackRequestMessage callback_message;"
             << "\n";
      out << "callback_message.set_id(GetCallbackID(\"" << callback_name
             << "\"));" << "\n";

      primitive_type_index = 0;
      for (const auto& arg : func_pt_spec.arg()) {
        out << "VariableSpecificationMessage* var_msg" << primitive_type_index
               << " = callback_message.add_arg();" << "\n";
        if (arg.type() == TYPE_SCALAR) {
          out << "var_msg" << primitive_type_index << "->set_type("
                 << "TYPE_SCALAR);" << "\n";
          out << "var_msg" << primitive_type_index << "->set_scalar_type(\""
                 << arg.scalar_type() << "\");" << "\n";
          out << "var_msg" << primitive_type_index << "->mutable_scalar_value()";
          if (arg.scalar_type() == "bool_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().bool_t() << ");" << "\n";
          } else if (arg.scalar_type() == "int8_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().int8_t() << ");" << "\n";
          } else if (arg.scalar_type() == "uint8_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().uint8_t() << ");" << "\n";
          } else if (arg.scalar_type() == "int16_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().int16_t() << ");" << "\n";
          } else if (arg.scalar_type() == "uint16_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().uint16_t() << ");" << "\n";
          } else if (arg.scalar_type() == "int32_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().int32_t() << ");" << "\n";
          } else if (arg.scalar_type() == "uint32_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().uint32_t() << ");" << "\n";
          } else if (arg.scalar_type() == "size_t") {
            out << "->set_uint32_t("
                << arg.scalar_value().uint32_t() << ");" << "\n";
          } else if (arg.scalar_type() == "int64_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().int64_t() << ");" << "\n";
          } else if (arg.scalar_type() == "uint64_t") {
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().uint64_t() << ");" << "\n";
          } else if (arg.scalar_type() == "char_pointer") {
            // pointer value is not meaning when it is passed to another machine.
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().char_pointer() << ");" << "\n";
          } else if (arg.scalar_type() == "uchar_pointer") {
            // pointer value is not meaning when it is passed to another machine.
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().uchar_pointer() << ");" << "\n";
          } else if (arg.scalar_type() == "void_pointer") {
            // pointer value is not meaning when it is passed to another machine.
            out << "->set_" << arg.scalar_type() << "("
                << arg.scalar_value().void_pointer() << ");" << "\n";
          } else {
            cerr << __func__ << " unsupported scalar type " << arg.scalar_type()
                 << "\n";
            exit(-1);
          }
        } else if (arg.type() == TYPE_PREDEFINED) {
          out << "var_msg" << primitive_type_index << "->set_type("
              << "TYPE_PREDEFINED);" << "\n";
          // TODO: actually handle such case.
        } else {
          cerr << __func__ << " unsupported type" << "\n";
          exit(-1);
        }
        primitive_type_index++;
      }
      out << "RpcCallToAgent(callback_message, callback_socket_name_);"
          << "\n";
      if (has_return_value) {
        // TODO: consider actual return type.
        out << "return NULL;";
      }
      out.unindent();
      out << "}" << "\n";
      out << "\n";

      primitive_format_index++;
    }
    out << "\n";
    out.unindent();
    out << " private:" << "\n";
    out << "};" << "\n";
    out << "\n";
  }
}

void HalCodeGen::GenerateCppBodyFuzzFunction(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  for (auto const& sub_struct : message.interface().sub_struct()) {
    GenerateCppBodyFuzzFunction(out, sub_struct, fuzzer_extended_class_name,
                                message.original_data_structure_name(),
                                sub_struct.is_pointer() ? "->" : ".");
  }

  out << "bool " << fuzzer_extended_class_name << "::Fuzz(" << "\n";
  out << "    FunctionSpecificationMessage* func_msg," << "\n";
  out << "    void** result, const string& callback_socket_name) {" << "\n";
  out.indent();
  out << "const char* func_name = func_msg->name().c_str();" << "\n";
  out << "LOG(INFO) << \" '\" << func_name << \"'\";"
      << "\n";

  // to call another function if it's for a sub_struct
  if (message.interface().sub_struct().size() > 0) {
    out << "if (func_msg->parent_path().length() > 0) {" << "\n";
    out.indent();
    for (auto const& sub_struct : message.interface().sub_struct()) {
      GenerateSubStructFuzzFunctionCall(out, sub_struct, "");
    }
    out.unindent();
    out << "}" << "\n";
  }

  out << message.original_data_structure_name()
      << "* local_device = ";
  out << "reinterpret_cast<" << message.original_data_structure_name()
      << "*>(" << kInstanceVariableName << ");" << "\n";

  out << "if (local_device == NULL) {" << "\n";
  out.indent();
  out << "LOG(INFO) << \"use hmi \" << (uint64_t)hmi_;"
      << "\n";
  out << "local_device = reinterpret_cast<"
      << message.original_data_structure_name() << "*>(hmi_);\n";
  out.unindent();
  out << "}" << "\n";
  out << "if (local_device == NULL) {" << "\n";
  out.indent();
  out << "LOG(ERROR) << \"both device_ and hmi_ are NULL.\";\n";
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";

  for (auto const& api : message.interface().api()) {
    out << "if (!strcmp(func_name, \"" << api.name() << "\")) {" << "\n";
    out.indent();
    out << "LOG(INFO) << \"match\" <<;\n";
    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_callback()) {  // arg.type() isn't always TYPE_FUNCTION_POINTER
        string name = "vts_callback_" + fuzzer_extended_class_name + "_" +
                      arg.predefined_type();  // TODO - check to make sure name
                                              // is always correct
        if (name.back() == '*') name.pop_back();
        out << name << "* arg" << arg_count << "callback = new ";
        out << name << "(callback_socket_name);" << "\n";
        out << "arg" << arg_count << "callback->Register(func_msg->arg("
            << arg_count << "));" << "\n";

        out << GetCppVariableType(arg) << " ";
        out << "arg" << arg_count << " = (" << GetCppVariableType(arg)
            << ") malloc(sizeof(" << GetCppVariableType(arg) << "));"
            << "\n";
        // TODO: think about how to free the malloced callback data structure.
        // find the spec.
        bool found = false;
        cout << name << "\n";
        for (auto const& attribute : message.interface().attribute()) {
          if (attribute.type() == TYPE_FUNCTION_POINTER &&
              attribute.is_callback()) {
            string target_name = "vts_callback_" + fuzzer_extended_class_name +
                                 "_" + attribute.name();
            cout << "compare" << "\n";
            cout << target_name << "\n";
            if (name == target_name) {
              if (attribute.function_pointer_size() > 1) {
                for (auto const& func_pt : attribute.function_pointer()) {
                  out << "arg" << arg_count << "->"
                      << func_pt.function_name() << " = arg" << arg_count
                      << "callback->" << func_pt.function_name() << ";"
                      << "\n";
                }
              } else {
                out << "arg" << arg_count << " = arg" << arg_count
                    << "callback->" << attribute.name() << ";" << "\n";
              }
              found = true;
              break;
            }
          }
        }
        if (!found) {
          cerr << __func__ << " ERROR callback definition missing for " << name
               << " of " << api.name() << "\n";
          exit(-1);
        }
      } else {
        out << GetCppVariableType(arg) << " ";
        out << "arg" << arg_count << " = ";
        if (arg_count == 0 && arg.type() == TYPE_PREDEFINED &&
            !strncmp(arg.predefined_type().c_str(),
                     message.original_data_structure_name().c_str(),
                     message.original_data_structure_name().length())) {
          out << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
                 << kInstanceVariableName << ")";
        } else {
          std::stringstream msg_ss;
          msg_ss << "func_msg->arg(" << arg_count << ")";
          string msg = msg_ss.str();

          if (arg.type() == TYPE_SCALAR) {
            out << "(" << msg << ".type() == TYPE_SCALAR)? ";
            if (arg.scalar_type() == "pointer" ||
                arg.scalar_type() == "pointer_pointer" ||
                arg.scalar_type() == "char_pointer" ||
                arg.scalar_type() == "uchar_pointer" ||
                arg.scalar_type() == "void_pointer" ||
                arg.scalar_type() == "function_pointer") {
              out << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
            }
            out << "(" << msg << ".scalar_value()";

            if (arg.scalar_type() == "bool_t" ||
                arg.scalar_type() == "int32_t" ||
                arg.scalar_type() == "uint32_t" ||
                arg.scalar_type() == "int64_t" ||
                arg.scalar_type() == "uint64_t" ||
                arg.scalar_type() == "int16_t" ||
                arg.scalar_type() == "uint16_t" ||
                arg.scalar_type() == "int8_t" ||
                arg.scalar_type() == "uint8_t" ||
                arg.scalar_type() == "float_t" ||
                arg.scalar_type() == "double_t") {
              out << "." << arg.scalar_type() << "() ";
            } else if (arg.scalar_type() == "pointer" ||
                       arg.scalar_type() == "char_pointer" ||
                       arg.scalar_type() == "uchar_pointer" ||
                       arg.scalar_type() == "void_pointer") {
              out << ".pointer() ";
            } else {
              cerr << __func__ << " ERROR unsupported scalar type "
                   << arg.scalar_type() << "\n";
              exit(-1);
            }
            out << ") : ";
          } else {
            cerr << __func__ << " unknown type " << msg << "\n";
          }

          out << "( (" << msg << ".type() == TYPE_PREDEFINED || " << msg
                 << ".type() == TYPE_STRUCT || " << msg
                 << ".type() == TYPE_SCALAR)? ";
          out << GetCppInstanceType(arg, msg);
          out << " : " << GetCppInstanceType(arg) << " )";
          // TODO: use the given message and call a lib function which converts
          // a message to a C/C++ struct.
        }
        out << ";" << "\n";
      }
      out << "LOG(INFO) << \"arg" << arg_count << " = \" << arg" << arg_count
          << ";\n";
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(out);
    out << "LOG(INFO) << \"hit2.\" << device_;\n";

    // checks whether the function is actually defined.
    out << "if (reinterpret_cast<"
           << message.original_data_structure_name() << "*>(local_device)->"
           << api.name() << " == NULL" << ") {" << "\n";
    out.indent();
    out << "LOG(ERROR) << \"api not set.\";\n";
    // todo: consider throwing an exception at least a way to tell more
    // specifically to the caller.
    out << "return false;" << "\n";
    out.unindent();
    out << "}" << "\n";

    out << "LOG(INFO) << \"Call an API.\";\n";
    out << "LOG(INFO) << \"local_device = \" << local_device;\n";

    if (!api.has_return_type() || api.return_type().type() == TYPE_VOID) {
      out << "*result = NULL;" << "\n";
    } else {
      out << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    out << "local_device->" << api.name() << "(";
    if (arg_count > 0) out << "\n";

    for (int index = 0; index < arg_count; index++) {
      out << "arg" << index;
      if (index != (arg_count - 1)) {
        out << "," << "\n";
      }
    }

    if (api.has_return_type() && api.return_type().type() != TYPE_VOID) {
      out << "))";
    }
    out << ");" << "\n";
    GenerateCodeToStopMeasurement(out);
    out << "LOG(INFO) << \"called\";\n";

    // Copy the output (call by pointer or reference cases).
    arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_output()) {
        // TODO check the return value
        out << GetConversionToProtobufFunctionName(arg) << "(arg"
            << arg_count << ", "
            << "func_msg->mutable_arg(" << arg_count << "));" << "\n";
      }
      arg_count++;
    }

    out << "return true;" << "\n";
    out.unindent();
    out << "}" << "\n";
  }
  // TODO: if there were pointers, free them.
  out << "LOG(ERROR) << \"func not found\";\n";
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";
}

void HalCodeGen::GenerateCppBodyFuzzFunction(
    Formatter& out, const StructSpecificationMessage& message,
    const string& fuzzer_extended_class_name,
    const string& original_data_structure_name, const string& parent_path) {
  for (auto const& sub_struct : message.sub_struct()) {
    GenerateCppBodyFuzzFunction(
        out, sub_struct, fuzzer_extended_class_name,
        original_data_structure_name,
        parent_path + message.name() + (sub_struct.is_pointer() ? "->" : "."));
  }

  string parent_path_printable(parent_path);
  ReplaceSubString(parent_path_printable, "->", "_");
  replace(parent_path_printable.begin(), parent_path_printable.end(), '.', '_');

  out << "bool " << fuzzer_extended_class_name << "::Fuzz_"
         << parent_path_printable + message.name() << "(" << "\n";
  out << "FunctionSpecificationMessage* func_msg," << "\n";
  out << "void** result, const string& callback_socket_name) {" << "\n";
  out.indent();
  out << "const char* func_name = func_msg->name().c_str();" << "\n";
  out << "LOG(INFO) << func_name;\n";

  bool is_open;
  for (auto const& api : message.api()) {
    is_open = false;
    if ((parent_path_printable + message.name()) == "_common_methods" &&
        api.name() == "open") {
      is_open = true;
    }

    out << "if (!strcmp(func_name, \"" << api.name() << "\")) {" << "\n";
    out.indent();

    out << original_data_structure_name << "* local_device = ";
    out << "reinterpret_cast<" << original_data_structure_name << "*>("
           << kInstanceVariableName << ");" << "\n";

    out << "if (local_device == NULL) {" << "\n";
    out.indent();
    out << "LOG(INFO) << \"use hmi\";\n";
    out << "local_device = reinterpret_cast<"
           << original_data_structure_name << "*>(hmi_);" << "\n";
    out.unindent();
    out << "}" << "\n";
    out << "if (local_device == NULL) {" << "\n";
    out.indent();
    out << "LOG(ERROR) << \"both device_ and hmi_ are NULL.\";\n";
    out << "return false;" << "\n";
    out.unindent();
    out << "}" << "\n";

    // args - definition;
    int arg_count = 0;
    for (auto const& arg : api.arg()) {
      out << GetCppVariableType(arg) << " ";
      out << "arg" << arg_count << " = ";
      if (arg_count == 0 && arg.type() == TYPE_PREDEFINED &&
          !strncmp(arg.predefined_type().c_str(),
                   original_data_structure_name.c_str(),
                   original_data_structure_name.length())) {
        out << "reinterpret_cast<" << GetCppVariableType(arg) << ">("
               << kInstanceVariableName << ")";
      } else {
        std::stringstream msg_ss;
        msg_ss << "func_msg->arg(" << arg_count << ")";
        string msg = msg_ss.str();

        if (arg.type() == TYPE_SCALAR) {
          out << "(" << msg << ".type() == TYPE_SCALAR && " << msg
                 << ".scalar_value()";
          if (arg.scalar_type() == "pointer" ||
              arg.scalar_type() == "char_pointer" ||
              arg.scalar_type() == "uchar_pointer" ||
              arg.scalar_type() == "void_pointer" ||
              arg.scalar_type() == "function_pointer") {
            out << ".has_pointer())? ";
            out << "reinterpret_cast<" << GetCppVariableType(arg) << ">";
          } else {
            out << ".has_" << arg.scalar_type() << "())? ";
          }
          out << "(" << msg << ".scalar_value()";

          if (arg.scalar_type() == "int32_t" ||
              arg.scalar_type() == "uint32_t" ||
              arg.scalar_type() == "int64_t" ||
              arg.scalar_type() == "uint64_t" ||
              arg.scalar_type() == "int16_t" ||
              arg.scalar_type() == "uint16_t" ||
              arg.scalar_type() == "int8_t" || arg.scalar_type() == "uint8_t" ||
              arg.scalar_type() == "float_t" ||
              arg.scalar_type() == "double_t") {
            out << "." << arg.scalar_type() << "() ";
          } else if (arg.scalar_type() == "pointer" ||
                     arg.scalar_type() == "char_pointer" ||
                     arg.scalar_type() == "uchar_pointer" ||
                     arg.scalar_type() == "function_pointer" ||
                     arg.scalar_type() == "void_pointer") {
            out << ".pointer() ";
          } else {
            cerr << __func__ << " ERROR unsupported type " << arg.scalar_type()
                 << "\n";
            exit(-1);
          }
          out << ") : ";
        }

        if (is_open) {
          if (arg_count == 0) {
            out << "hmi_;" << "\n";
          } else if (arg_count == 1) {
            out << "((hmi_) ? const_cast<char*>(hmi_->name) : NULL)" << "\n";
          } else if (arg_count == 2) {
            out << "(struct hw_device_t**) &device_" << "\n";
          } else {
            cerr << __func__ << " ERROR additional args for open " << arg_count
                 << "\n";
            exit(-1);
          }
        } else {
          out << "( (" << msg << ".type() == TYPE_PREDEFINED || " << msg
                 << ".type() == TYPE_STRUCT || " << msg
                 << ".type() == TYPE_SCALAR)? ";
          out << GetCppInstanceType(arg, msg);
          out << " : " << GetCppInstanceType(arg) << " )";
          // TODO: use the given message and call a lib function which converts
          // a message to a C/C++ struct.
        }
      }
      out << ";" << "\n";
      out << "LOG(INFO) << \"arg" << arg_count << " = \" << arg" << arg_count
          << "\n\n";
      arg_count++;
    }

    // actual function call
    GenerateCodeToStartMeasurement(out);
    out << "LOG(INFO) << \"hit2.\" << device_;\n";

    out << "if (reinterpret_cast<" << original_data_structure_name
           << "*>(local_device)" << parent_path << message.name() << "->"
           << api.name() << " == NULL";
    out << ") {" << "\n";
    out.indent();
    out << "LOG(ERROR) << \"api not set.\";\n";
    // todo: consider throwing an exception at least a way to tell more
    // specifically to the caller.
    out << "return false;" << "\n";
    out.unindent();
    out << "}" << "\n";

    out << "LOG(INFO) << \"Call an API.\";\n";
    if (!api.has_return_type() || api.return_type().type() == TYPE_VOID) {
      out << "*result = NULL;" << "\n";
    } else {
      out << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
    }
    out << "local_device" << parent_path << message.name() << "->"
        << api.name() << "(";
    if (arg_count > 0) out << "\n";

    for (int index = 0; index < arg_count; index++) {
      out << "arg" << index;
      if (index != (arg_count - 1)) {
        out << "," << "\n";
      }
    }
    if (api.has_return_type() && api.return_type().type() != TYPE_VOID) {
      out << "))";
    }
    out << ");" << "\n";
    GenerateCodeToStopMeasurement(out);
    out << "LOG(INFO) << \"called\";\n";

    // Copy the output (call by pointer or reference cases).
    arg_count = 0;
    for (auto const& arg : api.arg()) {
      if (arg.is_output()) {
        // TODO check the return value
        out << GetConversionToProtobufFunctionName(arg) << "(arg"
            << arg_count << ", "
            << "func_msg->mutable_arg(" << arg_count << "));" << "\n";
      }
      arg_count++;
    }

    out << "return true;" << "\n";
    out.unindent();
    out << "}" << "\n";
  }
  // TODO: if there were pointers, free them.
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";
}

void HalCodeGen::GenerateCppBodyGetAttributeFunction(
    Formatter& out, const ComponentSpecificationMessage& message,
    const string& fuzzer_extended_class_name) {
  for (auto const& sub_struct : message.interface().sub_struct()) {
    GenerateCppBodyGetAttributeFunction(
        out, sub_struct, fuzzer_extended_class_name,
        message.original_data_structure_name(),
        sub_struct.is_pointer() ? "->" : ".");
  }

  out << "bool " << fuzzer_extended_class_name << "::GetAttribute(" << "\n";
  out << "    FunctionSpecificationMessage* func_msg," << "\n";
  out << "    void** result) {" << "\n";
  out.indent();
  out << "const char* func_name = func_msg->name().c_str();" << "\n";
  out << "LOG(INFO) << \" '\" << func_name << \"'\";\n";

  // to call another function if it's for a sub_struct
  if (message.interface().sub_struct().size() > 0) {
    out << "  if (func_msg->parent_path().length() > 0) {" << "\n";
    out.indent();
    for (auto const& sub_struct : message.interface().sub_struct()) {
      GenerateSubStructGetAttributeFunctionCall(out, sub_struct, "");
    }
    out.unindent();
    out << "}" << "\n";
  }

  out << message.original_data_structure_name()
      << "* local_device = ";
  out << "reinterpret_cast<" << message.original_data_structure_name()
      << "*>(" << kInstanceVariableName << ");" << "\n";

  out << "if (local_device == NULL) {" << "\n";
  out.indent();
  out << "LOG(INFO) << \"use hmi \" << (uint64_t)hmi_;\n";
  out << "local_device = reinterpret_cast<"
         << message.original_data_structure_name() << "*>(hmi_);" << "\n";
  out.unindent();
  out << "}" << "\n";
  out << "if (local_device == NULL) {" << "\n";
  out.indent();
  out << "LOG(ERROR) << \"both device_ and hmi_ are NULL.\";\n";
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";

  for (auto const& attribute : message.interface().attribute()) {
    if (attribute.type() == TYPE_SUBMODULE ||
        attribute.type() == TYPE_SCALAR) {
      out << "if (!strcmp(func_name, \"" << attribute.name() << "\")) {" << "\n";
      out.indent();
      out << "LOG(INFO) << \"match\";\n";

      // actual function call
      out << "LOG(INFO) << \"hit2.\" << device_ ;\n";

      out << "LOG(INFO) << \"ok. let's read attribute.\";\n";
      out << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
      out << "local_device->" << attribute.name();
      out << "));" << "\n";

      out << "LOG(INFO) << \"got\";\n";

      out << "return true;" << "\n";
      out.unindent();
      out << "}" << "\n";
    }
  }
  // TODO: if there were pointers, free them.
  out << "LOG(ERROR) << \"attribute not found\";\n";
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";
}

void HalCodeGen::GenerateCppBodyGetAttributeFunction(
    Formatter& out, const StructSpecificationMessage& message,
    const string& fuzzer_extended_class_name,
    const string& original_data_structure_name, const string& parent_path) {
  for (auto const& sub_struct : message.sub_struct()) {
    GenerateCppBodyGetAttributeFunction(
        out, sub_struct, fuzzer_extended_class_name,
        original_data_structure_name,
        parent_path + message.name() + (sub_struct.is_pointer() ? "->" : "."));
  }

  string parent_path_printable(parent_path);
  ReplaceSubString(parent_path_printable, "->", "_");
  replace(parent_path_printable.begin(), parent_path_printable.end(), '.', '_');

  out << "bool " << fuzzer_extended_class_name << "::GetAttribute_"
         << parent_path_printable + message.name() << "(" << "\n";
  out << "    FunctionSpecificationMessage* func_msg," << "\n";
  out << "    void** result) {" << "\n";
  out.indent();
  out << "const char* func_name = func_msg->name().c_str();" << "\n";
  out << "LOG(INFO) << func_name;\n";

  out << original_data_structure_name
      << "* local_device = ";
  out << "reinterpret_cast<" << original_data_structure_name
      << "*>(" << kInstanceVariableName << ");" << "\n";

  out << "if (local_device == NULL) {" << "\n";
  out.indent();
  out << "  LOG(INFO) << \"use hmi \" << (uint64_t)hmi_;\n";
  out << "  local_device = reinterpret_cast<"
      << original_data_structure_name << "*>(hmi_);" << "\n";
  out.unindent();
  out << "}" << "\n";
  out << "if (local_device == NULL) {" << "\n";
  out.indent();
  out << "LOG(ERROR) << \"both device_ and hmi_ are NULL.\";\n";
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";

  for (auto const& attribute : message.attribute()) {
    if (attribute.type() == TYPE_SUBMODULE ||
        attribute.type() == TYPE_SCALAR) {
      out << "if (!strcmp(func_name, \"" << attribute.name() << "\")) {" << "\n";
      out.indent();
      out << "LOG(INFO) << \"match\";\n";

      // actual function call
      out << "LOG(INFO) << \"hit2.\" << device_;\n";

      out << "LOG(INFO) << \"ok. let's read attribute.\";\n";
      out << "*result = const_cast<void*>(reinterpret_cast<const void*>(";
      out << "local_device" << parent_path << message.name() << ".";
      // TODO: use parent's is_pointer()
      out << attribute.name();
      out << "));" << "\n";

      out << "LOG(INFO) << \"got\";\n";

      out << "return true;" << "\n";
      out.unindent();
      out << "}" << "\n";
    }
  }
  // TODO: if there were pointers, free them.
  out << "LOG(ERROR) << \"attribute not found\";\n";
  out << "return false;" << "\n";
  out.unindent();
  out << "}" << "\n";
}

void HalCodeGen::GenerateClassConstructionFunction(Formatter& out,
      const ComponentSpecificationMessage& /*message*/,
      const string& fuzzer_extended_class_name) {
  out << fuzzer_extended_class_name << "() : DriverBase(HAL_CONVENTIONAL) {}\n";
}

void HalCodeGen::GenerateSubStructFuzzFunctionCall(
    Formatter& out, const StructSpecificationMessage& message,
    const string& parent_path) {
  string current_path(parent_path);
  if (current_path.length() > 0) {
    current_path += ".";
  }
  current_path += message.name();

  string current_path_printable(current_path);
  replace(current_path_printable.begin(), current_path_printable.end(), '.',
          '_');

  out << "if (func_msg->parent_path() == \"" << current_path << "\") {"
      << "\n";
  out.indent();
  out << "return Fuzz__" << current_path_printable
      << "(func_msg, result, callback_socket_name);" << "\n";
  out.unindent();
  out << "}" << "\n";

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateSubStructFuzzFunctionCall(out, sub_struct, current_path);
  }
}

void HalCodeGen::GenerateSubStructGetAttributeFunctionCall(
    Formatter& out, const StructSpecificationMessage& message,
    const string& parent_path) {
  string current_path(parent_path);
  if (current_path.length() > 0) {
    current_path += ".";
  }
  current_path += message.name();

  string current_path_printable(current_path);
  replace(current_path_printable.begin(), current_path_printable.end(), '.',
          '_');

  out << "if (func_msg->parent_path() == \"" << current_path << "\") {"
      << "\n";
  out.indent();
  out << "      return GetAttribute__" << current_path_printable
      << "(func_msg, result);" << "\n";
  out.unindent();
  out << "}" << "\n";

  for (auto const& sub_struct : message.sub_struct()) {
    GenerateSubStructGetAttributeFunctionCall(out, sub_struct, current_path);
  }
}

}  // namespace vts
}  // namespace android
