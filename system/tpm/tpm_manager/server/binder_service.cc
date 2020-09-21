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

#include "tpm_manager/server/binder_service.h"

#include <sysexits.h>

#include <base/bind.h>
#include <binderwrapper/binder_wrapper.h>

#include "tpm_manager/common/tpm_manager.pb.h"
#include "tpm_manager/common/tpm_manager_constants.h"

namespace {

// Sends a |response_proto| to |client| for an arbitrary protobuf type.
template <typename ResponseProtobufType>
void ResponseHandler(
    const android::sp<android::tpm_manager::ITpmManagerClient>& client,
    const ResponseProtobufType& response_proto) {
  VLOG(2) << __func__;
  std::vector<uint8_t> binder_response;
  binder_response.resize(response_proto.ByteSize());
  CHECK(response_proto.SerializeToArray(binder_response.data(),
                                        binder_response.size()))
      << "BinderService: Failed to serialize protobuf.";
  android::binder::Status status = client->OnCommandResponse(binder_response);
  if (!status.isOk()) {
    LOG(ERROR) << "BinderService: Failed to send response to client: "
               << status.toString8();
  }
}

// Creates an error protobuf for NVRAM commands.
template <typename ResponseProtobufType>
void CreateNvramErrorResponse(ResponseProtobufType* proto) {
  proto->set_result(tpm_manager::NVRAM_RESULT_IPC_ERROR);
}

// Creates an error protobuf for ownership commands.
template <typename ResponseProtobufType>
void CreateOwnershipErrorResponse(ResponseProtobufType* proto) {
  proto->set_status(tpm_manager::STATUS_DEVICE_ERROR);
}

// Calls |method| with a protobuf decoded from |request| using ResponseHandler()
// and |client| to handle the response. On error, uses |get_error_response| to
// construct a response and sends that to |client|.
template <typename RequestProtobufType, typename ResponseProtobufType>
void RequestHandler(
    const std::vector<uint8_t>& request,
    const base::Callback<
        void(const RequestProtobufType&,
             const base::Callback<void(const ResponseProtobufType&)>&)>& method,
    const base::Callback<void(ResponseProtobufType*)>& get_error_response,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  VLOG(2) << __func__;
  base::Callback<void(const ResponseProtobufType&)> callback =
      base::Bind(ResponseHandler<ResponseProtobufType>, client);
  RequestProtobufType request_proto;
  if (!request_proto.ParseFromArray(request.data(), request.size())) {
    LOG(ERROR) << "BinderService: Bad request data.";
    // Send an error response.
    ResponseProtobufType response_proto;
    get_error_response.Run(&response_proto);
    callback.Run(response_proto);
    return;
  }
  method.Run(request_proto, callback);
}

}  // namespace

