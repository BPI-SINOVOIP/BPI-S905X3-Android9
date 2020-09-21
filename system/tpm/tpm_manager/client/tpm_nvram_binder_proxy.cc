//
// Copyright (C) 2016 The Android Open Source Project
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

#include "tpm_manager/client/tpm_nvram_binder_proxy.h"

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <binderwrapper/binder_wrapper.h>
#include <utils/Errors.h>

#include "android/tpm_manager/BnTpmManagerClient.h"
#include "android/tpm_manager/BpTpmNvram.h"
#include "tpm_manager/client/binder_proxy_helper.h"
#include "tpm_manager/common/tpm_manager.pb.h"
#include "tpm_manager/common/tpm_manager_constants.h"

namespace {

template <typename ResponseProtobufType>
void CreateErrorResponse(ResponseProtobufType* response) {
  response->set_result(tpm_manager::NVRAM_RESULT_IPC_ERROR);
}

}  // namespace

namespace tpm_manager {

using android::tpm_manager::ITpmNvram;

TpmNvramBinderProxy::TpmNvramBinderProxy(ITpmNvram* binder) : binder_(binder) {}

bool TpmNvramBinderProxy::Initialize() {
  if (binder_) {
    return true;
  }
  android::sp<android::IBinder> service_binder =
      android::BinderWrapper::GetOrCreateInstance()->GetService(
          kTpmNvramBinderName);
  if (!service_binder.get()) {
    LOG(ERROR) << "TpmNvramBinderProxy: Service does not exist.";
    return false;
  }
  default_binder_ = new android::tpm_manager::BpTpmNvram(service_binder);
  binder_ = default_binder_.get();
  return true;
}

void TpmNvramBinderProxy::DefineSpace(const DefineSpaceRequest& request,
                                      const DefineSpaceCallback& callback) {
  auto method = base::Bind(&ITpmNvram::DefineSpace, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<DefineSpaceReply>);
  BinderProxyHelper<DefineSpaceRequest, DefineSpaceReply> helper(
      method, callback, get_error);
  helper.SendRequest(request);
}

void TpmNvramBinderProxy::DestroySpace(const DestroySpaceRequest& request,
                                       const DestroySpaceCallback& callback) {
  auto method = base::Bind(&ITpmNvram::DestroySpace, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<DestroySpaceReply>);
  BinderProxyHelper<DestroySpaceRequest, DestroySpaceReply> helper(
      method, callback, get_error);
  helper.SendRequest(request);
}

void TpmNvramBinderProxy::WriteSpace(const WriteSpaceRequest& request,
                                     const WriteSpaceCallback& callback) {
  auto method = base::Bind(&ITpmNvram::WriteSpace, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<WriteSpaceReply>);
  BinderProxyHelper<WriteSpaceRequest, WriteSpaceReply> helper(method, callback,
                                                               get_error);
  helper.SendRequest(request);
}

void TpmNvramBinderProxy::ReadSpace(const ReadSpaceRequest& request,
                                    const ReadSpaceCallback& callback) {
  auto method = base::Bind(&ITpmNvram::ReadSpace, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<ReadSpaceReply>);
  BinderProxyHelper<ReadSpaceRequest, ReadSpaceReply> helper(method, callback,
                                                             get_error);
  helper.SendRequest(request);
}

void TpmNvramBinderProxy::LockSpace(const LockSpaceRequest& request,
                                    const LockSpaceCallback& callback) {
  auto method = base::Bind(&ITpmNvram::LockSpace, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<LockSpaceReply>);
  BinderProxyHelper<LockSpaceRequest, LockSpaceReply> helper(method, callback,
                                                             get_error);
  helper.SendRequest(request);
}

void TpmNvramBinderProxy::ListSpaces(const ListSpacesRequest& request,
                                     const ListSpacesCallback& callback) {
  auto method = base::Bind(&ITpmNvram::ListSpaces, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<ListSpacesReply>);
  BinderProxyHelper<ListSpacesRequest, ListSpacesReply> helper(method, callback,
                                                               get_error);
  helper.SendRequest(request);
}

void TpmNvramBinderProxy::GetSpaceInfo(const GetSpaceInfoRequest& request,
                                       const GetSpaceInfoCallback& callback) {
  auto method = base::Bind(&ITpmNvram::GetSpaceInfo, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<GetSpaceInfoReply>);
  BinderProxyHelper<GetSpaceInfoRequest, GetSpaceInfoReply> helper(
      method, callback, get_error);
  helper.SendRequest(request);
}

}  // namespace tpm_manager
