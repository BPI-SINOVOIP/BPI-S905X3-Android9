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

#ifndef __VTS_SYSFUZZER_COMMON_SPECPARSER_SPECBUILDER_H__
#define __VTS_SYSFUZZER_COMMON_SPECPARSER_SPECBUILDER_H__

#include <queue>
#include <string>

#include "component_loader/DllLoader.h"
#include "driver_base/DriverBase.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {
// Builder of an interface specification.
class HalDriverLoader {
 public:
  // Constructor where the first argument is the path of a dir which contains
  // all available interface specification files.
  HalDriverLoader(const string dir_path, int epoch_count,
                  const string& callback_socket_name);

  // Scans the dir and returns an component specification for a requested
  // component.
  bool FindComponentSpecification(const int component_class,
                                  const string& package_name,
                                  const float version,
                                  const string& component_name,
                                  const int component_type,
                                  ComponentSpecificationMessage* spec_msg);

  // Create driver for given component.
  DriverBase* GetDriver(const string& driver_lib_path,
                        const ComponentSpecificationMessage& spec_msg,
                        const string& hw_binder_service_name,
                        const uint64_t interface_pt,
                        bool with_interface_pointer,
                        const string& dll_file_name);

  // Returns FuzzBase for a given interface specification, and adds all the
  // found functions to the fuzzing job queue.
  // TODO (zhuoyao): consider to deprecate this method.
  DriverBase* GetFuzzerBaseAndAddAllFunctionsToQueue(
      const char* driver_lib_path,
      const ComponentSpecificationMessage& iface_spec_msg,
      const char* dll_file_name, const char* hw_service_name);

  // Main function for the VTS system fuzzer where dll_file_name is the path of
  // a target component, spec_lib_file_path is the path of a specification
  // library file, and the rest three arguments are the basic information of
  // the target component.
  // TODO (zhuoyao): consider to deprecate this method.
  bool Process(const char* dll_file_name, const char* spec_lib_file_path,
               int target_class, int target_type, float target_version,
               const char* target_package, const char* target_component_name,
               const char* hal_service_name);

 private:
  // Internal method to create driver for library.
  DriverBase* GetLibDriver(const string& driver_lib_path,
                           const ComponentSpecificationMessage& spec_msg,
                           const string& dll_file_name);

  // Internal method to create driver for HIDL HAL.
  DriverBase* GetHidlHalDriver(const string& driver_lib_path,
                               const ComponentSpecificationMessage& spec_msg,
                               const string& hal_service_name,
                               const uint64_t interface_pt,
                               bool with_interface_pt);

  // Internal method to create driver for HIDL HAL with interface pointer.
  DriverBase* LoadDriverWithInterfacePointer(
      const string& driver_lib_path,
      const ComponentSpecificationMessage& spec_msg,
      const uint64_t interface_pt);

  // Helper method to load a driver library with the given path.
  DriverBase* LoadDriver(const string& driver_lib_path,
                         const ComponentSpecificationMessage& spec_msg);

  // A DLL Loader instance used to load the driver library.
  DllLoader dll_loader_;
  // the path of a dir which contains interface specification ASCII proto files.
  const string dir_path_;
  // the total number of epochs
  const int epoch_count_;
  // the server socket port # of the agent.
  const string callback_socket_name_;
  // fuzzing job queue. Used by Process method.
  queue<pair<FunctionSpecificationMessage*, DriverBase*>> job_queue_;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMMON_SPECPARSER_SPECBUILDER_H__
