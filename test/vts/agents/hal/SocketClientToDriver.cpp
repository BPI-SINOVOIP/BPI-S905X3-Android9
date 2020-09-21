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
#define LOG_TAG "VtsAgentSocketClient"

#include "SocketClientToDriver.h"

#include <android-base/logging.h>

#include "AgentRequestHandler.h"
#include "test/vts/proto/VtsDriverControlMessage.pb.h"

#define LOCALHOST_IP "127.0.0.1"

namespace android {
namespace vts {

bool VtsDriverSocketClient::Exit() {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(EXIT);
  if (!VtsSocketSendMessage(command_message)) return false;

  VtsDriverControlResponseMessage response_message;
  if (!VtsSocketRecvMessage(&response_message)) return false;
  return true;
}

int32_t VtsDriverSocketClient::LoadHal(const string& file_path,
                                       int target_class, int target_type,
                                       float target_version,
                                       const string& target_package,
                                       const string& target_component_name,
                                       const string& hw_binder_service_name,
                                       const string& module_name) {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(LOAD_HAL);
  command_message.set_file_path(file_path);
  command_message.set_target_class(target_class);
  command_message.set_target_type(target_type);
  command_message.set_target_version(target_version);
  command_message.set_target_package(target_package);
  command_message.set_target_component_name(target_component_name);
  command_message.set_module_name(module_name);
  command_message.set_hw_binder_service_name(hw_binder_service_name);
  if (!VtsSocketSendMessage(command_message)) return -1;

  VtsDriverControlResponseMessage response_message;
  if (!VtsSocketRecvMessage(&response_message)) return -1;
  if (response_message.response_code() != VTS_DRIVER_RESPONSE_SUCCESS) {
    LOG(ERROR) << "Failed to load the selected HAL.";
    return -1;
  }
  LOG(INFO) << "Loaded the selected HAL.";
  return response_message.return_value();
}

string VtsDriverSocketClient::GetFunctions() {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(LIST_FUNCTIONS);
  if (!VtsSocketSendMessage(command_message)) {
    return {};
  }

  VtsDriverControlResponseMessage response_message;
  if (!VtsSocketRecvMessage(&response_message)) {
    return {};
  }

  return response_message.return_message();
}

string VtsDriverSocketClient::ReadSpecification(const string& component_name,
                                                int target_class,
                                                int target_type,
                                                float target_version,
                                                const string& target_package) {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(
      VTS_DRIVER_COMMAND_READ_SPECIFICATION);
  command_message.set_module_name(component_name);
  command_message.set_target_class(target_class);
  command_message.set_target_type(target_type);
  command_message.set_target_version(target_version);
  command_message.set_target_package(target_package);

  if (!VtsSocketSendMessage(command_message)) {
    return {};
  }

  VtsDriverControlResponseMessage response_message;
  if (!VtsSocketRecvMessage(&response_message)) {
    return {};
  }

  return response_message.return_message();
}

string VtsDriverSocketClient::Call(const string& arg, const string& uid) {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(CALL_FUNCTION);
  command_message.set_arg(arg);
  command_message.set_driver_caller_uid(uid);
  if (!VtsSocketSendMessage(command_message)) {
    return {};
  }

  VtsDriverControlResponseMessage response_message;
  if (!VtsSocketRecvMessage(&response_message)) {
    return {};
  }

  LOG(DEBUG) << "Result: " << response_message.return_message();
  return response_message.return_message();
}

string VtsDriverSocketClient::GetAttribute(const string& arg) {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(GET_ATTRIBUTE);
  command_message.set_arg(arg);
  if (!VtsSocketSendMessage(command_message)) {
    return {};
  }

  VtsDriverControlResponseMessage response_message;
  if (!VtsSocketRecvMessage(&response_message)) {
    return {};
  }

  return response_message.return_message();
}

unique_ptr<VtsDriverControlResponseMessage>
VtsDriverSocketClient::ExecuteShellCommand(
    const ::google::protobuf::RepeatedPtrField<string> shell_command) {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(EXECUTE_COMMAND);
  for (const auto& cmd : shell_command) {
    command_message.add_shell_command(cmd);
  }
  if (!VtsSocketSendMessage(command_message)) return nullptr;

  auto response_message = make_unique<VtsDriverControlResponseMessage>();
  if (!VtsSocketRecvMessage(response_message.get())) return nullptr;

  return response_message;
}

int32_t VtsDriverSocketClient::Status(int32_t type) {
  VtsDriverControlCommandMessage command_message;
  command_message.set_command_type(CALL_FUNCTION);
  command_message.set_status_type(type);
  if (!VtsSocketSendMessage(command_message)) return 0;

  VtsDriverControlResponseMessage response_message;
  if (!VtsSocketRecvMessage(&response_message)) return 0;
  return response_message.return_value();
}

string GetSocketPortFilePath(const string& service_name) {
  string result("/data/local/tmp/");
  result += service_name;
  return result;
}

bool IsDriverRunning(const string& service_name, int retry_count) {
  for (int retry = 0; retry < retry_count; retry++) {
    VtsDriverSocketClient* client = GetDriverSocketClient(service_name);
    if (client) {
      client->Exit();
      delete client;
      return true;
    }
    sleep(1);
  }
  LOG(ERROR) << "Couldn't connect to " << service_name;
  return false;
}

VtsDriverSocketClient* GetDriverSocketClient(const string& service_name) {
  string socket_port_file_path = GetSocketPortFilePath(service_name);
  VtsDriverSocketClient* client = new VtsDriverSocketClient();
  if (!client->Connect(socket_port_file_path)) {
    LOG(ERROR) << "Can't connect to " << socket_port_file_path;
    delete client;
    return NULL;
  }
  return client;
}

}  // namespace vts
}  // namespace android
