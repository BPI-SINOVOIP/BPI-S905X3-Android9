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

#ifndef VTS_COMPILATION_TOOLS_VTSC_CODE_UTILS_H_
#define VTS_COMPILATION_TOOLS_VTSC_CODE_UTILS_H_

#include <string>

#include <hidl-util/FQName.h>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

// Returns the component class name as a string
extern string ComponentClassToString(int component_class);

// Returns the component type name as a string
extern string ComponentTypeToString(int component_type);

// Returns the C/C++ variable type name of a given data type.
extern string GetCppVariableType(const string primitive_type_string);

// Returns the C/C++ basic variable type name of a given argument.
string GetCppVariableType(const VariableSpecificationMessage& arg,
                          bool generate_const = false);

// Get the C/C++ instance type name of an argument.
extern string GetCppInstanceType(
    const VariableSpecificationMessage& arg,
    const string& msg = string(),
    const ComponentSpecificationMessage* message = NULL);

// Returns the name of a function which can convert the given arg to a protobuf.
extern string GetConversionToProtobufFunctionName(
    VariableSpecificationMessage arg);

// fs_mkdirs for VTS.
extern int vts_fs_mkdirs(char* file_path, mode_t mode);

// Replace the name space access symbol "::" in the string to "__" to prevent
// mis-interpretation in generated cpp code.
string ClearStringWithNameSpaceAccess(const string& str);

// Returns a string which joins the given dir_path and file_name.
string PathJoin(const char* dir_path, const char* file_name);

// Returns a string which remove given base_path from file_path if included.
string RemoveBaseDir(const string& file_path, const string& base_path);

// Get the package name from the message if set. e.g. android.hardware.foo
string GetPackageName(const ComponentSpecificationMessage& message);

// Get the path of package from the message. e.g. android/hardware/for
string GetPackagePath(const ComponentSpecificationMessage& message);

// Get the namespace token of package from the message.
// e.g. android::hardware::foo
string GetPackageNamespaceToken(const ComponentSpecificationMessage& message);

// Get component version string from the message. e.g. 1.0
// If for_macro = true, return the version string with format like V1_0.
std::string GetVersion(const ComponentSpecificationMessage& message,
                       bool for_macro = false);

// Get component major version from the message. e.g. 1.0 -> 1
int GetMajorVersion(const ComponentSpecificationMessage& message);

// Get component minor version from the message. e.g. 1.0 -> 0
int GetMinorVersion(const ComponentSpecificationMessage& message);

// Get the base name of component from the message. e.g. typs, Foo.
std::string GetComponentBaseName(const ComponentSpecificationMessage& message);

// Get the component name from message,e.g. IFoo, IFooCallback, types etc.
string GetComponentName(const ComponentSpecificationMessage& message);

// Generate the FQName of the given message..
FQName GetFQName(const ComponentSpecificationMessage& message);

// Generate a plain string from the name of given variable, replace any special
// character (non alphabat or digital) with '_'
// e.g. msg.test --> msg_test, msg[test] --> msg_test_
string GetVarString(const string& var_name);

}  // namespace vts
}  // namespace android

#endif  // VTS_COMPILATION_TOOLS_VTSC_CODE_UTILS_H_
