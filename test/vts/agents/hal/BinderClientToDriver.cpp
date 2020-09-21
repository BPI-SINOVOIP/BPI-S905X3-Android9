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

#include <iostream>

#include <utils/RefBase.h>
#define LOG_TAG "VtsFuzzerBinderClient"
#include <utils/Log.h>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "BinderClientToDriver.h"
#include "binder/VtsFuzzerBinderService.h"

using namespace std;

namespace android {
namespace vts {

// Returns a VtsFuzzer binder service's client.
sp<IVtsFuzzer> GetBinderClient(const string& service_name) {
  android::sp<IServiceManager> manager = defaultServiceManager();
  if (!manager.get()) {
    cerr << "can't get the default service manager." << std::endl;
    return NULL;
  }

  sp<IBinder> binder = manager->getService(String16(service_name.c_str()));
  if (!binder.get()) {
    cerr << "can't find the " << service_name << " binder service."
         << std::endl;
    return NULL;
  }

  sp<IVtsFuzzer> fuzzer = interface_cast<IVtsFuzzer>(binder);
  if (!fuzzer.get()) {
    cerr << "can't cast the obtained " << service_name << " binder instance"
         << std::endl;
    return NULL;
  }

  return fuzzer;
}

}  // namespace vts
}  // namespace android
