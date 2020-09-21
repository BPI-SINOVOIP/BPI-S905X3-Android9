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

#include "ShellDriverTest.h"

#include <errno.h>
#include <gtest/gtest.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include <VtsDriverCommUtil.h>
#include "test/vts/proto/VtsDriverControlMessage.pb.h"

#include "ShellDriver.h"

using namespace std;

namespace android {
namespace vts {

static int kMaxRetry = 3;

/*
 * send a command to the driver on specified UNIX domain socket and print out
 * the outputs from driver.
 */
static string vts_shell_driver_test_client_start(const string& command,
                                                 const string& socket_address) {
  struct sockaddr_un address;
  int socket_fd;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    fprintf(stderr, "socket() failed\n");
    return "";
  }

  VtsDriverCommUtil driverUtil(socket_fd);

  memset(&address, 0, sizeof(struct sockaddr_un));

  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, socket_address.c_str(),
          sizeof(address.sun_path) - 1);

  int conn_success;
  int retry_count = 0;

  conn_success = connect(socket_fd, (struct sockaddr*)&address,
                         sizeof(struct sockaddr_un));
  for (retry_count = 0; retry_count < kMaxRetry && conn_success != 0;
       retry_count++) {  // retry if server not ready
    printf("Client: connection failed, retrying...\n");
    retry_count++;
    if (usleep(50 * pow(retry_count, 3)) != 0) {
      fprintf(stderr, "shell driver unit test: sleep intrupted.");
    }

    conn_success = connect(socket_fd, (struct sockaddr*)&address,
                           sizeof(struct sockaddr_un));
  }

  if (conn_success != 0) {
    fprintf(stderr, "connect() failed\n");
    return "";
  }

  VtsDriverControlCommandMessage cmd_msg;

  cmd_msg.add_shell_command(command);
  cmd_msg.set_command_type(EXECUTE_COMMAND);

  if (!driverUtil.VtsSocketSendMessage(cmd_msg)) {
    return NULL;
  }

  // read driver output
  VtsDriverControlResponseMessage out_msg;

  if (!driverUtil.VtsSocketRecvMessage(
          static_cast<google::protobuf::Message*>(&out_msg))) {
    return "";
  }

  // TODO(yuexima) use vector for output messages
  stringstream ss;
  for (int i = 0; i < out_msg.stdout_size(); i++) {
    string out_str = out_msg.stdout(i);
    cout << "[Shell driver] output for command " << i << ": " << out_str
         << endl;
    ss << out_str;
  }
  close(socket_fd);

  cout << "[Client] receiving output: " << ss.str() << endl;
  return ss.str();
}

/*
 * Prototype unit test helper. It first forks a vts_shell_driver process
 * and then call a client function to execute a command.
 */
static string test_shell_command_output(const string& command,
                                        const string& socket_address) {
  pid_t p_driver;
  string res_client;

  VtsShellDriver shellDriver(socket_address.c_str());

  p_driver = fork();
  if (p_driver == 0) {  // child
    int res_driver = shellDriver.StartListen();

    if (res_driver != 0) {
      fprintf(stderr, "Driver reported error. The error code is: %d.\n",
              res_driver);
      exit(res_driver);
    }

    exit(0);
  } else if (p_driver > 0) {  // parent
    res_client = vts_shell_driver_test_client_start(command, socket_address);
    if (res_client.empty()) {
      fprintf(stderr, "Client reported error.\n");
      exit(1);
    }
    cout << "Client receiving: " << res_client << endl;
  } else {
    fprintf(stderr,
            "shell_driver_test.cpp: create child process failed for driver.");
    exit(-1);
  }

  // send kill signal to insure the process would not block
  kill(p_driver, SIGKILL);

  return res_client;
}

/*
 * This test tests whether the output of "uname" is "Linux\n"
 */
TEST(vts_shell_driver_start, vts_shell_driver_unit_test_uname) {
  string expected = "Linux\n";
  string output =
      test_shell_command_output("uname", "/data/local/tmp/test1_1.tmp");
  ASSERT_EQ(output.compare(expected), 0);
}

/*
 * This test tests whether the output of "which ls" is "/system/bin/ls\n"
 */
TEST(vts_shell_driver_start, vts_shell_driver_unit_test_which_ls) {
  string expected = "/system/bin/ls\n";
  string output =
      test_shell_command_output("which ls", "/data/local/tmp/test1_2.tmp");
  ASSERT_EQ(output.compare(expected), 0);
}

}  // namespace vts
}  // namespace android
