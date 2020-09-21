/*
 * Copyright (C) 2016 The Android Open Source Project
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
#define LOG_TAG "VtsHalDriverLoader"

#include "component_loader/HalDriverLoader.h"

#include <dirent.h>

#include <android-base/logging.h>
#include <cutils/properties.h>
#include <google/protobuf/text_format.h>

#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

static constexpr const char* kSpecFileExt = ".vts";
static constexpr const char* kDefaultHwbinderServiceName = "default";

namespace android {
namespace vts {

HalDriverLoader::HalDriverLoader(const string dir_path, int epoch_count,
                                 const string& callback_socket_name)
    : dir_path_(dir_path),
      epoch_count_(epoch_count),
      callback_socket_name_(callback_socket_name) {}

bool HalDriverLoader::FindComponentSpecification(
    const int component_class, const string& package_name, const float version,
    const string& component_name, const int component_type,
    ComponentSpecificationMessage* spec_msg) {
  DIR* dir;
  struct dirent* ent;

  // Derive the package-specific dir which contains .vts files
  string driver_lib_dir = dir_path_;
  if (!endsWith(driver_lib_dir, "/")) {
    driver_lib_dir += "/";
  }
  string package_path = package_name;
  ReplaceSubString(package_path, ".", "/");
  driver_lib_dir += package_path + "/";
  driver_lib_dir += GetVersionString(version);

  if (!(dir = opendir(driver_lib_dir.c_str()))) {
    LOG(ERROR) << "Can't open dir " << driver_lib_dir;
    return false;
  }

  while ((ent = readdir(dir))) {
    if (ent->d_type == DT_REG &&
        string(ent->d_name).find(kSpecFileExt) != std::string::npos) {
      LOG(DEBUG) << "Checking a file " << ent->d_name;
      const string file_path = driver_lib_dir + "/" + string(ent->d_name);
      if (ParseInterfaceSpec(file_path.c_str(), spec_msg)) {
        if (spec_msg->component_class() != component_class) {
          continue;
        }
        if (spec_msg->component_class() != HAL_HIDL) {
          if (spec_msg->component_type() != component_type ||
              spec_msg->component_type_version() != version) {
            continue;
          }
          closedir(dir);
          return true;
        } else {
          if (spec_msg->package() != package_name ||
              spec_msg->component_type_version() != version) {
            continue;
          }
          if (!component_name.empty()) {
            if (spec_msg->component_name() != component_name) {
              continue;
            }
          }
          closedir(dir);
          return true;
        }
      }
    }
  }
  closedir(dir);
  return false;
}

DriverBase* HalDriverLoader::GetDriver(
    const string& driver_lib_path,
    const ComponentSpecificationMessage& spec_msg,
    const string& hw_binder_service_name, const uint64_t interface_pt,
    bool with_interface_pointer, const string& dll_file_name) {
  DriverBase* driver = nullptr;
  if (spec_msg.component_class() == HAL_HIDL) {
    driver = GetHidlHalDriver(driver_lib_path, spec_msg, hw_binder_service_name,
                              interface_pt, with_interface_pointer);
  } else {
    driver = GetLibDriver(driver_lib_path, spec_msg, dll_file_name);
  }
  LOG(DEBUG) << "Loaded target comp";

  return driver;
}

DriverBase* HalDriverLoader::GetLibDriver(
    const string& driver_lib_path,
    const ComponentSpecificationMessage& spec_msg,
    const string& dll_file_name) {
  DriverBase* driver = LoadDriver(driver_lib_path, spec_msg);
  if (!driver) {
    LOG(ERROR) << "Couldn't get a driver base class";
    return nullptr;
  }
  if (!driver->LoadTargetComponent(dll_file_name.c_str())) {
    LOG(ERROR) << "Couldn't load target component file, " << dll_file_name;
    return nullptr;
  }
  return driver;
}

DriverBase* HalDriverLoader::GetFuzzerBaseAndAddAllFunctionsToQueue(
    const char* driver_lib_path,
    const ComponentSpecificationMessage& iface_spec_msg,
    const char* dll_file_name, const char* hw_service_name) {
  DriverBase* driver = GetDriver(driver_lib_path, iface_spec_msg,
                                 hw_service_name, 0, false, dll_file_name);
  if (!driver) {
    LOG(ERROR) << "Couldn't get a driver base class";
    return NULL;
  }

  for (const FunctionSpecificationMessage& func_msg :
       iface_spec_msg.interface().api()) {
    LOG(DEBUG) << "Add a job " << func_msg.name();
    FunctionSpecificationMessage* func_msg_copy = func_msg.New();
    func_msg_copy->CopyFrom(func_msg);
    job_queue_.push(make_pair(func_msg_copy, driver));
  }
  return driver;
}

DriverBase* HalDriverLoader::GetHidlHalDriver(
    const string& driver_lib_path,
    const ComponentSpecificationMessage& spec_msg,
    const string& hal_service_name, const uint64_t interface_pt,
    bool with_interface_pt) {
  string package_name = spec_msg.package();

  DriverBase* driver = nullptr;
  if (with_interface_pt) {
    driver =
        LoadDriverWithInterfacePointer(driver_lib_path, spec_msg, interface_pt);
  } else {
    driver = LoadDriver(driver_lib_path, spec_msg);
  }
  if (!driver) {
    LOG(ERROR) << "Couldn't get a driver base class";
    return nullptr;
  }
  LOG(DEBUG) << "Got Hidl Hal driver";

  if (!with_interface_pt) {
    string service_name;
    if (!hal_service_name.empty()) {
      service_name = hal_service_name;
    } else {
      service_name = kDefaultHwbinderServiceName;
    }

    char get_sub_property[PROPERTY_VALUE_MAX];
    bool get_stub = false; /* default is binderized */
    if (property_get("vts.hidl.get_stub", get_sub_property, "") > 0) {
      if (!strcmp(get_sub_property, "true") ||
          !strcmp(get_sub_property, "True") || !strcmp(get_sub_property, "1")) {
        get_stub = true;
      }
    }
    if (!driver->GetService(get_stub, service_name.c_str())) {
      LOG(ERROR) << "Couldn't get hal service";
      return nullptr;
    }
  } else {
    LOG(INFO) << "Created DriverBase with interface pointer:" << interface_pt;
  }
  LOG(DEBUG) << "Loaded target comp";
  return driver;
}

