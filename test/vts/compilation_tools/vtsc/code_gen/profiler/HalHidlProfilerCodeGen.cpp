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

#include "HalHidlProfilerCodeGen.h"
#include "VtsCompilerUtils.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

namespace android {
namespace vts {

void HalHidlProfilerCodeGen::GenerateProfilerForScalarVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_SCALAR);\n";
  out << arg_name << "->mutable_scalar_value()->set_" << val.scalar_type()
      << "(" << arg_value << ");\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForStringVariable(
    Formatter& out, const VariableSpecificationMessage&,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_STRING);\n";
  out << arg_name << "->mutable_string_value()->set_message"
      << "(" << arg_value << ".c_str());\n";
  out << arg_name << "->mutable_string_value()->set_length"
      << "(" << arg_value << ".size());\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForEnumVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_ENUM);\n";

  // For predefined type, call the corresponding profile method.
  if (val.has_predefined_type()) {
    std::string predefined_type = val.predefined_type();
    ReplaceSubString(predefined_type, "::", "__");
    out << "profile__" << predefined_type << "(" << arg_name << ", "
        << arg_value << ");\n";
  } else {
    const std::string scalar_type = val.enum_value().scalar_type();
    out << arg_name << "->mutable_scalar_value()->set_" << scalar_type
        << "(static_cast<" << scalar_type << ">(" << arg_value << "));\n";
    out << arg_name << "->set_scalar_type(\"" << scalar_type << "\");\n";
  }
}

void HalHidlProfilerCodeGen::GenerateProfilerForVectorVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_VECTOR);\n";
  out << arg_name << "->set_vector_size(" << arg_value << ".size());\n";
  std::string index_name = GetVarString(arg_name) + "_index";
  out << "for (int " << index_name << " = 0; " << index_name << " < (int)"
      << arg_value << ".size(); " << index_name << "++) {\n";
  out.indent();
  std::string vector_element_name =
      GetVarString(arg_name) + "_vector_" + index_name;
  out << "auto *" << vector_element_name
      << " __attribute__((__unused__)) = " << arg_name
      << "->add_vector_value();\n";
  GenerateProfilerForTypedVariable(out, val.vector_value(0),
                                   vector_element_name,
                                   arg_value + "[" + index_name + "]");
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForArrayVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_ARRAY);\n";
  out << arg_name << "->set_vector_size(" << val.vector_size() << ");\n";
  std::string index_name = GetVarString(arg_name) + "_index";
  out << "for (int " << index_name << " = 0; " << index_name << " < "
      << val.vector_size() << "; " << index_name << "++) {\n";
  out.indent();
  std::string array_element_name =
      GetVarString(arg_name) + "_array_" + index_name;
  out << "auto *" << array_element_name
      << " __attribute__((__unused__)) = " << arg_name
      << "->add_vector_value();\n";
  GenerateProfilerForTypedVariable(out, val.vector_value(0), array_element_name,
                                   arg_value + "[" + index_name + "]");
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForStructVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_STRUCT);\n";
  // For predefined type, call the corresponding profile method.
  if (val.struct_value().size() == 0 && val.has_predefined_type()) {
    std::string predefined_type = val.predefined_type();
    ReplaceSubString(predefined_type, "::", "__");
    out << "profile__" << predefined_type << "(" << arg_name << ", "
        << arg_value << ");\n";
  } else {
    for (const auto struct_field : val.struct_value()) {
      std::string struct_field_name = arg_name + "_" + struct_field.name();
      out << "auto *" << struct_field_name
          << " __attribute__((__unused__)) = " << arg_name
          << "->add_struct_value();\n";
      GenerateProfilerForTypedVariable(out, struct_field, struct_field_name,
                                       arg_value + "." + struct_field.name());
    }
  }
}

