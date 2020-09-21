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
#include "VtsHidlHalReplayer.h"

#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>

#include <android-base/logging.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "VtsProfilingUtil.h"
#include "driver_base/DriverBase.h"
#include "utils/InterfaceSpecUtil.h"
#include "utils/StringUtil.h"

using namespace std;

static constexpr const char* kErrorString = "error";
static constexpr const char* kVoidString = "void";
static constexpr const int kInvalidDriverId = -1;

namespace android {
namespace vts {

void VtsHidlHalReplayer::ListTestServices(const string& trace_file) {
  // Parse the trace file to get the sequence of function calls.
  int fd = open(trace_file.c_str(), O_RDONLY);
  if (fd < 0) {
    LOG(ERROR) << "Can not open trace file: " << trace_file
               << " error: " << std::strerror(errno);
    return;
  }

  google::protobuf::io::FileInputStream input(fd);

  VtsProfilingRecord msg;
  set<string> registeredHalServices;
  while (readOneDelimited(&msg, &input)) {
    string package_name = msg.package();
    float version = msg.version();
    string interface_name = msg.interface();
    string service_fq_name =
        GetInterfaceFQName(package_name, version, interface_name);
    registeredHalServices.insert(service_fq_name);
  }
  for (string service : registeredHalServices) {
    cout << "hal_service: " << service << endl;
  }
}

bool VtsHidlHalReplayer::ReplayTrace(
    const string& trace_file, map<string, string>& hal_service_instances) {
  // Parse the trace file to get the sequence of function calls.
  int fd = open(trace_file.c_str(), O_RDONLY);
  if (fd < 0) {
    LOG(ERROR) << "Can not open trace file: " << trace_file
               << "error: " << std::strerror(errno);
    return false;
  }

  google::protobuf::io::FileInputStream input(fd);

  VtsProfilingRecord call_msg;
  VtsProfilingRecord expected_result_msg;
  while (readOneDelimited(&call_msg, &input) &&
         readOneDelimited(&expected_result_msg, &input)) {
    if (call_msg.event() != InstrumentationEventType::SERVER_API_ENTRY &&
        call_msg.event() != InstrumentationEventType::CLIENT_API_ENTRY &&
        call_msg.event() != InstrumentationEventType::SYNC_CALLBACK_ENTRY &&
        call_msg.event() != InstrumentationEventType::ASYNC_CALLBACK_ENTRY &&
        call_msg.event() != InstrumentationEventType::PASSTHROUGH_ENTRY) {
      LOG(WARNING) << "Expected a call message but got message with event: "
                   << call_msg.event();
      continue;
    }
    if (expected_result_msg.event() !=
            InstrumentationEventType::SERVER_API_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::CLIENT_API_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::SYNC_CALLBACK_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::ASYNC_CALLBACK_EXIT &&
        expected_result_msg.event() !=
            InstrumentationEventType::PASSTHROUGH_EXIT) {
      LOG(WARNING) << "Expected a result message but got message with event: "
                   << call_msg.event();
      continue;
    }

    string package_name = call_msg.package();
    float version = call_msg.version();
    string interface_name = call_msg.interface();
    string instance_name =
        GetInterfaceFQName(package_name, version, interface_name);
    string hal_service_name = "default";

    if (hal_service_instances.find(instance_name) ==
        hal_service_instances.end()) {
      LOG(WARNING) << "Does not find service name for " << instance_name
                   << "; this could be a nested interface.";
    } else {
      hal_service_name = hal_service_instances[instance_name];
    }

    cout << "Replay function: " << call_msg.func_msg().name() << endl;
    LOG(DEBUG) << "Replay function: " << call_msg.func_msg().DebugString();

    int32_t driver_id = driver_manager_->GetDriverIdForHidlHalInterface(
        package_name, version, interface_name, hal_service_name);
    if (driver_id == kInvalidDriverId) {
      LOG(ERROR) << "Couldn't get a driver base class";
      return false;
    }

    vts::FunctionCallMessage func_call_msg;
    func_call_msg.set_component_class(HAL_HIDL);
    func_call_msg.set_hal_driver_id(driver_id);
    *func_call_msg.mutable_api() = call_msg.func_msg();
    const string& result = driver_manager_->CallFunction(&func_call_msg);
    if (result == kVoidString || result == kErrorString) {
      LOG(ERROR) << "Replay function fail. Failed function call: "
                 << func_call_msg.DebugString();
      return false;
    }
    vts::FunctionSpecificationMessage result_msg;
    if (!google::protobuf::TextFormat::ParseFromString(result, &result_msg)) {
      LOG(ERROR) << "Failed to parse result msg.";
      return false;
    }
    if (!driver_manager_->VerifyResults(
            driver_id, expected_result_msg.func_msg(), result_msg)) {
      // Verification is not strict, i.e. if fail, output error message and
      // continue the process.
      LOG(WARNING) << "Verification fail. Expected: "
                   << expected_result_msg.func_msg().DebugString()
                   << " Actual: " << result_msg.DebugString();
    }
    call_msg.Clear();
    expected_result_msg.Clear();
  }
  return true;
}

}  // namespace vts
}  // namespace android
