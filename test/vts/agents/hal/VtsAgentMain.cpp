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
#include <map>

#include <android-base/logging.h>
#include <getopt.h>

#include <android-base/logging.h>
#include "TcpServerForRunner.h"

static constexpr const char* kDefaultSpecDirPath = "/data/local/tmp/spec/";
static constexpr const char* kDefaultHalDriverPath32 = "./vts_hal_driver32";
static constexpr const char* kDefaultHalDriverPath64 = "./vts_hal_driver64";
static constexpr const char* kDefaultShellDriverPath32 = "./vts_shell_driver32";
static constexpr const char* kDefaultShellDriverPath64 = "./vts_shell_driver64";

using namespace std;

void ShowUsage() {
  printf(
      "Usage: vts_hal_agent [options]\n"
      "--hal_driver_path_32:   Set 32 bit hal driver binary path.\n"
      "--hal_driver_path_32:   Set 64 bit hal driver binary path.\n"
      "--spec_dir:             Set vts spec directory. \n"
      "--shell_driver_path_32: Set 32 bit shell driver binary path\n"
      "--shell_driver_path_64: Set 64 bit shell driver binary path\n"
      "--log_severity:         Set log severity\n"
      "--help:                 Show help\n");
  exit(1);
}

int main(int argc, char** argv) {
  string hal_driver_path32 = kDefaultHalDriverPath32;
  string hal_driver_path64 = kDefaultHalDriverPath64;
  string shell_driver_path32 = kDefaultShellDriverPath32;
  string shell_driver_path64 = kDefaultShellDriverPath64;
  string spec_dir_path = kDefaultSpecDirPath;
  string log_severity = "INFO";

  enum {
    HAL_DRIVER_PATH32 = 1000,
    HAL_DRIVER_PATH64,
    SHELL_DRIVER_PATH32,
    SHELL_DRIVER_PATH64,
    SPEC_DIR
  };

  const char* const short_opts = "hl:";
  const option long_opts[] = {
      {"help", no_argument, nullptr, 'h'},
      {"hal_driver_path_32", required_argument, nullptr, HAL_DRIVER_PATH32},
      {"hal_driver_path_64", required_argument, nullptr, HAL_DRIVER_PATH64},
      {"shell_driver_path_32", required_argument, nullptr, SHELL_DRIVER_PATH32},
      {"shell_driver_path_64", required_argument, nullptr, SHELL_DRIVER_PATH64},
      {"spec_dir", required_argument, nullptr, SPEC_DIR},
      {"log_severity", required_argument, nullptr, 'l'},
      {nullptr, 0, nullptr, 0},
  };

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
      case 'l': {
        log_severity = string(optarg);
        break;
      }
      case HAL_DRIVER_PATH32: {
        hal_driver_path32 = string(optarg);
        break;
      }
      case HAL_DRIVER_PATH64: {
        hal_driver_path64 = string(optarg);
        break;
      }
      case SHELL_DRIVER_PATH32: {
        shell_driver_path32 = string(optarg);
        break;
      }
      case SHELL_DRIVER_PATH64: {
        shell_driver_path64 = string(optarg);
        break;
      }
      case SPEC_DIR: {
        spec_dir_path = string(optarg);
        break;
      }
      default:
        printf("getopt_long returned unexpected value: %d\n", opt);
        return 2;
    }
  }

  map<string, string> log_map = {
      {"ERROR", "*:e"}, {"WARNING", "*:w"}, {"INFO", "*:i"}, {"DEBUG", "*:d"},
  };

  if (log_map.find(log_severity) != log_map.end()) {
    setenv("ANDROID_LOG_TAGS", log_map[log_severity].c_str(), 1);
  }

  android::base::InitLogging(argv, android::base::StderrLogger);

  LOG(INFO) << "|| VTS AGENT ||";

  char* dir_path;
  dir_path = (char*)malloc(strlen(argv[0]) + 1);
  strcpy(dir_path, argv[0]);
  for (int index = strlen(argv[0]) - 2; index >= 0; index--) {
    if (dir_path[index] == '/') {
      dir_path[index] = '\0';
      break;
    }
  }
  chdir(dir_path);

  android::vts::StartTcpServerForRunner(
      spec_dir_path.c_str(), hal_driver_path32.c_str(),
      hal_driver_path64.c_str(), shell_driver_path32.c_str(),
      shell_driver_path64.c_str());
  return 0;
}