namespace tpm_manager {

BinderService::BinderService(TpmNvramInterface* nvram_service,
                             TpmOwnershipInterface* ownership_service)
    : nvram_service_(nvram_service), ownership_service_(ownership_service) {}

void BinderService::InitForTesting() {
  nvram_binder_ = new NvramServiceInternal(nvram_service_);
  ownership_binder_ = new OwnershipServiceInternal(ownership_service_);
}

int BinderService::OnInit() {
  if (!watcher_.Init()) {
    LOG(ERROR) << "BinderService: BinderWatcher::Init failed.";
    return EX_UNAVAILABLE;
  }
  nvram_binder_ = new NvramServiceInternal(nvram_service_);
  ownership_binder_ = new OwnershipServiceInternal(ownership_service_);
  if (!android::BinderWrapper::GetOrCreateInstance()->RegisterService(
          kTpmNvramBinderName, android::IInterface::asBinder(nvram_binder_))) {
    LOG(ERROR) << "BinderService: RegisterService failed (nvram).";
    return EX_UNAVAILABLE;
  }
  if (!android::BinderWrapper::GetOrCreateInstance()->RegisterService(
          kTpmOwnershipBinderName,
          android::IInterface::asBinder(ownership_binder_))) {
    LOG(ERROR) << "BinderService: RegisterService failed (ownership).";
    return EX_UNAVAILABLE;
  }
  LOG(INFO) << "TpmManager: Binder services registered.";
  return brillo::Daemon::OnInit();
}

android::tpm_manager::ITpmNvram* BinderService::GetITpmNvram() {
  return nvram_binder_.get();
}

android::tpm_manager::ITpmOwnership* BinderService::GetITpmOwnership() {
  return ownership_binder_.get();
}

BinderService::NvramServiceInternal::NvramServiceInternal(
    TpmNvramInterface* nvram_service)
    : nvram_service_(nvram_service) {}

android::binder::Status BinderService::NvramServiceInternal::DefineSpace(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<DefineSpaceRequest, DefineSpaceReply>(
      command_proto, base::Bind(&TpmNvramInterface::DefineSpace,
                                base::Unretained(nvram_service_)),
      base::Bind(CreateNvramErrorResponse<DefineSpaceReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status BinderService::NvramServiceInternal::DestroySpace(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<DestroySpaceRequest, DestroySpaceReply>(
      command_proto, base::Bind(&TpmNvramInterface::DestroySpace,
                                base::Unretained(nvram_service_)),
      base::Bind(CreateNvramErrorResponse<DestroySpaceReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status BinderService::NvramServiceInternal::WriteSpace(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<WriteSpaceRequest, WriteSpaceReply>(
      command_proto, base::Bind(&TpmNvramInterface::WriteSpace,
                                base::Unretained(nvram_service_)),
      base::Bind(CreateNvramErrorResponse<WriteSpaceReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status BinderService::NvramServiceInternal::ReadSpace(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<ReadSpaceRequest, ReadSpaceReply>(
      command_proto, base::Bind(&TpmNvramInterface::ReadSpace,
                                base::Unretained(nvram_service_)),
      base::Bind(CreateNvramErrorResponse<ReadSpaceReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status BinderService::NvramServiceInternal::LockSpace(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<LockSpaceRequest, LockSpaceReply>(
      command_proto, base::Bind(&TpmNvramInterface::LockSpace,
                                base::Unretained(nvram_service_)),
      base::Bind(CreateNvramErrorResponse<LockSpaceReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status BinderService::NvramServiceInternal::ListSpaces(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<ListSpacesRequest, ListSpacesReply>(
      command_proto, base::Bind(&TpmNvramInterface::ListSpaces,
                                base::Unretained(nvram_service_)),
      base::Bind(CreateNvramErrorResponse<ListSpacesReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status BinderService::NvramServiceInternal::GetSpaceInfo(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<GetSpaceInfoRequest, GetSpaceInfoReply>(
      command_proto, base::Bind(&TpmNvramInterface::GetSpaceInfo,
                                base::Unretained(nvram_service_)),
      base::Bind(CreateNvramErrorResponse<GetSpaceInfoReply>), client);
  return android::binder::Status::ok();
}

BinderService::OwnershipServiceInternal::OwnershipServiceInternal(
    TpmOwnershipInterface* ownership_service)
    : ownership_service_(ownership_service) {}

android::binder::Status BinderService::OwnershipServiceInternal::GetTpmStatus(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<GetTpmStatusRequest, GetTpmStatusReply>(
      command_proto, base::Bind(&TpmOwnershipInterface::GetTpmStatus,
                                base::Unretained(ownership_service_)),
      base::Bind(CreateOwnershipErrorResponse<GetTpmStatusReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status BinderService::OwnershipServiceInternal::TakeOwnership(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<TakeOwnershipRequest, TakeOwnershipReply>(
      command_proto, base::Bind(&TpmOwnershipInterface::TakeOwnership,
                                base::Unretained(ownership_service_)),
      base::Bind(CreateOwnershipErrorResponse<TakeOwnershipReply>), client);
  return android::binder::Status::ok();
}

android::binder::Status
BinderService::OwnershipServiceInternal::RemoveOwnerDependency(
    const std::vector<uint8_t>& command_proto,
    const android::sp<android::tpm_manager::ITpmManagerClient>& client) {
  RequestHandler<RemoveOwnerDependencyRequest, RemoveOwnerDependencyReply>(
      command_proto, base::Bind(&TpmOwnershipInterface::RemoveOwnerDependency,
                                base::Unretained(ownership_service_)),
      base::Bind(CreateOwnershipErrorResponse<RemoveOwnerDependencyReply>),
      client);
  return android::binder::Status::ok();
}

}  // namespace tpm_manager
