/*
 * Copyright 2017 The Android Open Source Project
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

#ifndef __VTS_DRIVER_HAL_VTSHALDRIVERMANAGER_H
#define __VTS_DRIVER_HAL_VTSHALDRIVERMANAGER_H

#include <map>
#include <string>

#include "component_loader/HalDriverLoader.h"
#include "driver_base/DriverBase.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;
using DriverId = int32_t;

namespace android {
namespace vts {

class VtsHalDriverManager {
 public:
  // Constructor where the first argument is the path of a dir which contains
  // all available interface specification files.
  VtsHalDriverManager(const string& spec_dir, const int epoch_count,
                      const string& callback_socket_name);

  // Loads the driver library for the target HAL, creates the corresponding
  // driver instance, assign it a driver id and registers the created driver
  // instance in hal_driver_map_.
  // Returns the generated driver id.
  DriverId LoadTargetComponent(const string& dll_file_name,
                               const string& spec_lib_file_path,
                               const int component_class,
                               const int component_type, const float version,
                               const string& package_name,
                               const string& component_name,
                               const string& hw_binder_service_name);

  // Call the API specified in func_msg with the provided parameter using the
  // the corresonding driver instance. If func_msg specified the driver_id,
  // use the driver instance corresponds to driver_id, otherwise, use the
  // default driver instance (with driver_id = 0).
  // Returns a string which contians the return results (a text format of the
  // returned protobuf).
  // For error cases, returns string "error";
  // TODO (zhuoyao): use FunctionCallMessage instead of
  // FunctionSpecificationMessage which contains info such as component name and
  // driver id.
  string CallFunction(FunctionCallMessage* func_msg);

  // Searches hal_driver_map_ for Hidl HAL driver instance with the given
  // package name, version and component (interface) name. If found, returns
  // the correponding driver instance, otherwise, creates a new driver instance
  // with the given info, registers it in hal_driver_map_ and returns the
  // generated driver instance. This is used by VTS replay test.
  DriverId GetDriverIdForHidlHalInterface(const string& package_name,
                                          const float version,
                                          const string& interface_name,
                                          const string& hal_service_name);

  // Verify the return result of a function call matches the expected result.
  // This is used by VTS replay test.
  bool VerifyResults(DriverId id,
                     const FunctionSpecificationMessage& expected_result,
                     const FunctionSpecificationMessage& actual_result);

  // Loads the specification message for component with given component info
  // such as component_class etc. Used to server the ReadSpecification request
  // from host.
  // Returns true if load successfully, false otherwise.
  bool FindComponentSpecification(const int component_class,
                                  const int component_type, const float version,
                                  const string& package_name,
                                  const string& component_name,
                                  ComponentSpecificationMessage* spec_msg);

  // Returns the specification message for default driver. Used to serve the
  // ListFunctions request from host.
  // TODO (zhuoyao): needs to revisit this after supporting multi-hal testing.
  ComponentSpecificationMessage* GetComponentSpecification();

  // Used to serve the GetAttribute request from host. Only supported by
  // conventional HAL.
  // TODO (zhuoyao): consider deprecate this method.
  string GetAttribute(FunctionCallMessage* func_msg);

 private:
  // Internal method to register a HAL driver in hal_driver_map_.
  // Returns the driver id of registed driver.
  DriverId RegisterDriver(std::unique_ptr<DriverBase> driver,
                          const ComponentSpecificationMessage& spec_msg,
                          const uint64_t interface_pt);

  // Internal method to get the HAL driver based on the driver id. Returns
  // nullptr if no driver instance existes with given id.
  DriverBase* GetDriverById(const DriverId id);

  // Internal method to get the registered driver pointer based on driver id.
  // Returns -1 if no driver instance existes with given id.
  uint64_t GetDriverPointerById(const DriverId id);

  // Internal method to get the HAL driver based on FunctionCallMessage.
  DriverBase* GetDriverWithCallMsg(const FunctionCallMessage& call_msg);

  // Internal method to find the driver id based on component spec and
  // (for Hidl HAL) address to the hidl proxy.
  DriverId FindDriverIdInternal(const ComponentSpecificationMessage& spec_msg,
                                const uint64_t interface_pt = 0,
                                bool with_interface_pointer = false);

  // Internal method to process function return results for library.
  string ProcessFuncResultsForLibrary(FunctionSpecificationMessage* func_msg,
                                      void* result);

  // Util method to generate debug message with component info.
  string GetComponentDebugMsg(const int component_class,
                              const int component_type, const string& version,
                              const string& package_name,
                              const string& component_name);
  // ============== attributes ===================

  // The server socket port # of the agent.
  const string callback_socket_name_;
  // A HalDriverLoader instance.
  HalDriverLoader hal_driver_loader_;

  // struct that store the driver instance and its corresponding meta info.
  struct HalDriverInfo {
    // Spcification for the HAL.
    ComponentSpecificationMessage spec_msg;
    // Pointer to the HAL client proxy, used for HIDL HAL only.
    uint64_t hidl_hal_proxy_pt;
    // A HAL driver instance.
    std::unique_ptr<DriverBase> driver;

    // Constructor for halDriverInfo
    HalDriverInfo(const ComponentSpecificationMessage& spec_msg,
                  const uint64_t interface_pt,
                  std::unique_ptr<DriverBase> driver)
        : spec_msg(spec_msg),
          hidl_hal_proxy_pt(interface_pt),
          driver(std::move(driver)) {}
  };
  // map to keep all the active HAL driver instances and their corresponding
  // meta info.
  // TODO(zhuoyao): consider to use unordered_map for performance optimization.
  map<DriverId, HalDriverInfo> hal_driver_map_;
  // TODO(zhuoyao): use mutex to protect hal_driver_map_;
};

}  // namespace vts
}  // namespace android
#endif  //__VTS_DRIVER_HAL_VTSHALDRIVERMANAGER_H
