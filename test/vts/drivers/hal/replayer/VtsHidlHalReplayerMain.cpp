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
 * Example usage
 *  $ vts_hal_replayer /data/local/tmp/hal-trace/nfc/V1_0/nfc.vts.trace
 *  $ vts_hal_replayer --spec_dir_path /data/local/tmp/spec
 *    --hal_service_name default
 *    /data/local/tmp/hal-trace/nfc/V1_0/nfc.vts.trace
 */

#include <getopt.h>
#include <iostream>
#include <string>

#include <android-base/logging.h>

#include "VtsHidlHalReplayer.h"
#include "driver_manager/VtsHalDriverManager.h"

using namespace std;

static constexpr const char* kDefaultSpecDirPath = "/data/local/tmp/spec/";
static constexpr const char* kPassedMarker = "[  PASSED  ]";
static const int kDefaultEpochCount = 100;

static void AddHalServiceInstance(const string& instance,
                                  map<string, string>* halServiceInstances) {
  if (halServiceInstances == nullptr) {
    cerr << __func__ << ": halServiceInstances should not be null " << endl;
    return;
  }
  // hal_service_instance follows the format:
  // package@version::interface/service_name e.g.:
  // android.hardware.vibrator@1.0::IVibrator/default
  string instanceName = instance.substr(0, instance.find('/'));
  string serviceName = instance.substr(instance.find('/') + 1);
  // Fail the process if trying to pass multiple service names for the same
  // service instance.
  if (halServiceInstances->find(instanceName) != halServiceInstances->end()) {
    cerr << "Exisitng instance " << instanceName << " with name "
         << (*halServiceInstances)[instanceName] << endl;
  } else {
    (*halServiceInstances)[instanceName] = serviceName;
  }
}

void ShowUsage() {
  cout << "Usage: vts_hal_replayer [options] <trace file>\n"
          "--spec_dir_path <path>:     Set path that store the vts spec files\n"
          "--hal_service_instances <name>:  Set the hal service name\n"
          "--help:                     Show help\n";
  exit(1);
}

int main(int argc, char** argv) {
  android::base::InitLogging(argv, android::base::StderrLogger);

  const char* const short_opts = "hld:n:";
  const option long_opts[] = {
      {"help", no_argument, nullptr, 'h'},
      {"list_service", no_argument, nullptr, 'l'},
      {"spec_dir_path", optional_argument, nullptr, 'd'},
      {"hal_service_instances", optional_argument, nullptr, 'n'},
      {nullptr, 0, nullptr, 0}};

  string spec_dir_path = kDefaultSpecDirPath;
  map<string, string> hal_service_instances;
  bool list_service = false;

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
      case 'n': {
        AddHalServiceInstance(string(optarg), &hal_service_instances);
        break;
      }
      case 'l': {
        list_service = true;
        break;
      }
      default:
        cerr << "getopt_long returned unexpected value " << opt << endl;
        return 2;
    }
  }

  if (optind != argc - 1) {
    cerr << "Must specify the trace file (see --help).\n" << endl;
    return 2;
  }

  string trace_path = argv[optind];

  android::vts::VtsHalDriverManager driver_manager(spec_dir_path,
                                                   kDefaultEpochCount, "");
  android::vts::VtsHidlHalReplayer replayer(&driver_manager);

  if (list_service) {
    replayer.ListTestServices(trace_path);
  } else {
    bool success = replayer.ReplayTrace(trace_path, hal_service_instances);
    if (success) {
      cout << endl << kPassedMarker << endl;
    }
  }
  return 0;
}
