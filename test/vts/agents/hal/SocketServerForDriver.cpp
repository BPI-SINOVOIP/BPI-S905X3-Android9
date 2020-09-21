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
#define LOG_TAG "VtsAgentSocketServer"

#include "SocketServerForDriver.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string>

#include <android-base/logging.h>

#include "test/vts/proto/AndroidSystemControlMessage.pb.h"

namespace android {
namespace vts {

static const int kCallbackServerPort = 5010;

void SocketServerForDriver::RpcCallToRunner(
    const AndroidSystemCallbackRequestMessage& message) {
  struct sockaddr_in serv_addr;
  struct hostent* server;

  int sockfd;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    LOG(ERROR) << "ERROR opening socket";
    exit(-1);
    return;
  }
  server = gethostbyname("127.0.0.1");
  if (server == NULL) {
    LOG(ERROR) << "Can't resolve the host name, localhost";
    exit(-1);
    return;
  }
  bzero((char*)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(runner_port_);

  if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    LOG(ERROR) << "ERROR connecting";
    exit(-1);
    return;
  }

  VtsDriverCommUtil util(sockfd);
  if (!util.VtsSocketSendMessage(message)) return;
}

void SocketServerForDriver::Start() {
  AndroidSystemCallbackRequestMessage message;
  if (!VtsSocketRecvMessage(&message)) return;
  LOG(INFO) << "Start server for driver with callback ID: " << message.id();
  RpcCallToRunner(message);
  Close();
}

int StartSocketServerForDriver(const string& callback_socket_name,
                               int runner_port) {
  struct sockaddr_un serv_addr;
  int pid = fork();
  if (pid < 0) {
    LOG(ERROR) << "ERROR on fork";
    return -1;
  } else if (pid > 0) {
    return 0;
  }

  if (runner_port == -1) {
    runner_port = kCallbackServerPort;
  }
  // only child process continues;
  int sockfd;
  sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) {
    LOG(ERROR) << "ERROR opening socket";
    return -1;
  }

  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, callback_socket_name.c_str());
  LOG(INFO) << "Callback server at " << callback_socket_name;

  if (::bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    int error_save = errno;
    LOG(ERROR) << "ERROR on binding " << callback_socket_name
               << " errno = " << error_save << " " << strerror(error_save);
    return -1;
  }

  if (listen(sockfd, 5) < 0) {
    LOG(ERROR) << "ERROR on listening";
    return -1;
  }

  while (1) {
    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen;

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
    if (newsockfd < 0) {
      LOG(ERROR) << "ERROR on accept " << strerror(errno);
      break;
    }
    LOG(DEBUG) << "New callback connection.";
    pid = fork();
    if (pid == 0) {
      close(sockfd);
      SocketServerForDriver server(newsockfd, runner_port);
      server.Start();
      exit(0);
    } else if (pid > 0) {
      close(newsockfd);
    } else {
      LOG(ERROR) << "ERROR on fork";
      break;
    }
  }
  close(sockfd);
  exit(0);
}

}  // namespace vts
}  // namespace android
