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

#define LOG_TAG "VtsDriverHalSocketServer"

#include "SocketServer.h"

#include <netinet/in.h>
#include <sys/un.h>
#include <string>

#include <VtsDriverCommUtil.h>
#include <android-base/logging.h>
#include <google/protobuf/text_format.h>

#include "test/vts/proto/ComponentSpecificationMessage.pb.h"
#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

void VtsDriverHalSocketServer::Exit() {
  LOG(INFO) << "VtsHalDriverServer::Exit";
}

int32_t VtsDriverHalSocketServer::LoadHal(const string& path, int target_class,
                                          int target_type, float target_version,
                                          const string& target_package,
                                          const string& target_component_name,
                                          const string& hw_binder_service_name,
                                          const string& /*module_name*/) {
  LOG(DEBUG) << "LoadHal(" << path << ")";
  int32_t driver_id = driver_manager_->LoadTargetComponent(
      path.c_str(), lib_path_, target_class, target_type, target_version,
      target_package.c_str(), target_component_name.c_str(),
      hw_binder_service_name.c_str());
  LOG(DEBUG) << "Result: " << driver_id;
  return driver_id;
}


string VtsDriverHalSocketServer::ReadSpecification(
    const string& name, int target_class, int target_type, float target_version,
    const string& target_package) {
  ComponentSpecificationMessage msg;
  driver_manager_->FindComponentSpecification(
      target_class, target_type, target_version, target_package, name, &msg);
  string result;
  google::protobuf::TextFormat::PrintToString(msg, &result);
  LOG(DEBUG) << "Result: " << result;
  return result;
}

string VtsDriverHalSocketServer::Call(const string& arg) {
  FunctionCallMessage* call_msg = new FunctionCallMessage();
  google::protobuf::TextFormat::MergeFromString(arg, call_msg);
  const string& result = driver_manager_->CallFunction(call_msg);
  LOG(DEBUG) << "Result: " << result;
  return result;
}

string VtsDriverHalSocketServer::GetAttribute(const string& arg) {
  FunctionCallMessage* call_msg = new FunctionCallMessage();
  google::protobuf::TextFormat::MergeFromString(arg, call_msg);
  const string& result = driver_manager_->GetAttribute(call_msg);
  LOG(DEBUG) << "Result: " << result;
  return result;
}

string VtsDriverHalSocketServer::ListFunctions() const {
  vts::ComponentSpecificationMessage* spec =
      driver_manager_->GetComponentSpecification();
  string output;
  if (!spec) {
    return output;
  }
  if (google::protobuf::TextFormat::PrintToString(*spec, &output)) {
    LOG(DEBUG) << "Result: " << output;
    return output;
  } else {
    LOG(ERROR) << "Can't serialize the interface spec message to a string.";
    return output;
  }
}