void HalHidlProfilerCodeGen::GenerateProfilerForUnionVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_UNION);\n";
  // For predefined type, call the corresponding profile method.
  if (val.union_value().size() == 0 && val.has_predefined_type()) {
    std::string predefined_type = val.predefined_type();
    ReplaceSubString(predefined_type, "::", "__");
    out << "profile__" << predefined_type << "(" << arg_name << ", "
        << arg_value << ");\n";
  } else {
    for (const auto union_field : val.union_value()) {
      std::string union_field_name = arg_name + "_" + union_field.name();
      out << "auto *" << union_field_name << " = " << arg_name
          << "->add_union_value();\n";
      GenerateProfilerForTypedVariable(out, union_field, union_field_name,
                                       arg_value + "." + union_field.name());
    }
  }
}

void HalHidlProfilerCodeGen::GenerateProfilerForHidlCallbackVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string&) {
  out << arg_name << "->set_type(TYPE_HIDL_CALLBACK);\n";
  out << arg_name << "->set_predefined_type(\"" << val.predefined_type()
      << "\");\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForHidlInterfaceVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string&) {
  out << arg_name << "->set_type(TYPE_HIDL_INTERFACE);\n";
  out << arg_name << "->set_predefined_type(\"" << val.predefined_type()
      << "\");\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForMaskVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_MASK);\n";
  out << arg_name << "->set_scalar_type(\"" << val.scalar_type() << "\");\n";
  out << arg_name << "->mutable_scalar_value()->set_" << val.scalar_type()
      << "(" << arg_value << ");\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForHandleVariable(
    Formatter& out, const VariableSpecificationMessage&,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_HANDLE);\n";
  std::string handle_name = arg_name + "_h";
  out << "auto " << handle_name << " = " << arg_value
      << ".getNativeHandle();\n";
  out << "if (!" << handle_name << ") {\n";
  out.indent();
  out << "LOG(WARNING) << \"null handle\";\n";
  out << "return;\n";
  out.unindent();
  out << "}\n";
  out << arg_name << "->mutable_handle_value()->set_version(" << handle_name
      << "->version);\n";
  out << arg_name << "->mutable_handle_value()->set_num_ints(" << handle_name
      << "->numInts);\n";
  out << arg_name << "->mutable_handle_value()->set_num_fds(" << handle_name
      << "->numFds);\n";
  out << "for (int i = 0; i < " << handle_name << "->numInts + " << handle_name
      << "->numFds; i++) {\n";
  out.indent();
  out << "if(i < " << handle_name << "->numFds) {\n";
  out.indent();
  out << "auto* fd_val_i = " << arg_name
      << "->mutable_handle_value()->add_fd_val();\n";
  out << "char filePath[PATH_MAX];\n";
  out << "string procPath = \"/proc/self/fd/\" + to_string(" << handle_name
      << "->data[i]);\n";
  out << "ssize_t r = readlink(procPath.c_str(), filePath, "
         "sizeof(filePath));\n";
  out << "if (r == -1) {\n";
  out.indent();
  out << "LOG(ERROR) << \"Unable to get file path\";\n";
  out << "continue;\n";
  out.unindent();
  out << "}\n";
  out << "filePath[r] = '\\0';\n";
  out << "fd_val_i->set_file_name(filePath);\n";
  out << "struct stat statbuf;\n";
  out << "fstat(" << handle_name << "->data[i], &statbuf);\n";
  out << "fd_val_i->set_mode(statbuf.st_mode);\n";
  out << "if (S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)) {\n";
  out.indent();
  out << "fd_val_i->set_type(S_ISREG(statbuf.st_mode)? FILE_TYPE: DIR_TYPE);\n";
  out << "int flags = fcntl(" << handle_name << "->data[i], F_GETFL);\n";
  out << "fd_val_i->set_flags(flags);\n";
  out.unindent();
  out << "}\n";
  out << "else if (S_ISCHR(statbuf.st_mode) || S_ISBLK(statbuf.st_mode)) {\n";
  out.indent();
  out << "fd_val_i->set_type(DEV_TYPE);\n";
  out << "if (strcmp(filePath, \"/dev/ashmem\") == 0) {\n";
  out.indent();
  out << "int size = ashmem_get_size_region(" << handle_name << "->data[i]);\n";
  out << "fd_val_i->mutable_memory()->set_size(size);\n";
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "}\n";
  out << "else if (S_ISFIFO(statbuf.st_mode)){\n";
  out.indent();
  out << "fd_val_i->set_type(PIPE_TYPE);\n";
  out.unindent();
  out << "}\n";
  out << "else if (S_ISSOCK(statbuf.st_mode)) {\n";
  out.indent();
  out << "fd_val_i->set_type(SOCKET_TYPE);\n";
  out.unindent();
  out << "}\n";
  out << "else {\n";
  out.indent();
  out << "fd_val_i->set_type(LINK_TYPE);\n";
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "} else {\n";
  out.indent();
  out << arg_name << "->mutable_handle_value()->add_int_val(" << handle_name
      << "->data[i]);\n";
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForHidlMemoryVariable(
    Formatter& out, const VariableSpecificationMessage&,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_HIDL_MEMORY);\n";
  out << arg_name << "->mutable_hidl_memory_value()->set_size"
      << "(" << arg_value << ".size());\n";
  // TODO(zhuoyao): dump the memory contents as well.
}