DriverBase* HalDriverLoader::LoadDriver(
    const string& driver_lib_path,
    const ComponentSpecificationMessage& spec_msg) {
  if (!dll_loader_.Load(driver_lib_path.c_str())) {
    LOG(ERROR) << "Failed to load  " << driver_lib_path;
    return nullptr;
  }
  LOG(DEBUG) << "DLL loaded " << driver_lib_path;
  string function_name_prefix = GetFunctionNamePrefix(spec_msg);
  loader_function func =
      dll_loader_.GetLoaderFunction(function_name_prefix.c_str());
  if (!func) {
    LOG(ERROR) << "Function not found.";
    return nullptr;
  }
  LOG(DEBUG) << "Function found; trying to call.";
  DriverBase* driver = func();
  return driver;
}

DriverBase* HalDriverLoader::LoadDriverWithInterfacePointer(
    const string& driver_lib_path,
    const ComponentSpecificationMessage& spec_msg,
    const uint64_t interface_pt) {
  // Assumption: no shared library lookup is needed because that is handled
  // the by the driver's linking dependency.
  // Example: name (android::hardware::gnss::V1_0::IAGnssRil) converted to
  // function name (vts_func_4_android_hardware_tests_bar_V1_0_IBar_with_arg)
  if (!dll_loader_.Load(driver_lib_path.c_str())) {
    LOG(ERROR) << "Failed to load  " << driver_lib_path;
    return nullptr;
  }
  LOG(DEBUG) << "DLL loaded " << driver_lib_path;
  string function_name_prefix = GetFunctionNamePrefix(spec_msg);
  function_name_prefix += "with_arg";
  loader_function_with_arg func =
      dll_loader_.GetLoaderFunctionWithArg(function_name_prefix.c_str());
  if (!func) {
    LOG(ERROR) << "Function not found.";
    return nullptr;
  }
  return func(interface_pt);
}

bool HalDriverLoader::Process(const char* dll_file_name,
                              const char* spec_lib_file_path, int target_class,
                              int target_type, float target_version,
                              const char* target_package,
                              const char* target_component_name,
                              const char* hal_service_name) {
  ComponentSpecificationMessage interface_specification_message;
  if (!FindComponentSpecification(target_class, target_package, target_version,
                                  target_component_name, target_type,
                                  &interface_specification_message)) {
    LOG(ERROR) << "No interface specification file found for class "
               << target_class << " type " << target_type << " version "
               << target_version;
    return false;
  }

  if (!GetFuzzerBaseAndAddAllFunctionsToQueue(
          spec_lib_file_path, interface_specification_message, dll_file_name,
          hal_service_name)) {
    return false;
  }

  for (int i = 0; i < epoch_count_; i++) {
    // by default, breath-first-searching is used.
    if (job_queue_.empty()) {
      LOG(ERROR) << "No more job to process; stopping after epoch " << i;
      break;
    }

    pair<vts::FunctionSpecificationMessage*, DriverBase*> curr_job =
        job_queue_.front();
    job_queue_.pop();

    vts::FunctionSpecificationMessage* func_msg = curr_job.first;
    DriverBase* func_fuzzer = curr_job.second;

    void* result;
    FunctionSpecificationMessage result_msg;
    LOG(INFO) << "Iteration " << (i + 1) << " Function " << func_msg->name();
    // For Hidl HAL, use CallFunction method.
    if (interface_specification_message.component_class() == HAL_HIDL) {
      func_fuzzer->CallFunction(*func_msg, callback_socket_name_, &result_msg);
    } else {
      func_fuzzer->Fuzz(func_msg, &result, callback_socket_name_);
    }
  }

  return true;
}

}  // namespace vts
}  // namespace android
