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

#include "tpm_manager/server/tpm_manager_service.h"

#include <base/callback.h>
#include <base/command_line.h>
#include <brillo/bind_lambda.h>

namespace tpm_manager {

TpmManagerService::TpmManagerService(bool wait_for_ownership,
                                     LocalDataStore* local_data_store,
                                     TpmStatus* tpm_status,
                                     TpmInitializer* tpm_initializer,
                                     TpmNvram* tpm_nvram)
    : local_data_store_(local_data_store),
      tpm_status_(tpm_status),
      tpm_initializer_(tpm_initializer),
      tpm_nvram_(tpm_nvram),
      wait_for_ownership_(wait_for_ownership),
      weak_factory_(this) {}

bool TpmManagerService::Initialize() {
  worker_thread_.reset(new base::Thread("TpmManager Service Worker"));
  worker_thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  base::Closure task =
      base::Bind(&TpmManagerService::InitializeTask, base::Unretained(this));
  worker_thread_->task_runner()->PostNonNestableTask(FROM_HERE, task);
  VLOG(1) << "Worker thread started.";
  return true;
}

void TpmManagerService::InitializeTask() {
  VLOG(1) << "Initializing service...";
  if (!tpm_status_->IsTpmEnabled()) {
    LOG(WARNING) << __func__ << ": TPM is disabled.";
    return;
  }
  tpm_initializer_->VerifiedBootHelper();
  if (!wait_for_ownership_) {
    VLOG(1) << "Initializing TPM.";
    if (!tpm_initializer_->InitializeTpm()) {
      LOG(WARNING) << __func__ << ": TPM initialization failed.";
      return;
    }
  }
}

void TpmManagerService::GetTpmStatus(const GetTpmStatusRequest& request,
                                     const GetTpmStatusCallback& callback) {
  PostTaskToWorkerThread<GetTpmStatusReply>(
      request, callback, &TpmManagerService::GetTpmStatusTask);
}

void TpmManagerService::GetTpmStatusTask(
    const GetTpmStatusRequest& request,
    const std::shared_ptr<GetTpmStatusReply>& reply) {
  VLOG(1) << __func__;
  reply->set_enabled(tpm_status_->IsTpmEnabled());
  reply->set_owned(tpm_status_->IsTpmOwned());
  LocalData local_data;
  if (local_data_store_ && local_data_store_->Read(&local_data)) {
    *reply->mutable_local_data() = local_data;
  }
  int counter;
  int threshold;
  bool lockout;
  int lockout_time_remaining;
  if (tpm_status_->GetDictionaryAttackInfo(&counter, &threshold, &lockout,
                                           &lockout_time_remaining)) {
    reply->set_dictionary_attack_counter(counter);
    reply->set_dictionary_attack_threshold(threshold);
    reply->set_dictionary_attack_lockout_in_effect(lockout);
    reply->set_dictionary_attack_lockout_seconds_remaining(
        lockout_time_remaining);
  }
  reply->set_status(STATUS_SUCCESS);
}

void TpmManagerService::TakeOwnership(const TakeOwnershipRequest& request,
                                      const TakeOwnershipCallback& callback) {
  PostTaskToWorkerThread<TakeOwnershipReply>(
      request, callback, &TpmManagerService::TakeOwnershipTask);
}

void TpmManagerService::TakeOwnershipTask(
    const TakeOwnershipRequest& request,
    const std::shared_ptr<TakeOwnershipReply>& reply) {
  VLOG(1) << __func__;
  if (!tpm_status_->IsTpmEnabled()) {
    reply->set_status(STATUS_NOT_AVAILABLE);
    return;
  }
  if (!tpm_initializer_->InitializeTpm()) {
    reply->set_status(STATUS_DEVICE_ERROR);
    return;
  }
  reply->set_status(STATUS_SUCCESS);
}

void TpmManagerService::RemoveOwnerDependency(
    const RemoveOwnerDependencyRequest& request,
    const RemoveOwnerDependencyCallback& callback) {
  PostTaskToWorkerThread<RemoveOwnerDependencyReply>(
      request, callback, &TpmManagerService::RemoveOwnerDependencyTask);
}

void TpmManagerService::RemoveOwnerDependencyTask(
    const RemoveOwnerDependencyRequest& request,
    const std::shared_ptr<RemoveOwnerDependencyReply>& reply) {
  VLOG(1) << __func__;
  LocalData local_data;
  if (!local_data_store_->Read(&local_data)) {
    reply->set_status(STATUS_DEVICE_ERROR);
    return;
  }
  RemoveOwnerDependency(request.owner_dependency(), &local_data);
  if (!local_data_store_->Write(local_data)) {
    reply->set_status(STATUS_DEVICE_ERROR);
    return;
  }
  reply->set_status(STATUS_SUCCESS);
}

void TpmManagerService::RemoveOwnerDependency(
    const std::string& owner_dependency,
    LocalData* local_data) {
  google::protobuf::RepeatedPtrField<std::string>* dependencies =
      local_data->mutable_owner_dependency();
  for (int i = 0; i < dependencies->size(); i++) {
    if (dependencies->Get(i) == owner_dependency) {
      dependencies->SwapElements(i, (dependencies->size() - 1));
      dependencies->RemoveLast();
      break;
    }
  }
  if (dependencies->empty()) {
    local_data->clear_owner_password();
    local_data->clear_endorsement_password();
    local_data->clear_lockout_password();
  }
}

void TpmManagerService::DefineSpace(const DefineSpaceRequest& request,
                                    const DefineSpaceCallback& callback) {
  PostTaskToWorkerThread<DefineSpaceReply>(request, callback,
                                           &TpmManagerService::DefineSpaceTask);
}

void TpmManagerService::DefineSpaceTask(
    const DefineSpaceRequest& request,
    const std::shared_ptr<DefineSpaceReply>& reply) {
  VLOG(1) << __func__;
  std::vector<NvramSpaceAttribute> attributes;
  for (int i = 0; i < request.attributes_size(); ++i) {
    attributes.push_back(request.attributes(i));
  }
  reply->set_result(
      tpm_nvram_->DefineSpace(request.index(), request.size(), attributes,
                              request.authorization_value(), request.policy()));
}

void TpmManagerService::DestroySpace(const DestroySpaceRequest& request,
                                     const DestroySpaceCallback& callback) {
  PostTaskToWorkerThread<DestroySpaceReply>(
      request, callback, &TpmManagerService::DestroySpaceTask);
}

void TpmManagerService::DestroySpaceTask(
    const DestroySpaceRequest& request,
    const std::shared_ptr<DestroySpaceReply>& reply) {
  VLOG(1) << __func__;
  reply->set_result(tpm_nvram_->DestroySpace(request.index()));
}

void TpmManagerService::WriteSpace(const WriteSpaceRequest& request,
                                   const WriteSpaceCallback& callback) {
  PostTaskToWorkerThread<WriteSpaceReply>(request, callback,
                                          &TpmManagerService::WriteSpaceTask);
}

void TpmManagerService::WriteSpaceTask(
    const WriteSpaceRequest& request,
    const std::shared_ptr<WriteSpaceReply>& reply) {
  VLOG(1) << __func__;
  std::string authorization_value = request.authorization_value();
  if (request.use_owner_authorization()) {
    authorization_value = GetOwnerPassword();
    if (authorization_value.empty()) {
      reply->set_result(NVRAM_RESULT_ACCESS_DENIED);
      return;
    }
  }
  reply->set_result(tpm_nvram_->WriteSpace(request.index(), request.data(),
                                           authorization_value));
}

void TpmManagerService::ReadSpace(const ReadSpaceRequest& request,
                                  const ReadSpaceCallback& callback) {
  PostTaskToWorkerThread<ReadSpaceReply>(request, callback,
                                         &TpmManagerService::ReadSpaceTask);
}

void TpmManagerService::ReadSpaceTask(
    const ReadSpaceRequest& request,
    const std::shared_ptr<ReadSpaceReply>& reply) {
  VLOG(1) << __func__;
  std::string authorization_value = request.authorization_value();
  if (request.use_owner_authorization()) {
    authorization_value = GetOwnerPassword();
    if (authorization_value.empty()) {
      reply->set_result(NVRAM_RESULT_ACCESS_DENIED);
      return;
    }
  }
  reply->set_result(tpm_nvram_->ReadSpace(
      request.index(), reply->mutable_data(), authorization_value));
}

void TpmManagerService::LockSpace(const LockSpaceRequest& request,
                                  const LockSpaceCallback& callback) {
  PostTaskToWorkerThread<LockSpaceReply>(request, callback,
                                         &TpmManagerService::LockSpaceTask);
}

void TpmManagerService::LockSpaceTask(
    const LockSpaceRequest& request,
    const std::shared_ptr<LockSpaceReply>& reply) {
  VLOG(1) << __func__;
  std::string authorization_value = request.authorization_value();
  if (request.use_owner_authorization()) {
    authorization_value = GetOwnerPassword();
    if (authorization_value.empty()) {
      reply->set_result(NVRAM_RESULT_ACCESS_DENIED);
      return;
    }
  }
  reply->set_result(tpm_nvram_->LockSpace(request.index(), request.lock_read(),
                                          request.lock_write(),
                                          authorization_value));
}

void TpmManagerService::ListSpaces(const ListSpacesRequest& request,
                                   const ListSpacesCallback& callback) {
  PostTaskToWorkerThread<ListSpacesReply>(request, callback,
                                          &TpmManagerService::ListSpacesTask);
}

void TpmManagerService::ListSpacesTask(
    const ListSpacesRequest& request,
    const std::shared_ptr<ListSpacesReply>& reply) {
  VLOG(1) << __func__;
  std::vector<uint32_t> index_list;
  reply->set_result(tpm_nvram_->ListSpaces(&index_list));
  if (reply->result() == NVRAM_RESULT_SUCCESS) {
    for (auto index : index_list) {
      reply->add_index_list(index);
    }
  }
}

void TpmManagerService::GetSpaceInfo(const GetSpaceInfoRequest& request,
                                     const GetSpaceInfoCallback& callback) {
  PostTaskToWorkerThread<GetSpaceInfoReply>(
      request, callback, &TpmManagerService::GetSpaceInfoTask);
}

void TpmManagerService::GetSpaceInfoTask(
    const GetSpaceInfoRequest& request,
    const std::shared_ptr<GetSpaceInfoReply>& reply) {
  VLOG(1) << __func__;
  std::vector<NvramSpaceAttribute> attributes;
  size_t size = 0;
  bool is_read_locked = false;
  bool is_write_locked = false;
  NvramSpacePolicy policy = NVRAM_POLICY_NONE;
  reply->set_result(tpm_nvram_->GetSpaceInfo(request.index(), &size,
                                             &is_read_locked, &is_write_locked,
                                             &attributes, &policy));
  if (reply->result() == NVRAM_RESULT_SUCCESS) {
    reply->set_size(size);
    reply->set_is_read_locked(is_read_locked);
    reply->set_is_write_locked(is_write_locked);
    for (auto attribute : attributes) {
      reply->add_attributes(attribute);
    }
    reply->set_policy(policy);
  }
}

std::string TpmManagerService::GetOwnerPassword() {
  LocalData local_data;
  if (local_data_store_ && local_data_store_->Read(&local_data)) {
    return local_data.owner_password();
  }
  LOG(ERROR) << "TPM owner password requested but not available.";
  return std::string();
}

template <typename ReplyProtobufType>
void TpmManagerService::TaskRelayCallback(
    const base::Callback<void(const ReplyProtobufType&)> callback,
    const std::shared_ptr<ReplyProtobufType>& reply) {
  callback.Run(*reply);
}

template <typename ReplyProtobufType,
          typename RequestProtobufType,
          typename ReplyCallbackType,
          typename TaskType>
void TpmManagerService::PostTaskToWorkerThread(RequestProtobufType& request,
                                               ReplyCallbackType& callback,
                                               TaskType task) {
  auto result = std::make_shared<ReplyProtobufType>();
  base::Closure background_task =
      base::Bind(task, base::Unretained(this), request, result);
  base::Closure reply =
      base::Bind(&TpmManagerService::TaskRelayCallback<ReplyProtobufType>,
                 weak_factory_.GetWeakPtr(), callback, result);
  worker_thread_->task_runner()->PostTaskAndReply(FROM_HERE, background_task,
                                                  reply);
}

}  // namespace tpm_manager