void HalHidlProfilerCodeGen::GenerateProfilerForPointerVariable(
    Formatter& out, const VariableSpecificationMessage&,
    const std::string& arg_name, const std::string&) {
  out << arg_name << "->set_type(TYPE_POINTER);\n";
  // TODO(zhuoyao): figure the right way to profile pointer type.
}

void HalHidlProfilerCodeGen::GenerateProfilerForFMQSyncVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_FMQ_SYNC);\n";
  string element_type = GetCppVariableType(val.fmq_value(0));
  std::string queue_name = arg_name + "_q";
  std::string temp_result_name = arg_name + "_result";
  out << "MessageQueue<" << element_type << ", kSynchronizedReadWrite> "
      << queue_name << "(" << arg_value << ", false);\n";
  out << "if (" << queue_name << ".isValid()) {\n";
  out.indent();
  out << "for (int i = 0; i < (int)" << queue_name
      << ".availableToRead(); i++) {\n";
  out.indent();
  std::string fmq_item_name = arg_name + "_item_i";
  out << "auto *" << fmq_item_name << " = " << arg_name
      << "->add_fmq_value();\n";
  out << element_type << " " << temp_result_name << ";\n";
  out << queue_name << ".read(&" << temp_result_name << ");\n";
  out << queue_name << ".write(&" << temp_result_name << ");\n";
  GenerateProfilerForTypedVariable(out, val.fmq_value(0), fmq_item_name,
                                   temp_result_name);
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForFMQUnsyncVariable(
    Formatter& out, const VariableSpecificationMessage& val,
    const std::string& arg_name, const std::string& arg_value) {
  out << arg_name << "->set_type(TYPE_FMQ_UNSYNC);\n";
  string element_type = GetCppVariableType(val.fmq_value(0));
  std::string queue_name = arg_name + "_q";
  std::string temp_result_name = arg_name + "_result";
  out << "MessageQueue<" << element_type << ", kUnsynchronizedWrite> "
      << queue_name << "(" << arg_value << ");\n";
  out << "if (" << queue_name << ".isValid()) {\n";
  out.indent();
  out << "for (int i = 0; i < (int)" << queue_name
      << ".availableToRead(); i++) {\n";
  out.indent();
  std::string fmq_item_name = arg_name + "_item_i";
  out << "auto *" << fmq_item_name << " = " << arg_name
      << "->add_fmq_value();\n";
  out << element_type << " " << temp_result_name << ";\n";
  out << queue_name << ".read(&" << temp_result_name << ");\n";
  GenerateProfilerForTypedVariable(out, val.fmq_value(0), fmq_item_name,
                                   temp_result_name);
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "}\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerForMethod(
    Formatter& out, const FunctionSpecificationMessage& method) {
  out << "FunctionSpecificationMessage msg;\n";
  out << "msg.set_name(\"" << method.name() << "\");\n";
  out << "if (!args) {\n";
  out.indent();
  out << "LOG(WARNING) << \"no argument passed\";\n";
  out.unindent();
  out << "} else {\n";
  out.indent();
  out << "switch (event) {\n";
  out.indent();
  // TODO(b/32141398): Support profiling in passthrough mode.
  out << "case details::HidlInstrumentor::CLIENT_API_ENTRY:\n";
  out << "case details::HidlInstrumentor::SERVER_API_ENTRY:\n";
  out << "case details::HidlInstrumentor::PASSTHROUGH_ENTRY:\n";
  out << "{\n";
  out.indent();
  ComponentSpecificationMessage message;
  out << "if ((*args).size() != " << method.arg().size() << ") {\n";
  out.indent();
  out << "LOG(ERROR) << \"Number of arguments does not match. expect: "
      << method.arg().size()
      << ", actual: \" << (*args).size() << \", method name: " << method.name()
      << ", event type: \" << event;\n";
  out << "break;\n";
  out.unindent();
  out << "}\n";
  for (int i = 0; i < method.arg().size(); i++) {
    const VariableSpecificationMessage arg = method.arg(i);
    std::string arg_name = "arg_" + std::to_string(i);
    std::string arg_value = "arg_val_" + std::to_string(i);
    out << "auto *" << arg_name
        << " __attribute__((__unused__)) = msg.add_arg();\n";
    out << GetCppVariableType(arg) << " *" << arg_value
        << " __attribute__((__unused__)) = reinterpret_cast<"
        << GetCppVariableType(arg) << "*> ((*args)[" << i << "]);\n";
    out << "if (" << arg_value << " != nullptr) {\n";
    out.indent();
    GenerateProfilerForTypedVariable(out, arg, arg_name,
                                     "(*" + arg_value + ")");
    out.unindent();
    out << "} else {\n";
    out.indent();
    out << "LOG(WARNING) << \"argument " << i << " is null.\";\n";
    out.unindent();
    out << "}\n";
  }
  out << "break;\n";
  out.unindent();
  out << "}\n";

  out << "case details::HidlInstrumentor::CLIENT_API_EXIT:\n";
  out << "case details::HidlInstrumentor::SERVER_API_EXIT:\n";
  out << "case details::HidlInstrumentor::PASSTHROUGH_EXIT:\n";
  out << "{\n";
  out.indent();
  out << "if ((*args).size() != " << method.return_type_hidl().size()
      << ") {\n";
  out.indent();
  out << "LOG(ERROR) << \"Number of return values does not match. expect: "
      << method.return_type_hidl().size()
      << ", actual: \" << (*args).size() << \", method name: " << method.name()
      << ", event type: \" << event;\n";
  out << "break;\n";
  out.unindent();
  out << "}\n";
  for (int i = 0; i < method.return_type_hidl().size(); i++) {
    const VariableSpecificationMessage arg = method.return_type_hidl(i);
    std::string result_name = "result_" + std::to_string(i);
    std::string result_value = "result_val_" + std::to_string(i);
    out << "auto *" << result_name
        << " __attribute__((__unused__)) = msg.add_return_type_hidl();\n";
    out << GetCppVariableType(arg) << " *" << result_value
        << " __attribute__((__unused__)) = reinterpret_cast<"
        << GetCppVariableType(arg) << "*> ((*args)[" << i << "]);\n";
    out << "if (" << result_value << " != nullptr) {\n";
    out.indent();
    GenerateProfilerForTypedVariable(out, arg, result_name,
                                     "(*" + result_value + ")");
    out.unindent();
    out << "} else {\n";
    out.indent();
    out << "LOG(WARNING) << \"return value " << i << " is null.\";\n";
    out.unindent();
    out << "}\n";
  }
  out << "break;\n";
  out.unindent();
  out << "}\n";
  out << "default:\n";
  out << "{\n";
  out.indent();
  out << "LOG(WARNING) << \"not supported. \";\n";
  out << "break;\n";
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "}\n";
  out.unindent();
  out << "}\n";
  out << "profiler.AddTraceEvent(event, package, version, interface, msg);\n";
}

