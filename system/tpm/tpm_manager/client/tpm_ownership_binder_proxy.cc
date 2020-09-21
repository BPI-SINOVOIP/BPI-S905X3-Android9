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

#include "tpm_manager/client/tpm_ownership_binder_proxy.h"

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <binderwrapper/binder_wrapper.h>
#include <utils/Errors.h>

#include "android/tpm_manager/BnTpmManagerClient.h"
#include "android/tpm_manager/BpTpmOwnership.h"
#include "tpm_manager/client/binder_proxy_helper.h"
#include "tpm_manager/common/tpm_manager.pb.h"
#include "tpm_manager/common/tpm_manager_constants.h"

namespace {

template <typename ResponseProtobufType>
void CreateErrorResponse(ResponseProtobufType* response) {
  response->set_status(tpm_manager::STATUS_DEVICE_ERROR);
}

}  // namespace

namespace tpm_manager {

using android::tpm_manager::ITpmOwnership;

TpmOwnershipBinderProxy::TpmOwnershipBinderProxy(ITpmOwnership* binder)
    : binder_(binder) {}

bool TpmOwnershipBinderProxy::Initialize() {
  if (binder_) {
    return true;
  }
  android::sp<android::IBinder> service_binder =
      android::BinderWrapper::GetOrCreateInstance()->GetService(
          kTpmOwnershipBinderName);
  if (!service_binder.get()) {
    LOG(ERROR) << "TpmOwnershipBinderProxy: Service does not exist.";
    return false;
  }
  default_binder_ = new android::tpm_manager::BpTpmOwnership(service_binder);
  binder_ = default_binder_.get();
  return true;
}

void TpmOwnershipBinderProxy::GetTpmStatus(
    const GetTpmStatusRequest& request,
    const GetTpmStatusCallback& callback) {
  auto method =
      base::Bind(&ITpmOwnership::GetTpmStatus, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<GetTpmStatusReply>);
  BinderProxyHelper<GetTpmStatusRequest, GetTpmStatusReply> helper(
      method, callback, get_error);
  helper.SendRequest(request);
}

void TpmOwnershipBinderProxy::TakeOwnership(
    const TakeOwnershipRequest& request,
    const TakeOwnershipCallback& callback) {
  auto method =
      base::Bind(&ITpmOwnership::TakeOwnership, base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<TakeOwnershipReply>);
  BinderProxyHelper<TakeOwnershipRequest, TakeOwnershipReply> helper(
      method, callback, get_error);
  helper.SendRequest(request);
}

void TpmOwnershipBinderProxy::RemoveOwnerDependency(
    const RemoveOwnerDependencyRequest& request,
    const RemoveOwnerDependencyCallback& callback) {
  auto method = base::Bind(&ITpmOwnership::RemoveOwnerDependency,
                           base::Unretained(binder_));
  auto get_error = base::Bind(&CreateErrorResponse<RemoveOwnerDependencyReply>);
  BinderProxyHelper<RemoveOwnerDependencyRequest, RemoveOwnerDependencyReply>
      helper(method, callback, get_error);
  helper.SendRequest(request);
}

}  // namespace tpm_manager
