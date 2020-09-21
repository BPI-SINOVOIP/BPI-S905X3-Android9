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

#include <stdlib.h>

#include <iostream>
#include <string>

#include <utils/RefBase.h>
#define LOG_TAG "VtsFuzzerBinderService"
#include <utils/Log.h>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/TextOutput.h>

#include "binder/VtsFuzzerBinderService.h"

using namespace std;

namespace android {
namespace vts {

IMPLEMENT_META_INTERFACE(VtsFuzzer, VTS_FUZZER_BINDER_SERVICE_NAME);

void BpVtsFuzzer::Exit() {
  Parcel data;
  Parcel reply;
  data.writeInterfaceToken(IVtsFuzzer::getInterfaceDescriptor());
  data.writeString16(String16("Exit code"));
  remote()->transact(EXIT, data, &reply, IBinder::FLAG_ONEWAY);
}

int32_t BpVtsFuzzer::LoadHal(const string& path, int target_class,
                             int target_type, float target_version,
                             const string& module_name) {
  Parcel data;
  Parcel reply;

  printf("agent->driver: LoadHal(%s, %d, %d, %f, %s)\n", path.c_str(),
         target_class, target_type, target_version, module_name.c_str());
  data.writeInterfaceToken(IVtsFuzzer::getInterfaceDescriptor());
  data.writeCString(path.c_str());
  data.writeInt32(target_class);
  data.writeInt32(target_type);
  data.writeFloat(target_version);
  data.writeCString(module_name.c_str());

#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << "BpVtsFuzzer::Status request parcel:\n"
       << data
       << endl;
#endif

  remote()->transact(LOAD_HAL, data, &reply);

#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << "BpVtsFuzzer::Status response parcel:\n"
       << reply
       << endl;
#endif

  int32_t res;
  status_t status = reply.readInt32(&res);

  printf("driver->agent: LoadHal returns %d\n", status);
  return res;
}

int32_t BpVtsFuzzer::Status(int32_t type) {
  Parcel data;
  Parcel reply;

  data.writeInterfaceToken(IVtsFuzzer::getInterfaceDescriptor());
  data.writeInt32(type);

#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << "BpVtsFuzzer::Status request parcel:\n"
       << data
       << endl;
#endif

  remote()->transact(STATUS, data, &reply);

#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << "BpVtsFuzzer::Status response parcel:\n"
       << reply
       << endl;
#endif

  int32_t res;
  /* status_t */ reply.readInt32(&res);
  return res;
}

string BpVtsFuzzer::Call(const string& call_payload) {
  Parcel data, reply;
  data.writeInterfaceToken(IVtsFuzzer::getInterfaceDescriptor());
  data.writeCString(call_payload.c_str());
#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << data << endl;
#endif

  remote()->transact(CALL, data, &reply);
#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << reply << endl;
#endif

  const char* res = reply.readCString();
  if (res == NULL) {
    printf("reply == NULL\n");
    return res;
  }

  printf("len(reply) = %zu\n", strlen(res));
  return {res};
}

const char* BpVtsFuzzer::GetFunctions() {
  Parcel data, reply;
  data.writeInterfaceToken(IVtsFuzzer::getInterfaceDescriptor());
#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << data << endl;
#endif

  remote()->transact(GET_FUNCTIONS, data, &reply);
#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << reply << endl;
#endif

  const char* res = reply.readCString();
  if (res == NULL) {
    printf("reply == NULL\n");
    return res;
  }

  printf("len(reply) = %zu\n", strlen(res));
  return res;
}

}  // namespace vts
}  // namespace android
