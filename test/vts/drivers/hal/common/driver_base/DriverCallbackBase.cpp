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
#define LOG_TAG "VtsHalDriverCallbackBase"

#include "driver_base/DriverCallbackBase.h"

#include <VtsDriverCommUtil.h>
#include <android-base/logging.h>

#include "component_loader/DllLoader.h"
#include "test/vts/proto/AndroidSystemControlMessage.pb.h"
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"
#include "utils/InterfaceSpecUtil.h"

using namespace std;

namespace android {
namespace vts {

static std::map<string, string> id_map_;

DriverCallbackBase::DriverCallbackBase() {}

DriverCallbackBase::~DriverCallbackBase() {}

bool DriverCallbackBase::Register(const VariableSpecificationMessage& message) {
  LOG(DEBUG) << "type = " << message.type();
  if (!message.is_callback()) {
    LOG(ERROR) << "ERROR: argument is not a callback.";
    return false;
  }

  if (!message.has_type() || message.type() != TYPE_FUNCTION_POINTER) {
    LOG(ERROR) << "ERROR: inconsistent message.";
    return false;
  }

  for (const auto& func_pt : message.function_pointer()) {
    LOG(DEBUG) << "map[" << func_pt.function_name() << "] = " << func_pt.id();
    id_map_[func_pt.function_name()] = func_pt.id();
  }
  return true;
}

const char* DriverCallbackBase::GetCallbackID(const string& name) {
  // TODO: handle when not found.
  LOG(DEBUG) << "GetCallbackID for " << name << "returns '"
             << id_map_[name].c_str() << "'";
  return id_map_[name].c_str();
}

void DriverCallbackBase::RpcCallToAgent(
    const AndroidSystemCallbackRequestMessage& message,
    const string& callback_socket_name) {
  LOG(DEBUG) << " id = '" << message.id() << "'";
  if (message.id().empty() || callback_socket_name.empty()) {
    LOG(DEBUG) << "Abort callback forwarding.";
    return;
  }
  VtsDriverCommUtil util;
  if (!util.Connect(callback_socket_name)) exit(-1);
  util.VtsSocketSendMessage(message);
  util.Close();
}

}  // namespace vts
}  // namespace android
