/*
 * Copyright 2017 The Android Open Source Project
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

/*
 * Example usage:
 *  $ vts_hal_driver
 *  $ vts_hal_driver --spec_dir_path /data/local/tmp/spec/
 *    --callback_socket_name /data/local/tmp/vts_agent_callback
 *    --server_socket_path /data/local/tmp/vts_tcp_server_port
 */
#include <getopt.h>
#include <iostream>
#include <string>

#include <android-base/logging.h>

#include "BinderServer.h"
#include "SocketServer.h"
#include "binder/VtsFuzzerBinderService.h"
#include "driver_manager/VtsHalDriverManager.h"

using namespace std;

static constexpr const char* kDefaultSpecDirPath = "/data/local/tmp/spec/";
static constexpr const char* kInterfaceSpecLibName =
    "libvts_interfacespecification.so";
static const int kDefaultEpochCount = 100;

void ShowUsage() {
  cout << "Usage: vts_hal_driver [options] <interface spec lib>\n"
          "--spec_dir_path <path>:         Set path that store the vts spec "
          "files\n"
          "--callback_socket_name <name>:  Set the callback (agent) socket "
          "name\n"
          "--server_socket_path <path>:    Set the driver server socket path\n"
          "--service_name <name>:          Set the binder service name\n"
          "--help:                         Show help\n";
  exit(1);
}

int main(int argc, char** argv) {
  android::base::InitLogging(argv, android::base::StderrLogger);

  const char* const short_opts = "h:d:c:s:n";
  const option long_opts[] = {
      {"help", no_argument, nullptr, 'h'},
      {"spec_dir_path", optional_argument, nullptr, 'd'},
      {"callback_socket_name", optional_argument, nullptr, 'c'},
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
      {"server_socket_path", optional_argument, NULL, 's'},
#else  // binder
      {"service_name", required_argument, NULL, 'n'},
#endif
      {nullptr, 0, nullptr, 0},
  };

  string spec_dir_path = kDefaultSpecDirPath;
  string callback_socket_name = "";
  string server_socket_path;
  string service_name;

  while (true) {
    int opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    if (opt == -1) {
      break;
    }

    switch (opt) {
      case 'h':
      case '?':
        ShowUsage();
        return 0;
      case 'd': {
        spec_dir_path = string(optarg);
        break;
      }
      case 'c': {
        callback_socket_name = string(optarg);
        break;
      }
#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
      case 's':
        server_socket_path = string(optarg);
        break;
#else  // binder
      case 'n':
        service_name = string(optarg);
        break;
#endif
      default:
        cerr << "getopt_long returned unexpected value " << opt << endl;
        return 2;
    }
  }

  android::vts::VtsHalDriverManager driver_manager(
      spec_dir_path, kDefaultEpochCount, callback_socket_name);

#ifndef VTS_AGENT_DRIVER_COMM_BINDER  // socket
  android::vts::StartSocketServer(server_socket_path, &driver_manager,
                                  kInterfaceSpecLibName);
#else  // binder
  android::vts::StartBinderServer(service_name, &driver_manager,
                                  kInterfaceSpecLibName);
#endif

  return 0;
}
