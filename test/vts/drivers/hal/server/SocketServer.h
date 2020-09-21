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

#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket

#ifndef __VTS_DRIVER_HAL_SOCKET_SERVER_
#define __VTS_DRIVER_HAL_SOCKET_SERVER_

#include <VtsDriverCommUtil.h>

#include "driver_manager/VtsHalDriverManager.h"

namespace android {
namespace vts {

class VtsDriverHalSocketServer : public VtsDriverCommUtil {
 public:
  VtsDriverHalSocketServer(VtsHalDriverManager* driver_manager,
                           const char* lib_path)
      : VtsDriverCommUtil(),
        driver_manager_(driver_manager),
        lib_path_(lib_path) {}

  // Start a session to handle a new request.
  bool ProcessOneCommand();

 protected:
  void Exit();

  // Load a Hal driver with the given info (package, version etc.),
  // returns the loaded hal driver id if scuccess, -1 otherwise.
  int32_t LoadHal(const string& path, int target_class, int target_type,
                  float target_version, const string& target_package,
                  const string& target_component_name,
                  const string& hw_binder_service_name,
                  const string& module_name);
  string ReadSpecification(const string& name, int target_class,
                           int target_type, float target_version,
                           const string& target_package);
  string Call(const string& arg);
  string GetAttribute(const string& arg);
  string ListFunctions() const;

 private:
  android::vts::VtsHalDriverManager* driver_manager_;
  const char* lib_path_;
};

extern int StartSocketServer(const string& socket_port_file,
                             VtsHalDriverManager* driver_manager,
                             const char* lib_path);

}  // namespace vts
}  // namespace android

#endif  // __VTS_DRIVER_HAL_SOCKET_SERVER_

#endif  // VTS_AGENT_DRIVER_COMM_BINDER
