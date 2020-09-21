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

#ifndef __VTS_FUZZER_BINDER_SERVICE_H__
#define __VTS_FUZZER_BINDER_SERVICE_H__

#include <string>

#include <utils/RefBase.h>
#include <utils/String8.h>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/ProcessState.h>

// #ifdef VTS_FUZZER_BINDER_DEBUG

#define VTS_FUZZER_BINDER_SERVICE_NAME "VtsFuzzer"

using namespace std;

namespace android {
namespace vts {

// VTS Fuzzer Binder Interface
class IVtsFuzzer : public IInterface {
 public:
  enum {
    EXIT = IBinder::FIRST_CALL_TRANSACTION,
    LOAD_HAL,
    STATUS,
    CALL,
    GET_FUNCTIONS
  };

  // Sends an exit command.
  virtual void Exit() = 0;

  // Requests to load a HAL.
  virtual int32_t LoadHal(const string& path, int target_class, int target_type,
                          float target_version, const string& module_name) = 0;

  // Requests to return the specified status.
  virtual int32_t Status(int32_t type) = 0;

  // Requests to call the specified function using the provided arguments.
  virtual string Call(const string& call_payload) = 0;

  virtual const char* GetFunctions() = 0;

  DECLARE_META_INTERFACE(VtsFuzzer);
};

// For client
class BpVtsFuzzer : public BpInterface<IVtsFuzzer> {
 public:
  BpVtsFuzzer(const sp<IBinder>& impl) : BpInterface<IVtsFuzzer>(impl) {}

  void Exit();
  int32_t LoadHal(const string& path, int target_class, int target_type,
                  float target_version, const string& module_name);
  int32_t Status(int32_t type);
  string Call(const string& call_payload);
  const char* GetFunctions();
};

}  // namespace vts
}  // namespace android

#endif
