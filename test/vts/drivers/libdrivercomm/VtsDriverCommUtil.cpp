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

#define LOG_TAG "VtsDriverCommUtil"

#include "VtsDriverCommUtil.h"

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sstream>

#include <android-base/logging.h>

#include "test/vts/proto/VtsDriverControlMessage.pb.h"

using namespace std;

#define MAX_HEADER_BUFFER_SIZE 128

namespace android {
namespace vts {

bool VtsDriverCommUtil::Connect(const string& socket_name) {
  struct sockaddr_un serv_addr;
  struct hostent* server;

  LOG(DEBUG) << "Connect socket: " << socket_name;
  sockfd_ = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sockfd_ < 0) {
    LOG(ERROR) << "ERROR opening socket.";
    return false;
  }

  server = gethostbyname("127.0.0.1");
  if (server == NULL) {
    LOG(ERROR) << "ERROR can't resolve the host name, 127.0.0.1";
    return false;
  }

  bzero((char*)&serv_addr, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, socket_name.c_str());

  if (connect(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    LOG(ERROR) << "ERROR connecting to " << socket_name << " errno = " << errno
               << " " << strerror(errno);
    sockfd_ = -1;
    return false;
  }
  return true;
}

int VtsDriverCommUtil::Close() {
  int result = 0;
  if (sockfd_ != -1) {
    result = close(sockfd_);
    if (result != 0) {
      LOG(ERROR) << "ERROR closing socket (errno = " << errno << ")";
    }

    sockfd_ = -1;
  }

  return result;
}

bool VtsDriverCommUtil::VtsSocketSendBytes(const string& message) {
  if (sockfd_ == -1) {
    LOG(ERROR) << "ERROR sockfd not set.";
    return false;
  }
  stringstream header;
  header << message.length() << "\n";
  LOG(DEBUG) << "[agent->driver] len = " << message.length();
  int n = write(sockfd_, header.str().c_str(), header.str().length());
  if (n < 0) {
    LOG(ERROR) << " ERROR writing to socket.";
    return false;
  }

  int bytes_sent = 0;
  int msg_len = message.length();
  while (bytes_sent < msg_len) {
    n = write(sockfd_, &message.c_str()[bytes_sent], msg_len - bytes_sent);
    if (n <= 0) {
      LOG(ERROR) << "ERROR writing to socket.";
      return false;
    }
    bytes_sent += n;
  }
  return true;
}

string VtsDriverCommUtil::VtsSocketRecvBytes() {
  if (sockfd_ == -1) {
    LOG(ERROR) << "ERROR sockfd not set.";
    return string();
  }

  int header_index = 0;
  char header_buffer[MAX_HEADER_BUFFER_SIZE];

  for (header_index = 0; header_index < MAX_HEADER_BUFFER_SIZE;
       header_index++) {
    int ret = read(sockfd_, &header_buffer[header_index], 1);
    if (ret != 1) {
      int errno_save = errno;
      LOG(DEBUG) << "ERROR reading the length ret = " << ret
                 << " sockfd = " << sockfd_ << " "
                 << " errno = " << errno_save << " " << strerror(errno_save);
      return string();
    }
    if (header_buffer[header_index] == '\n' ||
        header_buffer[header_index] == '\r') {
      header_buffer[header_index] = '\0';
      break;
    }
  }

  int msg_len = atoi(header_buffer);
  char* msg = (char*)malloc(msg_len + 1);
  if (!msg) {
    LOG(ERROR) << "ERROR malloc failed.";
    return string();
  }

  int bytes_read = 0;
  while (bytes_read < msg_len) {
    int result = read(sockfd_, &msg[bytes_read], msg_len - bytes_read);
    if (result <= 0) {
      LOG(ERROR) << "ERROR read failed.";
      return string();
    }
    bytes_read += result;
  }
  msg[msg_len] = '\0';
  return string(msg, msg_len);
}

bool VtsDriverCommUtil::VtsSocketSendMessage(
    const google::protobuf::Message& message) {
  if (sockfd_ == -1) {
    LOG(ERROR) << "ERROR sockfd not set.";
    return false;
  }

  string message_string;
  if (!message.SerializeToString(&message_string)) {
    LOG(ERROR) << "ERROR can't serialize the message to a string.";
    return false;
  }
  return VtsSocketSendBytes(message_string);
}

bool VtsDriverCommUtil::VtsSocketRecvMessage(
    google::protobuf::Message* message) {
  if (sockfd_ == -1) {
    LOG(ERROR) << "ERROR sockfd not set.";
    return false;
  }

  string message_string = VtsSocketRecvBytes();
  if (message_string.length() == 0) {
    LOG(DEBUG) << "ERROR message string zero length.";
    return false;
  }

  return message->ParseFromString(message_string);
}

}  // namespace vts
}  // namespace android