void HalHidlProfilerCodeGen::GenerateHeaderIncludeFiles(
    Formatter& out, const ComponentSpecificationMessage& message) {
  // Basic includes.
  out << "#include <android-base/logging.h>\n";
  out << "#include <hidl/HidlSupport.h>\n";
  out << "#include <linux/limits.h>\n";
  out << "#include <test/vts/proto/ComponentSpecificationMessage.pb.h>\n";
  out << "#include \"VtsProfilingInterface.h\"\n";
  out << "\n";

  // Include generated hal classes.
  out << "#include <" << GetPackagePath(message) << "/" << GetVersion(message)
      << "/" << GetComponentName(message) << ".h>\n";

  // Include imported classes.
  for (const auto& import : message.import()) {
    FQName import_name = FQName(import);
    string imported_package_name = import_name.package();
    string imported_package_version = import_name.version();
    string imported_component_name = import_name.name();
    string imported_package_path = imported_package_name;
    ReplaceSubString(imported_package_path, ".", "/");
    out << "#include <" << imported_package_path << "/"
        << imported_package_version << "/" << imported_component_name
        << ".h>\n";
    // Exclude the base hal in include list.
    if (imported_package_name.find("android.hidl.base") == std::string::npos) {
      if (imported_component_name[0] == 'I') {
        imported_component_name = imported_component_name.substr(1);
      }
      out << "#include <" << imported_package_path << "/"
          << imported_package_version << "/" << imported_component_name
          << ".vts.h>\n";
    }
  }
  out << "\n\n";
}

