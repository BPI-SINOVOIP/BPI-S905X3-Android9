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

#ifndef __VTS_DRIVER_COMM_UTIL_H_
#define __VTS_DRIVER_COMM_UTIL_H_

#include <string>

#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class VtsDriverCommUtil {
 public:
  VtsDriverCommUtil() : sockfd_(-1) {}

  explicit VtsDriverCommUtil(int sockfd) : sockfd_(sockfd) {}

  ~VtsDriverCommUtil() {
    //    if (sockfd_ != -1) Close();
  }

  // returns true if connection to the server is successful, false otherwise.
  bool Connect(const string& socket_name);

  // sets sockfd_
  void SetSockfd(int sockfd) {
    sockfd_ = sockfd;
  }

  // closes the channel. returns 0 if success or socket already closed
  int Close();

  // Sends a message using the VTS's protocol for socket communication.
  bool VtsSocketSendBytes(const string& message);

  // Receives a message using the VTS's protocol for socket communication.
  string VtsSocketRecvBytes();

  // Sends a protobuf message.
  bool VtsSocketSendMessage(const google::protobuf::Message& message);

  // Receives a protobuf message.
  bool VtsSocketRecvMessage(google::protobuf::Message* message);

 private:
  // sockfd
  int sockfd_;
};

}  // namespace vts
}  // namespace android

#endif  // __VTS_DRIVER_COMM_UTIL_H_
