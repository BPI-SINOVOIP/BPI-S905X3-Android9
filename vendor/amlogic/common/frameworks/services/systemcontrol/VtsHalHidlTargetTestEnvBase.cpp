/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "VtsHalHidlTargetTestEnvBase"

#include "VtsHalHidlTargetTestEnvBase.h"

#include <string>

#include <utils/Log.h>

static constexpr const char* kListFlag = "--list_registered_services";
static constexpr const char* kServceInstanceFlag = "--hal_service_instance";

using namespace std;

namespace testing {

void VtsHalHidlTargetTestEnvBase::SetUp() {
  if (!inited_) {
    ALOGE("Environment not inited, did you forget to call init()?");
    abort();
  }
  // Register services used in the test.
  registerTestServices();
  // For a dummy run which just print the registered hal services.
  if (listService_) {
    listRegisteredServices();
    exit(0);
  }
  // Call the customized setup process.
  HidlSetUp();
}

void VtsHalHidlTargetTestEnvBase::TearDown() {
  // Call the customized teardown process.
  HidlTearDown();
}

void VtsHalHidlTargetTestEnvBase::init(int* argc, char** argv) {
  if (inited_) return;
  for (int i = 1; i < *argc; i++) {
    if (parseVtsTestOption(argv[i])) {
      // Shift the remainder of the argv list left by one.
      for (int j = i; j != *argc; j++) {
        argv[j] = argv[j + 1];
      }

      // Decrements the argument count.
      (*argc)--;

      // We also need to decrement the iterator as we just removed an element.
      i--;
    }
  }
  inited_ = true;
}

bool VtsHalHidlTargetTestEnvBase::parseVtsTestOption(const char* arg) {
  // str and flag must not be NULL.
  if (arg == NULL) return false;

  if (strncmp(arg, kListFlag, strlen(kListFlag)) == 0) {
    listService_ = true;
    return true;
  }

  if (strncmp(arg, kServceInstanceFlag, strlen(kServceInstanceFlag)) == 0) {
    // value is the past after "--hal_service_instance="
    const char* value = arg + strlen(kServceInstanceFlag) + 1;
    addHalServiceInstance(string(value));
    return true;
  }
  return false;
}

void VtsHalHidlTargetTestEnvBase::addHalServiceInstance(
    string halServiceInstance) {
  // hal_service_instance follows the format:
  // package@version::interface/service_name e.g.:
  // android.hardware.vibrator@1.0::IVibrator/default
  string instance_name =
      halServiceInstance.substr(0, halServiceInstance.find('/'));
  string service_name =
      halServiceInstance.substr(halServiceInstance.find('/') + 1);
  // Fail the process if trying to pass multiple service names for the same
  // service instance.
  if (halServiceInstances_.find(instance_name) != halServiceInstances_.end()) {
    ALOGE("Exisitng instance %s with name %s", instance_name.c_str(),
          halServiceInstances_[instance_name].c_str());
    abort();
  }
  halServiceInstances_[instance_name] = service_name;
}

string VtsHalHidlTargetTestEnvBase::getServiceName(string instanceName) {
  if (halServiceInstances_.find(instanceName) != halServiceInstances_.end()) {
    return halServiceInstances_[instanceName];
  }
  // Could not find the instance.
  return "";
}

void VtsHalHidlTargetTestEnvBase::registerTestService(string FQName) {
  registeredHalServices_.insert(FQName);
}

void VtsHalHidlTargetTestEnvBase::listRegisteredServices() {
  for (string service : registeredHalServices_) {
    printf("hal_service: %s\n", service.c_str());
  }
}

}  // namespace testing
