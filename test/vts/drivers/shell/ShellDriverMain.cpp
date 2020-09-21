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

#include <getopt.h>
#include <string>

#include <android-base/logging.h>

#include "ShellDriver.h"

using namespace std;

static constexpr const char* DEFAULT_SOCKET_PATH =
    "/data/local/tmp/tmp_socket_shell_driver.tmp";

// Dumps usage on stderr.
static void usage() {
  fprintf(
      stderr,
      "Usage: vts_shell_driver --server_socket_path=<UNIX_domain_socket_path>\n"
      "\n"
      "Android Vts Shell Driver v0.1.  To run shell commands on Android system."
      "\n"
      "Options:\n"
      "--help\n"
      "    Show this message.\n"
      "--socket_path=<Unix_socket_path>\n"
      "    Show this message.\n"
      "\n"
      "Recording continues until Ctrl-C is hit or the time limit is reached.\n"
      "\n");
}

// Parses command args and kicks things off.
int main(int argc, char** argv) {
  android::base::InitLogging(argv, android::base::StderrLogger);

  static const struct option longOptions[] = {
      {"help", no_argument, NULL, 'h'},
      {"server_socket_path", required_argument, NULL, 's'},
      {NULL, 0, NULL, 0}};

  string socket_path = DEFAULT_SOCKET_PATH;

  while (true) {
    int optionIndex = 0;
    int ic = getopt_long(argc, argv, "", longOptions, &optionIndex);
    if (ic == -1) {
      break;
    }

    switch (ic) {
      case 'h':
        usage();
        return 0;
      case 's':
        socket_path = string(optarg);
        break;
      default:
        if (ic != '?') {
          fprintf(stderr, "getopt_long returned unexpected value 0x%x\n", ic);
        }
        return 2;
    }
  }

  android::vts::VtsShellDriver shellDriver(socket_path.c_str());
  return shellDriver.StartListen();
}
