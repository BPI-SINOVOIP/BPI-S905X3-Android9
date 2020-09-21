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

#include "VtsCompilerUtils.h"

#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>

#include <google/protobuf/text_format.h>

#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

string ComponentClassToString(int component_class) {
  switch (component_class) {
    case UNKNOWN_CLASS:
      return "unknown_class";
    case HAL_CONVENTIONAL:
      return "hal_conventional";
    case HAL_CONVENTIONAL_SUBMODULE:
      return "hal_conventional_submodule";
    case HAL_HIDL:
      return "hal_hidl";
    case HAL_HIDL_WRAPPED_CONVENTIONAL:
      return "hal_hidl_wrapped_conventional";
    case HAL_LEGACY:
      return "hal_legacy";
    case LIB_SHARED:
      return "lib_shared";
  }
  cerr << "error: invalid component_class " << component_class << endl;
  exit(-1);
}

string ComponentTypeToString(int component_type) {
  switch (component_type) {
    case UNKNOWN_TYPE:
      return "unknown_type";
    case AUDIO:
      return "audio";
    case CAMERA:
      return "camera";
    case GPS:
      return "gps";
    case LIGHT:
      return "light";
    case WIFI:
      return "wifi";
    case MOBILE:
      return "mobile";
    case BLUETOOTH:
      return "bluetooth";
    case TV_INPUT:
      return "tv_input";
    case NFC:
      return "nfc";
    case VEHICLE:
      return "vehicle";
    case VIBRATOR:
      return "vibrator";
    case THERMAL:
      return "thermal";
    case CONTEXTHUB:
      return "contexthub";
    case SENSORS:
      return "sensors";
    case VR:
      return "vr";
    case GRAPHICS_ALLOCATOR:
      return "graphics_allocator";
    case GRAPHICS_MAPPER:
      return "graphics_mapper";
    case GRAPHICS_COMPOSER:
      return "graphics_composer";
    case BIONIC_LIBM:
      return "bionic_libm";
    case TV_CEC:
      return "tv_cec";
    case RADIO:
      return "radio";
    case MEDIA_OMX:
      return "media_omx";
    case BIONIC_LIBC:
      return "bionic_libc";
    case VNDK_LIBCUTILS:
      return "vndk_libcutils";
  }
  cerr << "error: invalid component_type " << component_type << endl;
  exit(-1);
}

string GetCppVariableType(const std::string scalar_type_string) {
  if (scalar_type_string == "void" ||
      scalar_type_string == "int32_t" || scalar_type_string == "uint32_t" ||
      scalar_type_string == "int8_t" || scalar_type_string == "uint8_t" ||
      scalar_type_string == "int64_t" || scalar_type_string == "uint64_t" ||
      scalar_type_string == "int16_t" || scalar_type_string == "uint16_t") {
    return scalar_type_string;
  } else if (scalar_type_string == "bool_t") {
    return "bool";
  } else if (scalar_type_string == "float_t") {
    return "float";
  } else if (scalar_type_string == "double_t") {
    return "double";
  } else if (scalar_type_string == "ufloat") {
    return "unsigned float";
  } else if (scalar_type_string == "udouble") {
    return "unsigned double";
  } else if (scalar_type_string == "string") {
    return "std::string";
  } else if (scalar_type_string == "pointer") {
    return "void*";
  } else if (scalar_type_string == "char_pointer") {
    return "char*";
  } else if (scalar_type_string == "uchar_pointer") {
    return "unsigned char*";
  } else if (scalar_type_string == "void_pointer") {
    return "void*";
  } else if (scalar_type_string == "function_pointer") {
    return "void*";
  }

  cerr << __func__ << ":" << __LINE__ << " "
       << "error: unknown scalar_type " << scalar_type_string << endl;
  exit(-1);
}