bool VtsDriverHalSocketServer::ProcessOneCommand() {
  VtsDriverControlCommandMessage command_message;
  if (!VtsSocketRecvMessage(&command_message)) return false;
  // TODO: get the command type description.
  LOG(DEBUG) << " command_type " << command_message.command_type();
  switch (command_message.command_type()) {
    case EXIT: {
      Exit();
      VtsDriverControlResponseMessage response_message;
      response_message.set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      if (VtsSocketSendMessage(response_message)) {
        LOG(INFO) << "process: " << getpid() << " exiting";
        return false;
      }
      break;
    }
    case LOAD_HAL: {
      LOG(INFO) << "Process command LOAD_HAL";
      int32_t driver_id = LoadHal(
          command_message.file_path(), command_message.target_class(),
          command_message.target_type(), command_message.target_version(),
          command_message.target_package(),
          command_message.target_component_name(),
          command_message.hw_binder_service_name(),
          command_message.module_name());
      VtsDriverControlResponseMessage response_message;
      if (driver_id == -1) {
        response_message.set_response_code(VTS_DRIVER_RESPONSE_FAIL);
      } else {
        response_message.set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      }
      response_message.set_return_value(driver_id);
      if (VtsSocketSendMessage(response_message)) return true;
      break;
    }
    case GET_STATUS: {
      int32_t result = command_message.status_type();
      VtsDriverControlResponseMessage response_message;
      response_message.set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message.set_return_value(result);
      if (VtsSocketSendMessage(response_message)) return true;
      break;
    }
    case CALL_FUNCTION: {
      LOG(INFO) << "Process command CALL_FUNCTION";
      if (command_message.has_driver_caller_uid()) {
        setuid(atoi(command_message.driver_caller_uid().c_str()));
      }
      const string& result = Call(command_message.arg());
      VtsDriverControlResponseMessage response_message;
      response_message.set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message.set_return_message(result);
      if (VtsSocketSendMessage(response_message)) return true;
      break;
    }
    case VTS_DRIVER_COMMAND_READ_SPECIFICATION: {
      LOG(INFO) << "Process command READ_SPECIFICATION";
      const string& result = ReadSpecification(
          command_message.module_name(), command_message.target_class(),
          command_message.target_type(), command_message.target_version(),
          command_message.target_package());
      VtsDriverControlResponseMessage response_message;
      response_message.set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message.set_return_message(result);
      if (VtsSocketSendMessage(response_message)) return true;
      break;
    }
    case GET_ATTRIBUTE: {
      LOG(INFO) << "Process command GET_ATTRIBUTE";
      const string& result = GetAttribute(command_message.arg());
      VtsDriverControlResponseMessage response_message;
      response_message.set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
      response_message.set_return_message(result);
      if (VtsSocketSendMessage(response_message)) return true;
      break;
    }
    case LIST_FUNCTIONS: {
      LOG(INFO) << "Process command LIST_FUNCTIONS";
      string result = ListFunctions();
      VtsDriverControlResponseMessage response_message;
      if (result.size() > 0) {
        response_message.set_response_code(VTS_DRIVER_RESPONSE_SUCCESS);
        response_message.set_return_message(result.c_str());
      } else {
        response_message.set_response_code(VTS_DRIVER_RESPONSE_FAIL);
      }
      if (VtsSocketSendMessage(response_message)) return true;
      break;
    }
    default:
      break;
  }
  LOG(ERROR) << "Failed.";
  return false;
}

// Starts to run a UNIX socket server (foreground).
int StartSocketServer(const string& socket_port_file,
                      VtsHalDriverManager* driver_manager,
                      const char* lib_path) {
  int sockfd;
  socklen_t clilen;
  struct sockaddr_in cli_addr;
  struct sockaddr_un serv_addr;

  sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) {
    LOG(ERROR) << "Can't open the socket.";
    return -1;
  }

  unlink(socket_port_file.c_str());
  bzero((char*)&serv_addr, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, socket_port_file.c_str());

  LOG(DEBUG) << "Trying to bind (port file: " << socket_port_file << ")";

  if (::bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    int error_save = errno;
    LOG(ERROR) << "ERROR binding failed. errno = " << error_save << " "
               << strerror(error_save);
    return -1;
  }

  listen(sockfd, 5);
  clilen = sizeof(cli_addr);

  while (true) {
    LOG(DEBUG) << "Waiting for a new connection from the agent";
    int newsockfd = ::accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    if (newsockfd < 0) {
      LOG(ERROR) << "ERROR accept failed.";
      return -1;
    }

    LOG(DEBUG) << "New session";
    pid_t pid = fork();
    if (pid == 0) {  // child
      close(sockfd);
      LOG(DEBUG) << "Process for an agent - pid = " << getpid();
      VtsDriverHalSocketServer* server =
          new VtsDriverHalSocketServer(driver_manager, lib_path);
      server->SetSockfd(newsockfd);
      while (server->ProcessOneCommand())
        ;
      delete server;
      exit(0);
    } else if (pid < 0) {
      LOG(ERROR) << "Can't fork a child process to handle a session.";
      return -1;
    }
    close(newsockfd);
  }
  LOG(ERROR) << "Exiting";
  return 0;
}

}  // namespace vts
}  // namespace android

#endif