void HalHidlProfilerCodeGen::GenerateSourceIncludeFiles(
    Formatter& out, const ComponentSpecificationMessage& message) {
  // Include the corresponding profiler header file.
  out << "#include \"" << GetPackagePath(message) << "/" << GetVersion(message)
      << "/" << GetComponentBaseName(message) << ".vts.h\"\n";
  out << "#include <cutils/ashmem.h>\n";
  out << "#include <fcntl.h>\n";
  out << "#include <fmq/MessageQueue.h>\n";
  out << "#include <sys/stat.h>\n";
  out << "\n";
}

void HalHidlProfilerCodeGen::GenerateUsingDeclaration(
    Formatter& out, const ComponentSpecificationMessage& message) {
  out << "using namespace ";
  out << GetPackageNamespaceToken(message) << "::" << GetVersion(message, true)
      << ";\n";
  out << "using namespace android::hardware;\n";
  out << "\n";
}

void HalHidlProfilerCodeGen::GenerateMacros(
    Formatter& out, const ComponentSpecificationMessage&) {
  out << "#define TRACEFILEPREFIX \"/data/local/tmp/\"\n";
  out << "\n";
}

void HalHidlProfilerCodeGen::GenerateProfilerSanityCheck(
    Formatter& out, const ComponentSpecificationMessage& message) {
  out << "if (strcmp(package, \"" << GetPackageName(message) << "\") != 0) {\n";
  out.indent();
  out << "LOG(WARNING) << \"incorrect package. Expect: "
      << GetPackageName(message) << " actual: \" << package;\n";
  out << "return;\n";
  out.unindent();
  out << "}\n";
  out << "std::string version_str = std::string(version);\n";
  out << "int major_version = stoi(version_str.substr(0, "
         "version_str.find('.')));\n";
  out << "int minor_version = stoi(version_str.substr(version_str.find('.') + "
         "1));\n";
  out << "if (major_version != " << GetMajorVersion(message)
      << " || minor_version > " << GetMinorVersion(message) << ") {\n";
  out.indent();
  out << "LOG(WARNING) << \"incorrect version. Expect: " << GetVersion(message)
      << " or lower (if version != x.0), actual: \" << version;\n";
  out << "return;\n";
  out.unindent();
  out << "}\n";

  out << "if (strcmp(interface, \"" << GetComponentName(message)
      << "\") != 0) {\n";
  out.indent();
  out << "LOG(WARNING) << \"incorrect interface. Expect: "
      << GetComponentName(message) << " actual: \" << interface;\n";
  out << "return;\n";
  out.unindent();
  out << "}\n";
  out << "\n";
}

void HalHidlProfilerCodeGen::GenerateLocalVariableDefinition(
    Formatter& out, const ComponentSpecificationMessage&) {
  // create and initialize the VTS profiler interface.
  out << "VtsProfilingInterface& profiler = "
      << "VtsProfilingInterface::getInstance(TRACEFILEPREFIX);\n\n";
}

}  // namespace vts
}  // namespace android