string GetCppVariableType(const VariableSpecificationMessage& arg,
                          bool generate_const) {
  string result;
  switch (arg.type()) {
    case TYPE_VOID:
    {
      return "void";
    }
    case TYPE_PREDEFINED:
    {
      result = arg.predefined_type();
      break;
    }
    case TYPE_SCALAR:
    {
      result = GetCppVariableType(arg.scalar_type());
      break;
    }
    case TYPE_STRING:
    {
      result = "::android::hardware::hidl_string";
      break;
    }
    case TYPE_ENUM:
    {
      if (!arg.has_enum_value() && arg.has_predefined_type()) {
        result = arg.predefined_type();
      } else if (arg.has_enum_value() && arg.has_name()) {
        result = arg.name();  // nested enum type.
      } else {
        cerr << __func__ << ":" << __LINE__
             << " ERROR no predefined_type set for enum variable" << endl;
        exit(-1);
      }
      break;
    }
    case TYPE_VECTOR:
    {
      string element_type = GetCppVariableType(arg.vector_value(0));
      result = "::android::hardware::hidl_vec<" + element_type + ">";
      break;
    }
    case TYPE_ARRAY:
    {
      VariableSpecificationMessage cur_val = arg;
      vector<int32_t> array_sizes;
      while (cur_val.type() == TYPE_ARRAY) {
        array_sizes.push_back(cur_val.vector_size());
        VariableSpecificationMessage temp = cur_val.vector_value(0);
        cur_val = temp;
      }
      string element_type = GetCppVariableType(cur_val);
      result = "::android::hardware::hidl_array<" + element_type + ", ";
      for (size_t i = 0; i < array_sizes.size(); i++) {
        result += to_string(array_sizes[i]);
        if (i != array_sizes.size() - 1) result += ", ";
      }
      result += ">";
      break;
    }
    case TYPE_STRUCT:
    {
      if (arg.struct_value_size() == 0 && arg.has_predefined_type()) {
        result = arg.predefined_type();
      } else if (arg.has_struct_type()) {
        result = arg.struct_type();
      } else if (arg.sub_struct_size() > 0 || arg.struct_value_size() > 0) {
        result = arg.name();
      } else {
        cerr << __func__ << ":" << __LINE__ << " ERROR"
             << " no predefined_type, struct_type, nor sub_struct set"
             << " for struct variable"
             << " (arg name " << arg.name() << ")" << endl;
        exit(-1);
      }
      break;
    }
    case TYPE_UNION:
    {
      if (arg.union_value_size() == 0 && arg.has_predefined_type()) {
        result = arg.predefined_type();
      } else if (arg.has_union_type()) {
        result = arg.union_type();
      } else {
        cerr << __func__ << ":" << __LINE__
             << " ERROR no predefined_type or union_type set for union"
             << " variable" << endl;
        exit(-1);
      }
      break;
    }
    case TYPE_HIDL_CALLBACK:
    {
      if (arg.has_predefined_type()) {
        result = "sp<" + arg.predefined_type() + ">";
      } else {
        cerr << __func__ << ":" << __LINE__
             << " ERROR no predefined_type set for hidl callback variable"
             << endl;
        exit(-1);
      }
      break;
    }
    case TYPE_HANDLE:
    {
      result = "::android::hardware::hidl_handle";
      break;
    }
    case TYPE_HIDL_INTERFACE:
    {
      if (arg.has_predefined_type()) {
        result = "sp<" + arg.predefined_type() + ">";
      } else {
        cerr << __func__ << ":" << __LINE__
             << " ERROR no predefined_type set for hidl interface variable"
             << endl;
        exit(-1);
      }
      break;
    }
    case TYPE_MASK:
    {
      result = GetCppVariableType(arg.scalar_type());
      break;
    }
    case TYPE_HIDL_MEMORY:
    {
      result = "::android::hardware::hidl_memory";
      break;
    }
    case TYPE_POINTER:
    {
      result = "void*";
      if (generate_const) {
        result = "const " + result;
      }
      return result;
    }
    case TYPE_FMQ_SYNC:
    {
      string element_type = GetCppVariableType(arg.fmq_value(0));
      result = "::android::hardware::MQDescriptorSync<" + element_type + ">";
      break;
    }
    case TYPE_FMQ_UNSYNC:
    {
      string element_type = GetCppVariableType(arg.fmq_value(0));
      result = "::android::hardware::MQDescriptorUnsync<" + element_type + ">";
      break;
    }
    case TYPE_REF:
    {
      VariableSpecificationMessage cur_val = arg;
      int ref_depth = 0;
      while (cur_val.type() == TYPE_REF) {
        ref_depth++;
        VariableSpecificationMessage temp = cur_val.ref_value();
        cur_val = temp;
      }
      string element_type = GetCppVariableType(cur_val);
      result = element_type;
      for (int i = 0; i < ref_depth; i++) {
        result += " const*";
      }
      return result;
      break;
    }
    default:
    {
      cerr << __func__ << ":" << __LINE__ << " " << ": type " << arg.type()
           << " not supported" << endl;
      exit(-1);
    }
  }
  if (generate_const) {
    return "const " + result + "&";
  }
  return result;
}

