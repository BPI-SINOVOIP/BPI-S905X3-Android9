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

#ifndef __VTS_HAL_HIDL_TARGET_TEST_ENV_BASE_H
#define __VTS_HAL_HIDL_TARGET_TEST_ENV_BASE_H

#include <gtest/gtest.h>

using namespace std;

namespace testing {

// A class for test environment setup
class VtsHalHidlTargetTestEnvBase : public ::testing::Environment {
 public:
  VtsHalHidlTargetTestEnvBase() {}

  /*
   * SetUp process, should not be overridden by the test.
   */
  void SetUp() final;

  /*
   * TearDown process, should not be overridden by the test.
   */
  void TearDown() final;

  /*
   * Test should override this method for any custom setup process.
   */
  virtual void HidlSetUp() {}

  /*
   * Test should override this method for any custom teardown process.
   */
  virtual void HidlTearDown() {}

  /*
   * Test should override this method to register hal services used in the test.
   */
  virtual void registerTestServices() {}

  /* Parses the command line argument, extracts the vts reserved flags and
   * leaves other options untouched.
   * Must be called when the test environment is created is registered.
   */
  void init(int* argc, char** argv);

  /*
   * Adds a hal sevice identified into registeredHalServices_.
   */
  template <class T>
  void registerTestService() {
    registerTestService(T::descriptor);
  }

  /*
   * Gets the service name for a hal instance. Returns empty string if the hal
   * instance is unkonwn (not in hal_instances_).
   */
  template <class T>
  string getServiceName() {
    return getServiceName(T::descriptor);
  }

 private:
  /*
   * Parses VTS specific flags, currently support two flags:
   * --list_registered_services to print all registered service.
   * --hal_service_instance to pass a running service instance. e.g.
   * --hal_service_instance=android.hardware.vibrator@1.0::IVibrator/default
   * It is possible to have mulitple --hal_service_instance options passed if
   * mutliple hal service is used in the test.
   * Returns true if successfully pased the given arg, false if arg is null or
   * unknown flag.
   */
  bool parseVtsTestOption(const char* arg);

  /*
   * Prints all registered sercives.
   */
  void listRegisteredServices();

  /*
   * Internal method to get the service name for a hal instance.
   */
  string getServiceName(string instanceName);

  /*
   * Internal method to register a HAL sevice identified with the FQName.
   */
  void registerTestService(string FQName);

  /*
   * Internal method to add a hal service instance.
   */
  void addHalServiceInstance(string halServiceInstance);

  // Map of hal instances with their correpoding service names.
  map<string, string> halServiceInstances_;
  // Set of all hal services used in the test.
  set<string> registeredHalServices_;
  // Flag to print registered hal services and exit the process.
  bool listService_ = false;
  // Flag whether init is called.
  bool inited_ = false;
};

}  // namespace testing

#endif  // __VTS_HAL_HIDL_TARGET_TEST_ENV_BASE_H
