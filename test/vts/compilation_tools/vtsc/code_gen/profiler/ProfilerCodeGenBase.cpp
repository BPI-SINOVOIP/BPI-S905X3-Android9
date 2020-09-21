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

#include "ProfilerCodeGenBase.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"
#include "VtsCompilerUtils.h"

namespace android {
namespace vts {

void ProfilerCodeGenBase::GenerateAll(
    Formatter& header_out, Formatter& source_out,
    const ComponentSpecificationMessage& message) {
  GenerateHeaderFile(header_out, message);
  GenerateSourceFile(source_out, message);
}

void ProfilerCodeGenBase::GenerateHeaderFile(
    Formatter& out, const ComponentSpecificationMessage& message) {
  FQName component_fq_name = GetFQName(message);
  out << "#ifndef __VTS_PROFILER_" << component_fq_name.tokenName()
      << "__\n";
  out << "#define __VTS_PROFILER_" << component_fq_name.tokenName()
      << "__\n";
  out << "\n\n";
  GenerateHeaderIncludeFiles(out, message);
  GenerateUsingDeclaration(out, message);
  GenerateOpenNameSpaces(out, message);

  if (message.has_interface()) {
    InterfaceSpecificationMessage interface = message.interface();
    // First generate the declaration of profiler functions for all user
    // defined types within the interface.
    for (const auto attribute : interface.attribute()) {
      GenerateProfilerMethodDeclForAttribute(out, attribute);
    }

    out << "extern \"C\" {\n";
    out.indent();

    // Generate the declaration of main profiler function.
    FQName component_fq_name = GetFQName(message);
    out << "\nvoid HIDL_INSTRUMENTATION_FUNCTION_" << component_fq_name.tokenName()
        << "(\n";
    out.indent();
    out.indent();
    out << "details::HidlInstrumentor::InstrumentationEvent event,\n";
    out << "const char* package,\n";
    out << "const char* version,\n";
    out << "const char* interface,\n";
    out << "const char* method,\n";
    out << "std::vector<void *> *args);\n";
    out.unindent();
    out.unindent();

    out.unindent();
    out << "}\n\n";
  } else {
    // For types.vts, just generate the declaration of profiler functions
    // for all user defined types.
    for (const auto attribute : message.attribute()) {
      GenerateProfilerMethodDeclForAttribute(out, attribute);
    }
  }

  GenerateCloseNameSpaces(out, message);
  out << "#endif\n";
}

void ProfilerCodeGenBase::GenerateSourceFile(
    Formatter& out, const ComponentSpecificationMessage& message) {
  GenerateSourceIncludeFiles(out, message);
  GenerateUsingDeclaration(out, message);
  GenerateMacros(out, message);
  GenerateOpenNameSpaces(out, message);

  if (message.has_interface()) {
    InterfaceSpecificationMessage interface = message.interface();
    // First generate profiler functions for all user defined types within
    // the interface.
    for (const auto attribute : interface.attribute()) {
      GenerateProfilerMethodImplForAttribute(out, attribute);
    }
    // Generate the main profiler function.
    FQName component_fq_name = GetFQName(message);
    out << "\nvoid HIDL_INSTRUMENTATION_FUNCTION_" << component_fq_name.tokenName()
        << "(\n";
    out.indent();
    out.indent();
    out << "details::HidlInstrumentor::InstrumentationEvent event "
           "__attribute__((__unused__)),\n";
    out << "const char* package,\n";
    out << "const char* version,\n";
    out << "const char* interface,\n";
    out << "const char* method __attribute__((__unused__)),\n";
    out << "std::vector<void *> *args __attribute__((__unused__))) {\n";
    out.unindent();

    // Generate code for sanity check.
    GenerateProfilerSanityCheck(out, message);

    if (interface.api_size() > 0) {
      // Generate code to define local variables.
      GenerateLocalVariableDefinition(out, message);

      // Generate the profiler code for each method.
      for (const FunctionSpecificationMessage api : interface.api()) {
        out << "if (strcmp(method, \"" << api.name() << "\") == 0) {\n";
        out.indent();
        GenerateProfilerForMethod(out, api);
        out.unindent();
        out << "}\n";
      }
    }

    out.unindent();
    out << "}\n\n";
  } else {
    // For types.vts, just generate profiler functions for the user defined
    // types.
    for (const auto attribute : message.attribute()) {
      GenerateProfilerMethodImplForAttribute(out, attribute);
    }
  }

  GenerateCloseNameSpaces(out, message);
}

void ProfilerCodeGenBase::GenerateProfilerForTypedVariable(Formatter& out,
  const VariableSpecificationMessage& val, const std::string& arg_name,
  const std::string& arg_value) {
  switch (val.type()) {
    case TYPE_SCALAR:
    {
      GenerateProfilerForScalarVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_STRING:
    {
      GenerateProfilerForStringVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_ENUM:
    {
      GenerateProfilerForEnumVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_VECTOR:
    {
      GenerateProfilerForVectorVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_ARRAY:
    {
      GenerateProfilerForArrayVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_STRUCT:
    {
      GenerateProfilerForStructVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_UNION: {
       GenerateProfilerForUnionVariable(out, val, arg_name, arg_value);
       break;
    }
    case TYPE_HIDL_CALLBACK:
    {
      GenerateProfilerForHidlCallbackVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_HIDL_INTERFACE:
    {
      GenerateProfilerForHidlInterfaceVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_MASK:
    {
      GenerateProfilerForMaskVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_HANDLE: {
      GenerateProfilerForHandleVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_HIDL_MEMORY:
    {
      GenerateProfilerForHidlMemoryVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_POINTER:
    {
      GenerateProfilerForPointerVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_FMQ_SYNC:
    {
      GenerateProfilerForFMQSyncVariable(out, val, arg_name, arg_value);
      break;
    }
    case TYPE_FMQ_UNSYNC:
    {
      GenerateProfilerForFMQUnsyncVariable(out, val, arg_name, arg_value);
      break;
    }
    default:
    {
      cout << "not supported.\n";
    }
  }
}

void ProfilerCodeGenBase::GenerateProfilerMethodDeclForAttribute(Formatter& out,
  const VariableSpecificationMessage& attribute) {
  if (attribute.type() == TYPE_STRUCT || attribute.type() == TYPE_UNION) {
    // Recursively generate profiler method declaration for all sub_types.
    for (const auto sub_struct : attribute.sub_struct()) {
      GenerateProfilerMethodDeclForAttribute(out, sub_struct);
    }
    for (const auto sub_union : attribute.sub_union()) {
      GenerateProfilerMethodDeclForAttribute(out, sub_union);
    }
  }
  std::string attribute_name = attribute.name();
  ReplaceSubString(attribute_name, "::", "__");
  out << "void profile__" << attribute_name
      << "(VariableSpecificationMessage* arg_name,\n" << attribute.name()
      << " arg_val_name);\n";
}

void ProfilerCodeGenBase::GenerateProfilerMethodImplForAttribute(
    Formatter& out, const VariableSpecificationMessage& attribute) {
  if (attribute.type() == TYPE_STRUCT || attribute.type() == TYPE_UNION) {
    // Recursively generate profiler method implementation for all sub_types.
    for (const auto sub_struct : attribute.sub_struct()) {
      GenerateProfilerMethodImplForAttribute(out, sub_struct);
    }
    for (const auto sub_union : attribute.sub_union()) {
      GenerateProfilerMethodImplForAttribute(out, sub_union);
    }
  }
  std::string attribute_name = attribute.name();
  ReplaceSubString(attribute_name, "::", "__");
  out << "void profile__" << attribute_name
      << "(VariableSpecificationMessage* arg_name,\n"
      << attribute.name() << " arg_val_name __attribute__((__unused__))) {\n";
  out.indent();
  GenerateProfilerForTypedVariable(out, attribute, "arg_name", "arg_val_name");
  out.unindent();
  out << "}\n\n";
}

void ProfilerCodeGenBase::GenerateOpenNameSpaces(Formatter& out,
    const ComponentSpecificationMessage& /*message*/) {
  out << "namespace android {\n";
  out << "namespace vts {\n";
}

void ProfilerCodeGenBase::GenerateCloseNameSpaces(Formatter& out,
    const ComponentSpecificationMessage& /*message*/) {
  out << "}  // namespace vts\n";
  out << "}  // namespace android\n";
}

}  // namespace vts
}  // namespace android