string GetConversionToProtobufFunctionName(VariableSpecificationMessage arg) {
  if (arg.type() == TYPE_PREDEFINED) {
    if (arg.predefined_type() == "camera_info_t*") {
      return "ConvertCameraInfoToProtobuf";
    } else if (arg.predefined_type() == "hw_device_t**") {
      return "";
    } else {
      cerr << __FILE__ << ":" << __LINE__ << " "
           << "error: unknown instance type " << arg.predefined_type() << endl;
    }
  }
  cerr << __FUNCTION__ << ": non-supported type " << arg.type() << endl;
  exit(-1);
}

string GetCppInstanceType(
    const VariableSpecificationMessage& arg,
    const string& msg,
    const ComponentSpecificationMessage* message) {
  switch(arg.type()) {
    case TYPE_PREDEFINED: {
      if (arg.predefined_type() == "struct light_state_t*") {
        if (msg.length() == 0) {
          return "GenerateLightState()";
        } else {
          return "GenerateLightStateUsingMessage(" + msg + ")";
        }
      } else if (arg.predefined_type() == "GpsCallbacks*") {
        return "GenerateGpsCallbacks()";
      } else if (arg.predefined_type() == "GpsUtcTime") {
        return "GenerateGpsUtcTime()";
      } else if (arg.predefined_type() == "vts_gps_latitude") {
        return "GenerateLatitude()";
      } else if (arg.predefined_type() == "vts_gps_longitude") {
        return "GenerateLongitude()";
      } else if (arg.predefined_type() == "vts_gps_accuracy") {
        return "GenerateGpsAccuracy()";
      } else if (arg.predefined_type() == "vts_gps_flags_uint16") {
        return "GenerateGpsFlagsUint16()";
      } else if (arg.predefined_type() == "GpsPositionMode") {
        return "GenerateGpsPositionMode()";
      } else if (arg.predefined_type() == "GpsPositionRecurrence") {
        return "GenerateGpsPositionRecurrence()";
      } else if (arg.predefined_type() == "hw_module_t*") {
        return "(hw_module_t*) malloc(sizeof(hw_module_t))";
      } else if (arg.predefined_type() == "hw_module_t**") {
        return "(hw_module_t**) malloc(sizeof(hw_module_t*))";
      } else if (arg.predefined_type() == "hw_device_t**") {
        return "(hw_device_t**) malloc(sizeof(hw_device_t*))";
      } else if (arg.predefined_type() == "camera_info_t*") {
        if (msg.length() == 0) {
          return "GenerateCameraInfo()";
        } else {
          return "GenerateCameraInfoUsingMessage(" + msg + ")";
        }
      } else if (arg.predefined_type() == "camera_module_callbacks_t*") {
        return "GenerateCameraModuleCallbacks()";
      } else if (arg.predefined_type() == "camera_notify_callback") {
        return "GenerateCameraNotifyCallback()";
      } else if (arg.predefined_type() == "camera_data_callback") {
        return "GenerateCameraDataCallback()";
      } else if (arg.predefined_type() == "camera_data_timestamp_callback") {
        return "GenerateCameraDataTimestampCallback()";
      } else if (arg.predefined_type() == "camera_request_memory") {
        return "GenerateCameraRequestMemory()";
      } else if (arg.predefined_type() == "wifi_handle*") {
        return "(wifi_handle*) malloc(sizeof(wifi_handle))";
      } else if (arg.predefined_type() == "struct camera_device*") {
        return "(struct camera_device*) malloc(sizeof(struct camera_device))";
      } else if (arg.predefined_type() == "struct preview_stream_ops*") {
        return "(preview_stream_ops*) malloc(sizeof(preview_stream_ops))";
      } else if (endsWith(arg.predefined_type(), "*")) {
        // known use cases: bt_callbacks_t
        return "(" + arg.predefined_type() + ") malloc(sizeof("
            + arg.predefined_type().substr(0, arg.predefined_type().size() - 1)
            + "))";
      } else {
        cerr << __func__ << ":" << __LINE__ << " "
             << "error: unknown instance type " << arg.predefined_type() << endl;
      }
      break;
    }
    case TYPE_SCALAR: {
      if (arg.scalar_type() == "bool_t") {
        return "RandomBool()";
      } else if (arg.scalar_type() == "uint32_t") {
        return "RandomUint32()";
      } else if (arg.scalar_type() == "int32_t") {
        return "RandomInt32()";
      } else if (arg.scalar_type() == "uint64_t") {
        return "RandomUint64()";
      } else if (arg.scalar_type() == "int64_t") {
        return "RandomInt64()";
      } else if (arg.scalar_type() == "uint16_t") {
        return "RandomUint16()";
      } else if (arg.scalar_type() == "int16_t") {
        return "RandomInt16()";
      } else if (arg.scalar_type() == "uint8_t") {
        return "RandomUint8()";
      } else if (arg.scalar_type() == "int8_t") {
        return "RandomInt8()";
      } else if (arg.scalar_type() == "float_t") {
        return "RandomFloat()";
      } else if (arg.scalar_type() == "double_t") {
        return "RandomDouble()";
      } else if (arg.scalar_type() == "char_pointer") {
        return "RandomCharPointer()";
      } else if (arg.scalar_type() == "uchar_pointer") {
        return "(unsigned char*) RandomCharPointer()";
      } else if (arg.scalar_type() == "pointer" ||
                 arg.scalar_type() == "void_pointer") {
        return "RandomVoidPointer()";
      }
      cerr << __FILE__ << ":" << __LINE__ << " "
           << "error: unsupported scalar data type " << arg.scalar_type() << endl;
      exit(-1);
    }
    case TYPE_ENUM:
    case TYPE_MASK: {
      if (!arg.has_enum_value() && arg.has_predefined_type()) {
        if (!message || message->component_class() != HAL_HIDL) {
          return "(" + arg.predefined_type() +  ") RandomUint32()";
        } else {
          std::string predefined_type_name = arg.predefined_type();
          ReplaceSubString(predefined_type_name, "::", "__");
          return "Random" + predefined_type_name + "()";
          // TODO: generate a function which can dynamically choose the value.
          /* for (const auto& attribute : message->attribute()) {
            if (attribute.type() == TYPE_ENUM &&
                attribute.name() == arg.predefined_type()) {
              // TODO: pick at runtime
              return message->component_name() + "::"
                  + arg.predefined_type() + "::"
                  + attribute.enum_value().enumerator(0);
            }
          } */
        }
      } else {
        cerr << __func__
             << " ENUM either has enum value or doesn't have predefined type"
             << endl;
        exit(-1);
      }
      break;
    }
    case TYPE_STRING: {
      return "android::hardware::hidl_string(RandomCharPointer())";
    }
    case TYPE_STRUCT: {
      if (arg.struct_value_size() == 0 && arg.has_predefined_type()) {
        return message->component_name() + "::" + arg.predefined_type() +  "()";
      }
      break;
    }
    case TYPE_VECTOR: {  // only for HAL_HIDL
      // TODO: generate code that initializes a local hidl_vec.
      return "";
    }
    case TYPE_HIDL_CALLBACK: {
      return arg.predefined_type() + "()";
    }
    default:
      break;
  }
  cerr << __func__ << ": error: unsupported type " << arg.type() << endl;
  exit(-1);
}

