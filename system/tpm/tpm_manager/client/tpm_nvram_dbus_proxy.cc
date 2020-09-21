//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "tpm_manager/client/tpm_nvram_dbus_proxy.h"

#include <brillo/bind_lambda.h>
#include <brillo/dbus/dbus_method_invoker.h>

#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/common/tpm_nvram_dbus_interface.h"

namespace {

// Use a two minute timeout because TPM operations can take a long time.
const int kDBusTimeoutMS = 2 * 60 * 1000;

}  // namespace

namespace tpm_manager {

TpmNvramDBusProxy::~TpmNvramDBusProxy() {
  if (bus_) {
    bus_->ShutdownAndBlock();
  }
}

bool TpmNvramDBusProxy::Initialize() {
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  bus_ = new dbus::Bus(options);
  object_proxy_ = bus_->GetObjectProxy(
      tpm_manager::kTpmManagerServiceName,
      dbus::ObjectPath(tpm_manager::kTpmManagerServicePath));
  return (object_proxy_ != nullptr);
}

void TpmNvramDBusProxy::DefineSpace(const DefineSpaceRequest& request,
                                    const DefineSpaceCallback& callback) {
  CallMethod<DefineSpaceReply>(tpm_manager::kDefineSpace, request, callback);
}

void TpmNvramDBusProxy::DestroySpace(const DestroySpaceRequest& request,
                                     const DestroySpaceCallback& callback) {
  CallMethod<DestroySpaceReply>(tpm_manager::kDestroySpace, request, callback);
}

void TpmNvramDBusProxy::WriteSpace(const WriteSpaceRequest& request,
                                   const WriteSpaceCallback& callback) {
  CallMethod<WriteSpaceReply>(tpm_manager::kWriteSpace, request, callback);
}

void TpmNvramDBusProxy::ReadSpace(const ReadSpaceRequest& request,
                                  const ReadSpaceCallback& callback) {
  CallMethod<ReadSpaceReply>(tpm_manager::kReadSpace, request, callback);
}

void TpmNvramDBusProxy::LockSpace(const LockSpaceRequest& request,
                                  const LockSpaceCallback& callback) {
  CallMethod<LockSpaceReply>(tpm_manager::kLockSpace, request, callback);
}

void TpmNvramDBusProxy::ListSpaces(const ListSpacesRequest& request,
                                   const ListSpacesCallback& callback) {
  CallMethod<ListSpacesReply>(tpm_manager::kListSpaces, request, callback);
}

void TpmNvramDBusProxy::GetSpaceInfo(const GetSpaceInfoRequest& request,
                                     const GetSpaceInfoCallback& callback) {
  CallMethod<GetSpaceInfoReply>(tpm_manager::kGetSpaceInfo, request, callback);
}

template <typename ReplyProtobufType,
          typename RequestProtobufType,
          typename CallbackType>
void TpmNvramDBusProxy::CallMethod(const std::string& method_name,
                                   const RequestProtobufType& request,
                                   const CallbackType& callback) {
  auto on_error = [callback](brillo::Error* error) {
    ReplyProtobufType reply;
    reply.set_result(NVRAM_RESULT_IPC_ERROR);
    callback.Run(reply);
  };
  brillo::dbus_utils::CallMethodWithTimeout(
      kDBusTimeoutMS, object_proxy_, tpm_manager::kTpmNvramInterface,
      method_name, callback, base::Bind(on_error), request);
}

}  // namespace tpm_manager
