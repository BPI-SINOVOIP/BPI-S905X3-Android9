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
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <cutils/properties.h>
#include <hidl/ServiceManagement.h>
#include <iostream>

using namespace std;
using namespace android;

// Turns on the HAL instrumentation feature on all registered HIDL HALs.
bool SetHALInstrumentation() {
  using ::android::hidl::base::V1_0::IBase;
  using ::android::hidl::manager::V1_0::IServiceManager;
  using ::android::hardware::hidl_string;
  using ::android::hardware::Return;

  sp<IServiceManager> sm = ::android::hardware::defaultServiceManager();

  if (sm == nullptr) {
    fprintf(stderr, "failed to get IServiceManager to poke HAL services.\n");
    return false;
  }

  auto listRet = sm->list([&](const auto &interfaces) {
    for (const string &fqInstanceName : interfaces) {
      string::size_type n = fqInstanceName.find("/");
      if (n == std::string::npos || fqInstanceName.size() == n + 1) continue;

      hidl_string fqInterfaceName = fqInstanceName.substr(0, n);
      hidl_string instanceName =
          fqInstanceName.substr(n + 1, std::string::npos);
      Return<sp<IBase>> interfaceRet = sm->get(fqInterfaceName, instanceName);
      if (!interfaceRet.isOk()) {
        fprintf(stderr, "failed to get service %s: %s\n",
                fqInstanceName.c_str(), interfaceRet.description().c_str());
        continue;
      }
      sp<IBase> interface = interfaceRet;
      auto notifyRet = interface->setHALInstrumentation();
      if (!notifyRet.isOk()) {
        fprintf(stderr, "failed to setHALInstrumentation on service %s: %s\n",
                fqInstanceName.c_str(), notifyRet.description().c_str());
      }
      printf("- updated the HAL instrumentation mode setting for %s\n",
             fqInstanceName.c_str());
    }
  });
  if (!listRet.isOk()) {
    fprintf(stderr, "failed to list services: %s\n",
            listRet.description().c_str());
    return false;
  }
  return true;
}

bool EnableHALProfiling() {
  property_set("hal.instrumentation.enable", "true");
  if (!SetHALInstrumentation()) {
    fprintf(stderr, "failed to set instrumentation on services.\n");
    return false;
  }
  return true;
}

bool DisableHALProfiling() {
  property_set("hal.instrumentation.enable", "false");
  if (!SetHALInstrumentation()) {
    fprintf(stderr, "failed to set instrumentation on services.\n");
    return false;
  }
  return true;
}

void PrintUsage() {
  printf(
      "Usage: \n"
      "To enable profiling: <binary> enable <lib path 32> <lib path 64>"
      "To disable profiling <binary> disable");
}

int main(int argc, char *argv[]) {
  if (argc == 2 && !strcmp(argv[1], "disable")) {
    printf("* disable profiling.\n");
    property_set("hal.instrumentation.lib.path.32", "");
    property_set("hal.instrumentation.lib.path.64", "");
    if (!DisableHALProfiling()) {
      printf("failed to disable profiling.\n");
      return -1;
    }
  } else if (argc >= 2 && !strcmp(argv[1], "enable")) {
    printf("* enable profiling.\n");
    if (argc == 4) {
      property_set("hal.instrumentation.lib.path.32", argv[2]);
      property_set("hal.instrumentation.lib.path.64", argv[3]);
    }
    if (!EnableHALProfiling()) {
      printf("failed to enable profiling.\n");
      return -1;
    }
  } else {
    PrintUsage();
  }
  return 0;
}