int vts_fs_mkdirs(char* file_path, mode_t mode) {
  char* p;

  for (p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
    *p = '\0';
    if (mkdir(file_path, mode) == -1) {
      if (errno != EEXIST) {
        *p = '/';
        return -1;
      }
    }
    *p = '/';
  }
  return 0;
}

string ClearStringWithNameSpaceAccess(const string& str) {
  string result = str;
  ReplaceSubString(result, "::", "__");
  return result;
}

// Returns a string which joins the given dir_path and file_name.
string PathJoin(const char* dir_path, const char* file_name) {
  string result;
  if (dir_path) {
    result = dir_path;
    if (!file_name) return result;
  } else if (!file_name) return result;

  if (file_name[0] != '.') {
    if (result.c_str()[result.length()-1] != '/') {
      result += "/";
    }
  }
  result += file_name;
  return result;
}

// Returns a string which remove given base_path from file_path if included.
string RemoveBaseDir(const string& file_path, const string& base_path) {
  if (strncmp(file_path.c_str(), base_path.c_str(), base_path.length())) {
    return file_path;
  }
  string result;
  result = &file_path.c_str()[base_path.length()];
  if (result.c_str()[0] == '/') {
    result = &result.c_str()[1];
  }
  return result;
}

string GetPackageName(const ComponentSpecificationMessage& message) {
  if (!message.package().empty()) {
    return message.package();
  }
  return "";
}

