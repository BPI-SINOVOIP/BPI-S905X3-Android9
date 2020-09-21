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

#ifdef VTS_AGENT_DRIVER_COMM_BINDER  // binder

#include "BinderServer.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>

#include <utils/RefBase.h>
#define LOG_TAG "VtsFuzzerBinderServer"
#include <utils/Log.h>
#include <utils/String8.h>

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/TextOutput.h>

#include "binder/VtsFuzzerBinderService.h"

#include <google/protobuf/text_format.h>
#include "test/vts/proto/ComponentSpecificationMessage.pb.h"

using namespace std;

namespace android {
namespace vts {

class BnVtsFuzzer : public BnInterface<IVtsFuzzer> {
  virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                              uint32_t flags = 0);
};

status_t BnVtsFuzzer::onTransact(uint32_t code, const Parcel& data,
                                 Parcel* reply, uint32_t flags) {
  ALOGD("BnVtsFuzzer::%s(%i) %i", __func__, code, flags);

  data.checkInterface(this);
#ifdef VTS_FUZZER_BINDER_DEBUG
  alog << data << endl;
#endif

  switch (code) {
    case EXIT:
      Exit();
      break;
    case LOAD_HAL: {
      const char* path = data.readCString();
      const int target_class = data.readInt32();
      const int target_type = data.readInt32();
      const float target_version = data.readFloat();
      const char* module_name = data.readCString();
      int32_t result = LoadHal(string(path), target_class, target_type,
                               target_version, string(module_name));
      ALOGD("BnVtsFuzzer::%s LoadHal(%s) -> %i", __FUNCTION__, path, result);
      if (reply == NULL) {
        ALOGE("reply == NULL");
        abort();
      }
#ifdef VTS_FUZZER_BINDER_DEBUG
      alog << reply << endl;
#endif
      reply->writeInt32(result);
      break;
    }
    case STATUS: {
      int32_t type = data.readInt32();
      int32_t result = Status(type);

      ALOGD("BnVtsFuzzer::%s status(%i) -> %i", __FUNCTION__, type, result);
      if (reply == NULL) {
        ALOGE("reply == NULL");
        abort();
      }
#ifdef VTS_FUZZER_BINDER_DEBUG
      alog << reply << endl;
#endif
      reply->writeInt32(result);
      break;
    }
    case CALL: {
      const char* arg = data.readCString();
      const string& result = Call(arg);

      ALOGD("BnVtsFuzzer::%s call(%s) = %i", __FUNCTION__, arg, result.c_str());
      if (reply == NULL) {
        ALOGE("reply == NULL");
        abort();
      }
#ifdef VTS_FUZZER_BINDER_DEBUG
      alog << reply << endl;
#endif
      reply->writeCString(result.c_str());
      break;
    }
    case GET_FUNCTIONS: {
      const char* result = GetFunctions();

      if (reply == NULL) {
        ALOGE("reply == NULL");
        abort();
      }
#ifdef VTS_FUZZER_BINDER_DEBUG
      alog << reply << endl;
#endif
      reply->writeCString(result);
      break;
    }
    default:
      return BBinder::onTransact(code, data, reply, flags);
  }
  return NO_ERROR;
}

class VtsFuzzerServer : public BnVtsFuzzer {
 public:
  VtsFuzzerServer(android::vts::VtsHalDriverManager* driver_manager,
                  const char* lib_path)
      : driver_manager_(driver_manager), lib_path_(lib_path) {}

  void Exit() { printf("VtsFuzzerServer::Exit\n"); }

  int32_t LoadHal(const string& path, int target_class, int target_type,
                  float target_version, const string& module_name) {
    printf("VtsFuzzerServer::LoadHal(%s)\n", path.c_str());
    bool success = driver_manager_->LoadTargetComponent(
        path.c_str(), lib_path_, target_class, target_type, target_version, "",
        "", "", module_name.c_str());
    cout << "Result: " << success << std::endl;
    if (success) {
      return 0;
    } else {
      return -1;
    }
  }

  int32_t Status(int32_t type) {
    printf("VtsFuzzerServer::Status(%i)\n", type);
    return 0;
  }

  string Call(const string& arg) {
    printf("VtsFuzzerServer::Call(%s)\n", arg.c_str());
    FunctionCallMessage* call_msg = new FunctionCallMessage();
    google::protobuf::TextFormat::MergeFromString(arg, call_msg);
    return driver_manager_->CallFunction(call_msg);
  }

  const char* GetFunctions() {
    printf("Get functions*");
    vts::ComponentSpecificationMessage* spec =
        driver_manager_->GetComponentSpecification();
    if (!spec) {
      return NULL;
    }
    string* output = new string();
    printf("getfunctions serial1\n");
    if (google::protobuf::TextFormat::PrintToString(*spec, output)) {
      printf("getfunctions length %d\n", output->length());
      return output->c_str();
    } else {
      printf("can't serialize the interface spec message to a string.\n");
      return NULL;
    }
  }

 private:
  android::vts::VtsHalDriverManager* driver_manager_;
  const char* lib_path_;
};

void StartBinderServer(const string& service_name,
                       android::vts::VtsHalDriverManager* driver_manager,
                       const char* lib_path) {
  defaultServiceManager()->addService(
      String16(service_name.c_str()),
      new VtsFuzzerServer(driver_manager, lib_path));
  android::ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();
}

}  // namespace vts
}  // namespace android

#endif
