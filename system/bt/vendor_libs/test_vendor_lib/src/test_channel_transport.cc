/*
 * Copyright 2015 The Android Open Source Project
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

#define LOG_TAG "test_channel_transport"

#include "test_channel_transport.h"

#include <base/logging.h>

#include "osi/include/log.h"
#include "osi/include/osi.h"

#include <netinet/in.h>
#include <sys/socket.h>

using std::vector;

namespace test_vendor_lib {

int TestChannelTransport::SetUp(int port) {
  struct sockaddr_in listen_address;
  socklen_t sockaddr_in_size = sizeof(struct sockaddr_in);
  memset(&listen_address, 0, sockaddr_in_size);

  OSI_NO_INTR(listen_fd_ = socket(AF_INET, SOCK_STREAM, 0));
  if (listen_fd_ < 0) {
    LOG_INFO(LOG_TAG, "Error creating socket for test channel.");
    return -1;
  }

  LOG_INFO(LOG_TAG, "port: %d", port);
  listen_address.sin_family = AF_INET;
  listen_address.sin_port = htons(port);
  listen_address.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&listen_address),
           sockaddr_in_size) < 0) {
    LOG_INFO(LOG_TAG, "Error binding test channel listener socket to address.");
    close(listen_fd_);
    return -1;
  }

  if (listen(listen_fd_, 1) < 0) {
    LOG_INFO(LOG_TAG, "Error listening for test channel.");
    close(listen_fd_);
    return -1;
  }
  return listen_fd_;
}

void TestChannelTransport::CleanUp() {
  if (listen_fd_ == -1) {
    return;
  }
  if (close(listen_fd_)) {
    LOG_ERROR(LOG_TAG, "Error closing listen_fd_.");
  }
  listen_fd_ = -1;
}

int TestChannelTransport::Accept(int listen_fd_) {
  int accept_fd = -1;
  struct sockaddr_in test_channel_address;
  socklen_t sockaddr_in_size = sizeof(struct sockaddr_in);
  memset(&test_channel_address, 0, sockaddr_in_size);

  OSI_NO_INTR(accept_fd =
                  accept(listen_fd_,
                         reinterpret_cast<sockaddr*>(&test_channel_address),
                         &sockaddr_in_size));
  if (accept_fd < 0) {
    LOG_INFO(LOG_TAG, "Error accepting test channel connection errno=%d (%s).",
             errno, strerror(errno));

    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      LOG_ERROR(LOG_TAG, "Closing listen_fd_ (won't try again).");
      close(listen_fd_);
      return -1;
    }
  }

  LOG_INFO(LOG_TAG, "accept_fd = %d.", accept_fd);

  return accept_fd;
}

void TestChannelTransport::OnCommandReady(int fd,
                                          std::function<void(void)> unwatch) {
  uint8_t command_name_size = 0;
  read(fd, &command_name_size, 1);
  vector<uint8_t> command_name_raw;
  command_name_raw.resize(command_name_size);
  read(fd, &command_name_raw[0], command_name_size);
  std::string command_name(command_name_raw.begin(), command_name_raw.end());
  LOG_INFO(LOG_TAG, "Received command from test channel: %s",
           command_name.data());

  if (command_name == "CLOSE_TEST_CHANNEL" || command_name == "") {
    LOG_INFO(LOG_TAG, "Test channel closed");
    unwatch();
    close(fd);
    return;
  }

  uint8_t num_args = 0;
  read(fd, &num_args, 1);
  LOG_INFO(LOG_TAG, "num_args: %d", num_args);
  vector<std::string> args;
  for (uint8_t i = 0; i < num_args; ++i) {
    uint8_t arg_size = 0;
    read(fd, &arg_size, 1);
    vector<uint8_t> arg;
    arg.resize(arg_size);
    read(fd, &arg[0], arg_size);
    args.push_back(std::string(arg.begin(), arg.end()));
  }

  for (size_t i = 0; i < args.size(); ++i)
    LOG_INFO(LOG_TAG, "Command argument %zu: %s", i, args[i].data());

  command_handler_(command_name, args);
}

void TestChannelTransport::RegisterCommandHandler(
    const std::function<void(const std::string&, const vector<std::string>&)>&
        callback) {
  command_handler_ = callback;
}

}  // namespace test_vendor_lib