string GetPackagePath(const ComponentSpecificationMessage& message) {
  string package_path = GetPackageName(message);
  ReplaceSubString(package_path, ".", "/");
  return package_path;
}

string GetPackageNamespaceToken(const ComponentSpecificationMessage& message) {
  string package_token = GetPackageName(message);
  ReplaceSubString(package_token, ".", "::");
  return package_token;
}

string GetVersion(const ComponentSpecificationMessage& message,
                  bool for_macro) {
  return GetVersionString(message.component_type_version(), for_macro);
}

int GetMajorVersion(const ComponentSpecificationMessage& message) {
  string version = GetVersion(message);
  return stoi(version.substr(0, version.find('.')));
}

int GetMinorVersion(const ComponentSpecificationMessage& message) {
  string version = GetVersion(message);
  return stoi(version.substr(version.find('.') + 1));
}

string GetComponentBaseName(const ComponentSpecificationMessage& message) {
  if (!message.component_name().empty()) {
    return (message.component_name() == "types"
                ? "types"
                : message.component_name().substr(1));
  } else
    return GetComponentName(message);
}

string GetComponentName(const ComponentSpecificationMessage& message) {
  if (!message.component_name().empty()) {
    return message.component_name();
  }

  string component_name = message.original_data_structure_name();
  while (!component_name.empty()
      && (std::isspace(component_name.back()) || component_name.back() == '*')) {
    component_name.pop_back();
  }
  const auto pos = component_name.find_last_of(" ");
  if (pos != std::string::npos) {
    component_name = component_name.substr(pos + 1);
  }
  return component_name;
}

FQName GetFQName(const ComponentSpecificationMessage& message) {
  return FQName(message.package(),
                GetVersionString(message.component_type_version()),
                GetComponentName(message));
}

string GetVarString(const string& var_name) {
  string var_str = var_name;
  for (size_t i = 0; i < var_name.length(); i++) {
    if (!isdigit(var_str[i]) && !isalpha(var_str[i])) {
      var_str[i] = '_';
    }
  }
  return var_str;
}

}  // namespace vts
}  // namespace android
