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
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <cutils/properties.h>
#include <hidl/ServiceManagement.h>
#include <iostream>

using namespace std;
using namespace android;

const string kSysPropHalCoverage = "hal.coverage.enable";

// Print usage directions.
void usage() {
  cout << "usage:\n";
  cout << "vts_coverage_configure flush\t\t\t\t: to flush coverage on all "
          "HALs\n";
  cout << "vts_coverage_configure flush <hal name>@<hal version>\t: to flush "
          "coverage on one HAL name/version instance"
       << std::endl;
}

// Parse the fully-qualified instance name and call the func with the interface
// name, instance name, and HAL name.
template <typename Lambda>
bool parseFqInstaceName(string fqInstanceName, Lambda &&func) {
  string::size_type n = fqInstanceName.find("/");
  if (n == std::string::npos || fqInstanceName.size() == n + 1) return false;

  string fqInterfaceName = fqInstanceName.substr(0, n);
  string instanceName = fqInstanceName.substr(n + 1, std::string::npos);

  n = fqInstanceName.find("::");
  if (n == std::string::npos || fqInstanceName.size() == n + 1) return false;
  string halName = fqInstanceName.substr(0, n);
  std::forward<Lambda>(func)(fqInterfaceName, instanceName, halName);
  return true;
}

// Flush coverage on all HAL processes, or just the provided HAL name if
// provided.
bool FlushHALCoverage(string flushHal = "") {
  using ::android::hidl::base::V1_0::IBase;
  using ::android::hidl::manager::V1_0::IServiceManager;
  using ::android::hardware::hidl_string;
  using ::android::hardware::Return;

  sp<IServiceManager> sm = ::android::hardware::defaultServiceManager();

  if (sm == nullptr) {
    cerr << "failed to get IServiceManager to poke HAL services." << std::endl;
    return false;
  }
  property_set(kSysPropHalCoverage.c_str(), "true");
  auto listRet = sm->list([&](const auto &interfaces) {
    for (const string &fqInstanceName : interfaces) {
      hidl_string fqInterfaceName;
      hidl_string instanceName;
      string halName;

      auto cb = [&](string fqIface, string instance, string hal) {
        fqInterfaceName = fqIface;
        instanceName = instance;
        halName = hal;
      };
      if (!parseFqInstaceName(fqInstanceName, cb)) continue;
      if (halName.find("android.hidl") == 0) continue;
      if (flushHal == "" || !flushHal.compare(halName)) {
        Return<sp<IBase>> interfaceRet = sm->get(fqInterfaceName, instanceName);
        if (!interfaceRet.isOk()) {
          cerr << "failed to get service " << fqInstanceName << ": "
               << interfaceRet.description() << std::endl;
          continue;
        }
        sp<IBase> interface = interfaceRet;
        auto notifyRet = interface->notifySyspropsChanged();
        if (!notifyRet.isOk()) {
          cerr << "failed to notifySyspropsChanged on service "
               << fqInstanceName << ": " << notifyRet.description()
               << std::endl;
        }
        cout << "- flushed the coverage for HAL " << fqInstanceName
             << std::endl;
      }
    }
  });
  property_set(kSysPropHalCoverage.c_str(), "false");
  if (!listRet.isOk()) {
    cerr << "failed to list services: " << listRet.description() << std::endl;
    return false;
  }
  return true;
}

// The provided binary can be used to flush coverage on one or all HALs.
// Usage examples:
//   To flush gcov and/or sancov coverage data on all hals: <binary> flush
//   To flush gcov and/or sancov coverage data on one hal: <binary> flush <hal
//   name>@<hal version>
int main(int argc, char *argv[]) {
  bool flush_coverage = false;
  if (argc < 2) {
    usage();
    return -1;
  }
  if (!strcmp(argv[1], "flush")) {
    flush_coverage = true;
    string halString = "";
    if (argc == 3) {
      halString = string(argv[2]);
    }
    cout << "* flush coverage" << std::endl;
    if (!FlushHALCoverage(halString)) {
      cerr << "failed to flush coverage" << std::endl;
    }
  } else {
    usage();
  }
  return 0;
}
