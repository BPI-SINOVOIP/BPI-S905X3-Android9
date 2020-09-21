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

#include "tpm_manager/server/tpm2_nvram_impl.h"

#include <memory>
#include <string>

#include <base/logging.h>
#include <trunks/error_codes.h>
#include <trunks/policy_session.h>
#include <trunks/tpm_constants.h>
#include <trunks/tpm_utility.h>

namespace tpm_manager {

using trunks::GetErrorString;
using trunks::TPM_RC;
using trunks::TPM_RC_SUCCESS;

namespace {

void MapAttributesFromTpm(trunks::TPMA_NV tpm_flags,
                          std::vector<NvramSpaceAttribute>* attributes) {
  if (tpm_flags & trunks::TPMA_NV_WRITEDEFINE)
    attributes->push_back(NVRAM_PERSISTENT_WRITE_LOCK);
  if (tpm_flags & trunks::TPMA_NV_WRITE_STCLEAR)
    attributes->push_back(NVRAM_BOOT_WRITE_LOCK);
  if (tpm_flags & trunks::TPMA_NV_READ_STCLEAR)
    attributes->push_back(NVRAM_BOOT_READ_LOCK);
  if (tpm_flags & (trunks::TPMA_NV_AUTHWRITE))
    attributes->push_back(NVRAM_WRITE_AUTHORIZATION);
  if (tpm_flags & (trunks::TPMA_NV_AUTHREAD))
    attributes->push_back(NVRAM_READ_AUTHORIZATION);
  if (tpm_flags & trunks::TPMA_NV_GLOBALLOCK)
    attributes->push_back(NVRAM_GLOBAL_LOCK);
  if (tpm_flags & trunks::TPMA_NV_PPWRITE)
    attributes->push_back(NVRAM_PLATFORM_WRITE);
  if (tpm_flags & trunks::TPMA_NV_OWNERWRITE)
    attributes->push_back(NVRAM_OWNER_WRITE);
  if (tpm_flags & trunks::TPMA_NV_OWNERREAD)
    attributes->push_back(NVRAM_OWNER_READ);
  if (tpm_flags & trunks::TPMA_NV_EXTEND)
    attributes->push_back(NVRAM_WRITE_EXTEND);
}

bool MapAttributesToTpm(
    const std::vector<NvramSpaceAttribute>& attributes,
    trunks::TPMA_NV* tpm_flags,
    bool* world_read_allowed,
    bool* world_write_allowed) {
  // Always require policy, even if it's an empty policy.
  *tpm_flags = trunks::TPMA_NV_POLICYWRITE | trunks::TPMA_NV_POLICYREAD;
  *world_read_allowed = true;
  *world_write_allowed = true;
  for (auto attribute : attributes) {
    switch (attribute) {
      case NVRAM_PERSISTENT_WRITE_LOCK:
        *tpm_flags |= trunks::TPMA_NV_WRITEDEFINE;
        break;
      case NVRAM_BOOT_WRITE_LOCK:
        *tpm_flags |= trunks::TPMA_NV_WRITE_STCLEAR;
        break;
      case NVRAM_BOOT_READ_LOCK:
        *tpm_flags |= trunks::TPMA_NV_READ_STCLEAR;
        break;
      case NVRAM_WRITE_AUTHORIZATION:
        *world_write_allowed = false;
        break;
      case NVRAM_READ_AUTHORIZATION:
        *world_read_allowed = false;
        break;
      case NVRAM_WRITE_EXTEND:
        *tpm_flags |= trunks::TPMA_NV_EXTEND;
        break;
      case NVRAM_GLOBAL_LOCK:
      case NVRAM_PLATFORM_WRITE:
      case NVRAM_OWNER_WRITE:
      case NVRAM_OWNER_READ:
        return false;
      default:
        break;
    }
  }
  return true;
}

NvramResult MapTpmError(TPM_RC tpm_error) {
  switch (trunks::GetFormatOneError(tpm_error)) {
    case trunks::TPM_RC_SUCCESS:
      return NVRAM_RESULT_SUCCESS;
    case trunks::TPM_RC_NV_RANGE:
    case trunks::TPM_RC_NV_SIZE:
    case trunks::TPM_RC_ATTRIBUTES:
      return NVRAM_RESULT_INVALID_PARAMETER;
    case trunks::TPM_RC_NV_LOCKED:
    case trunks::TPM_RC_NV_UNINITIALIZED:
      return NVRAM_RESULT_OPERATION_DISABLED;
    case trunks::TPM_RC_NV_AUTHORIZATION:
    case trunks::TPM_RC_BAD_AUTH:
    case trunks::TPM_RC_AUTH_FAIL:
    case trunks::TPM_RC_POLICY_FAIL:
      return NVRAM_RESULT_ACCESS_DENIED;
    case trunks::TPM_RC_NV_SPACE:
      return NVRAM_RESULT_INSUFFICIENT_SPACE;
    case trunks::TPM_RC_NV_DEFINED:
      return NVRAM_RESULT_SPACE_ALREADY_EXISTS;
    case trunks::TPM_RC_HANDLE:
      return NVRAM_RESULT_SPACE_DOES_NOT_EXIST;
  }
  return NVRAM_RESULT_DEVICE_ERROR;
}

}  // namespace

Tpm2NvramImpl::Tpm2NvramImpl(const trunks::TrunksFactory& factory,
                             LocalDataStore* local_data_store)
    : trunks_factory_(factory),
      local_data_store_(local_data_store),
      initialized_(false),
      trunks_session_(trunks_factory_.GetHmacSession()),
      trunks_utility_(trunks_factory_.GetTpmUtility()) {}

NvramResult Tpm2NvramImpl::DefineSpace(
    uint32_t index,
    size_t size,
    const std::vector<NvramSpaceAttribute>& attributes,
    const std::string& authorization_value,
    NvramSpacePolicy policy) {
  if (!Initialize()) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  if (!SetupOwnerSession()) {
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  trunks::TPMA_NV attribute_flags = 0;
  bool world_read_allowed = false;
  bool world_write_allowed = false;
  if (!MapAttributesToTpm(attributes, &attribute_flags, &world_read_allowed,
                          &world_write_allowed)) {
    return NVRAM_RESULT_INVALID_PARAMETER;
  }
  NvramPolicyRecord policy_record;
  policy_record.set_index(index);
  policy_record.set_policy(policy);
  policy_record.set_world_read_allowed(world_read_allowed);
  policy_record.set_world_write_allowed(world_write_allowed);
  std::string policy_digest;
  if (!ComputePolicyDigest(&policy_record, &policy_digest)) {
    LOG(ERROR) << "Failed to compute policy digest.";
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  TPM_RC result = trunks_utility_->DefineNVSpace(
      index, size, attribute_flags, authorization_value, policy_digest,
      trunks_session_->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error defining nvram space: " << GetErrorString(result);
    return MapTpmError(result);
  }
  if (!SavePolicyRecord(policy_record)) {
    trunks_utility_->DestroyNVSpace(index, trunks_session_->GetDelegate());
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult Tpm2NvramImpl::DestroySpace(uint32_t index) {
  if (!Initialize()) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  if (!SetupOwnerSession()) {
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  TPM_RC result =
      trunks_utility_->DestroyNVSpace(index, trunks_session_->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error destroying nvram space:" << GetErrorString(result);
    return MapTpmError(result);
  }
  DeletePolicyRecord(index);
  return NVRAM_RESULT_SUCCESS;
}

NvramResult Tpm2NvramImpl::WriteSpace(uint32_t index,
                                      const std::string& data,
                                      const std::string& authorization_value) {
  if (!Initialize()) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  trunks::TPMS_NV_PUBLIC nvram_public;
  TPM_RC result = trunks_utility_->GetNVSpacePublicArea(index, &nvram_public);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading nvram space public area: "
               << GetErrorString(result);
    return MapTpmError(result);
  }
  if (nvram_public.attributes & trunks::TPMA_NV_WRITELOCKED) {
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  trunks::AuthorizationDelegate* authorization = nullptr;
  std::unique_ptr<trunks::PolicySession> policy_session =
      trunks_factory_.GetPolicySession();
  bool using_owner_authorization = false;
  bool extend = (nvram_public.attributes & trunks::TPMA_NV_EXTEND) != 0;
  if (nvram_public.attributes & trunks::TPMA_NV_POLICYWRITE) {
    NvramPolicyRecord policy_record;
    if (!GetPolicyRecord(index, &policy_record)) {
      LOG(ERROR) << "Policy record missing.";
      return NVRAM_RESULT_INVALID_PARAMETER;
    }
    if (!SetupPolicySession(
            policy_record, authorization_value,
            extend ? trunks::TPM_CC_NV_Extend : trunks::TPM_CC_NV_Write,
            policy_session.get())) {
      // This will fail if policy is not met, e.g. a PCR value is not the
      // required value.
      return NVRAM_RESULT_ACCESS_DENIED;
    }
    authorization = policy_session->GetDelegate();
  } else if (nvram_public.attributes & trunks::TPMA_NV_AUTHWRITE) {
    trunks_session_->SetEntityAuthorizationValue(authorization_value);
    authorization = trunks_session_->GetDelegate();
  } else if (nvram_public.attributes & trunks::TPMA_NV_OWNERWRITE) {
    if (!SetupOwnerSession()) {
      // The owner password has been destroyed.
      return NVRAM_RESULT_OPERATION_DISABLED;
    }
    using_owner_authorization = true;
    authorization = trunks_session_->GetDelegate();
  } else {
    // TPMA_NV_PPWRITE: Platform authorization is long gone.
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  result = trunks_utility_->WriteNVSpace(index, 0 /* offset */, data,
                                         using_owner_authorization, extend,
                                         authorization);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error writing to nvram space: " << GetErrorString(result);
    return MapTpmError(result);
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult Tpm2NvramImpl::ReadSpace(uint32_t index,
                                     std::string* data,
                                     const std::string& authorization_value) {
  if (!Initialize()) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  trunks::TPMS_NV_PUBLIC nvram_public;
  TPM_RC result = trunks_utility_->GetNVSpacePublicArea(index, &nvram_public);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading nvram space public area: "
               << GetErrorString(result);
    return MapTpmError(result);
  }
  if (nvram_public.attributes & trunks::TPMA_NV_READLOCKED) {
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  // Handle the case when the space has never been written to.
  if ((nvram_public.attributes & trunks::TPMA_NV_WRITTEN) == 0) {
    *data = std::string(nvram_public.data_size, 0);
    return NVRAM_RESULT_SUCCESS;
  }
  trunks::AuthorizationDelegate* authorization = nullptr;
  std::unique_ptr<trunks::PolicySession> policy_session =
      trunks_factory_.GetPolicySession();
  bool using_owner_authorization = false;
  if (nvram_public.attributes & trunks::TPMA_NV_POLICYREAD) {
    NvramPolicyRecord policy_record;
    if (!GetPolicyRecord(index, &policy_record)) {
      LOG(ERROR) << "Policy record missing.";
      return NVRAM_RESULT_INVALID_PARAMETER;
    }
    if (!SetupPolicySession(policy_record, authorization_value,
                            trunks::TPM_CC_NV_Read, policy_session.get())) {
      // This will fail if policy is not met, e.g. a PCR value is not the
      // required value.
      return NVRAM_RESULT_ACCESS_DENIED;
    }
    authorization = policy_session->GetDelegate();
  } else if (nvram_public.attributes & trunks::TPMA_NV_AUTHREAD) {
    trunks_session_->SetEntityAuthorizationValue(authorization_value);
    authorization = trunks_session_->GetDelegate();
  } else if (nvram_public.attributes & trunks::TPMA_NV_OWNERREAD) {
    if (!SetupOwnerSession()) {
      // The owner password has been destroyed.
      return NVRAM_RESULT_OPERATION_DISABLED;
    }
    using_owner_authorization = true;
    authorization = trunks_session_->GetDelegate();
  } else {
    // TPMA_NV_PPREAD: Platform authorization is long gone.
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  result = trunks_utility_->ReadNVSpace(
      index, 0 /* offset */, nvram_public.data_size, using_owner_authorization,
      data, authorization);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading nvram space: " << GetErrorString(result);
    return MapTpmError(result);
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult Tpm2NvramImpl::LockSpace(uint32_t index,
                                     bool lock_read,
                                     bool lock_write,
                                     const std::string& authorization_value) {
  if (!Initialize()) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  trunks::TPMS_NV_PUBLIC nvram_public;
  TPM_RC result = trunks_utility_->GetNVSpacePublicArea(index, &nvram_public);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading nvram space public area: "
               << GetErrorString(result);
    return MapTpmError(result);
  }
  bool is_read_locked =
      ((nvram_public.attributes & trunks::TPMA_NV_READLOCKED) != 0);
  bool is_write_locked =
      ((nvram_public.attributes & trunks::TPMA_NV_WRITELOCKED) != 0);
  if ((!lock_read || is_read_locked) && (!lock_write || is_write_locked)) {
    // Already locked.
    return NVRAM_RESULT_SUCCESS;
  }
  // Handle locking read and write separately because the authorization might be
  // different.
  if (lock_read && !is_read_locked) {
    trunks::AuthorizationDelegate* authorization = nullptr;
    std::unique_ptr<trunks::PolicySession> policy_session =
        trunks_factory_.GetPolicySession();
    bool using_owner_authorization = false;
    if (nvram_public.attributes & trunks::TPMA_NV_POLICYREAD) {
      NvramPolicyRecord policy_record;
      if (!GetPolicyRecord(index, &policy_record)) {
        LOG(ERROR) << "Policy record missing.";
        return NVRAM_RESULT_INVALID_PARAMETER;
      }
      if (!SetupPolicySession(policy_record, authorization_value,
                              trunks::TPM_CC_NV_ReadLock,
                              policy_session.get())) {
        // This will fail if policy is not met, e.g. a PCR value is not the
        // required value.
        return NVRAM_RESULT_ACCESS_DENIED;
      }
      authorization = policy_session->GetDelegate();
    } else if (nvram_public.attributes & trunks::TPMA_NV_AUTHREAD) {
      trunks_session_->SetEntityAuthorizationValue(authorization_value);
      authorization = trunks_session_->GetDelegate();
    } else if (nvram_public.attributes & trunks::TPMA_NV_OWNERREAD) {
      if (!SetupOwnerSession()) {
        // The owner password has been destroyed.
        return NVRAM_RESULT_OPERATION_DISABLED;
      }
      using_owner_authorization = true;
      authorization = trunks_session_->GetDelegate();
    } else {
      // TPMA_NV_PPREAD: Platform authorization is long gone.
      return NVRAM_RESULT_OPERATION_DISABLED;
    }
    result = trunks_utility_->LockNVSpace(
        index, true /* lock_read */, false /* lock_write */,
        using_owner_authorization, authorization);
    if (result != TPM_RC_SUCCESS) {
      LOG(ERROR) << "Error locking nvram space: " << GetErrorString(result);
      return MapTpmError(result);
    }
  }
  if (lock_write && !is_write_locked) {
    trunks::AuthorizationDelegate* authorization = nullptr;
    std::unique_ptr<trunks::PolicySession> policy_session =
        trunks_factory_.GetPolicySession();
    bool using_owner_authorization = false;
    if (nvram_public.attributes & trunks::TPMA_NV_POLICYWRITE) {
      NvramPolicyRecord policy_record;
      if (!GetPolicyRecord(index, &policy_record)) {
        LOG(ERROR) << "Policy record missing.";
        return NVRAM_RESULT_INVALID_PARAMETER;
      }
      if (!SetupPolicySession(policy_record, authorization_value,
                              trunks::TPM_CC_NV_WriteLock,
                              policy_session.get())) {
        // This will fail if policy is not met, e.g. a PCR value is not the
        // required value.
        return NVRAM_RESULT_ACCESS_DENIED;
      }
      authorization = policy_session->GetDelegate();
    } else if (nvram_public.attributes & trunks::TPMA_NV_AUTHWRITE) {
      trunks_session_->SetEntityAuthorizationValue(authorization_value);
      authorization = trunks_session_->GetDelegate();
    } else if (nvram_public.attributes & trunks::TPMA_NV_OWNERWRITE) {
      if (!SetupOwnerSession()) {
        // The owner password has been destroyed.
        return NVRAM_RESULT_OPERATION_DISABLED;
      }
      using_owner_authorization = true;
      authorization = trunks_session_->GetDelegate();
    } else {
      // TPMA_NV_PPWRITE: Platform authorization is long gone.
      return NVRAM_RESULT_OPERATION_DISABLED;
    }
    result = trunks_utility_->LockNVSpace(
        index, false /* lock_read */, true /* lock_write */,
        using_owner_authorization, authorization);
    if (result != TPM_RC_SUCCESS) {
      LOG(ERROR) << "Error locking nvram space: " << GetErrorString(result);
      return MapTpmError(result);
    }
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult Tpm2NvramImpl::ListSpaces(std::vector<uint32_t>* index_list) {
  return MapTpmError(trunks_utility_->ListNVSpaces(index_list));
}

NvramResult Tpm2NvramImpl::GetSpaceInfo(
    uint32_t index,
    size_t* size,
    bool* is_read_locked,
    bool* is_write_locked,
    std::vector<NvramSpaceAttribute>* attributes,
    NvramSpacePolicy* policy) {
  trunks::TPMS_NV_PUBLIC nvram_public;
  TPM_RC result = trunks_utility_->GetNVSpacePublicArea(index, &nvram_public);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading NV space for index " << index
               << " with error: " << GetErrorString(result);
    return MapTpmError(result);
  }
  *size = nvram_public.data_size;
  *is_read_locked =
      ((nvram_public.attributes & trunks::TPMA_NV_READLOCKED) != 0);
  *is_write_locked =
      ((nvram_public.attributes & trunks::TPMA_NV_WRITELOCKED) != 0);
  MapAttributesFromTpm(nvram_public.attributes, attributes);
  *policy = NVRAM_POLICY_NONE;
  NvramPolicyRecord policy_record;
  if (GetPolicyRecord(index, &policy_record)) {
    *policy = policy_record.policy();
    if (!policy_record.world_read_allowed()) {
      attributes->push_back(NVRAM_READ_AUTHORIZATION);
    }
    if (!policy_record.world_write_allowed()) {
      attributes->push_back(NVRAM_WRITE_AUTHORIZATION);
    }
  }
  return NVRAM_RESULT_SUCCESS;
}

bool Tpm2NvramImpl::Initialize() {
  if (initialized_) {
    return true;
  }
  TPM_RC result =
      trunks_session_->StartUnboundSession(true /* enable_encryption */);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting a default authorization session: "
               << GetErrorString(result);
    return false;
  }
  initialized_ = true;
  return true;
}

std::string Tpm2NvramImpl::GetOwnerPassword() {
  LocalData local_data;
  if (local_data_store_ && local_data_store_->Read(&local_data)) {
    return local_data.owner_password();
  }
  LOG(ERROR) << "TPM owner password requested but not available.";
  return std::string();
}

bool Tpm2NvramImpl::SetupOwnerSession() {
  std::string owner_password = GetOwnerPassword();
  if (owner_password.empty()) {
    LOG(ERROR) << "Owner authorization required but not available.";
    return false;
  }
  trunks_session_->SetEntityAuthorizationValue(owner_password);
  return true;
}

bool Tpm2NvramImpl::SetupPolicySession(
    const NvramPolicyRecord& policy_record,
    const std::string& authorization_value,
    trunks::TPM_CC command_code,
    trunks::PolicySession* session) {
  TPM_RC result = session->StartUnboundSession(true /* enable_encryption */);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting a policy authorization session: "
               << GetErrorString(result);
    return false;
  }
  session->SetEntityAuthorizationValue(authorization_value);
  if (!AddPoliciesForCommand(policy_record, command_code, session)) {
    return false;
  }
  if (!AddPolicyOR(policy_record, session)) {
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::AddPoliciesForCommand(
    const NvramPolicyRecord& policy_record,
    trunks::TPM_CC command_code,
    trunks::PolicySession* session) {
  TPM_RC result = session->PolicyCommandCode(command_code);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Failed to setup command code policy.";
    return false;
  }
  bool is_write_command = (command_code == trunks::TPM_CC_NV_Write ||
                           command_code == trunks::TPM_CC_NV_WriteLock ||
                           command_code == trunks::TPM_CC_NV_Extend);
  bool is_read_command = !is_write_command;
  // Check if this operation requires an authorization value.
  if ((is_read_command && !policy_record.world_read_allowed()) ||
      (is_write_command && !policy_record.world_write_allowed())) {
    result = session->PolicyAuthValue();
    if (result != TPM_RC_SUCCESS) {
      LOG(ERROR) << "Failed to setup auth policy.";
      return false;
    }
  }
  if (policy_record.policy() == NVRAM_POLICY_PCR0) {
    std::string current_pcr_value;
    result = trunks_utility_->ReadPCR(0, &current_pcr_value);
    if (result != TPM_RC_SUCCESS) {
      LOG(ERROR) << "Failed to read the current PCR value.";
      return false;
    }
    result = session->PolicyPCR(0, current_pcr_value);
    if (result != TPM_RC_SUCCESS) {
      LOG(ERROR) << "Failed to setup PCR policy.";
      return false;
    }
  }
  return true;
}

bool Tpm2NvramImpl::AddPolicyOR(
    const NvramPolicyRecord& policy_record,
    trunks::PolicySession* session) {
  std::vector<std::string> digests;
  for (int i = 0; i < policy_record.policy_digests_size(); ++i) {
    digests.push_back(policy_record.policy_digests(i));
  }
  TPM_RC result = session->PolicyOR(digests);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Failed to setup OR policy.";
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::ComputePolicyDigest(NvramPolicyRecord* policy_record,
                                        std::string* digest) {
  // Compute a policy digest for each command then OR them all together. This
  // approach gives flexibility to have different requirements for read and
  // write operations, and the ability to support authorization values combined
  // with other policies.
  std::unique_ptr<trunks::PolicySession> trial_session;
  for (trunks::TPM_CC command_code :
       {trunks::TPM_CC_NV_Extend, trunks::TPM_CC_NV_Write,
        trunks::TPM_CC_NV_WriteLock, trunks::TPM_CC_NV_Read,
        trunks::TPM_CC_NV_ReadLock, trunks::TPM_CC_NV_Certify}) {
    trial_session = trunks_factory_.GetTrialSession();
    if (trial_session->StartUnboundSession(false /* enable_encryption */) !=
        TPM_RC_SUCCESS) {
      return false;
    }
    if (!AddPoliciesForCommand(*policy_record, command_code,
                               trial_session.get())) {
      return false;
    }
    if (trial_session->GetDigest(digest) != TPM_RC_SUCCESS) {
      return false;
    }
    policy_record->add_policy_digests(*digest);
  }
  if (!AddPolicyOR(*policy_record, trial_session.get())) {
    return false;
  }
  if (trial_session->GetDigest(digest) != TPM_RC_SUCCESS) {
    return false;
  }
  return true;
}

bool Tpm2NvramImpl::GetPolicyRecord(uint32_t index, NvramPolicyRecord* record) {
  LocalData local_data;
  if (local_data_store_ && local_data_store_->Read(&local_data)) {
    for (int i = 0; i < local_data.nvram_policy_size(); ++i) {
      if (local_data.nvram_policy(i).index() == index) {
        *record = local_data.nvram_policy(i);
        return true;
      }
    }
  }
  return false;
}

bool Tpm2NvramImpl::SavePolicyRecord(const NvramPolicyRecord& record) {
  LocalData local_data;
  if (!local_data_store_ || !local_data_store_->Read(&local_data)) {
    LOG(ERROR) << "Failed to read local data.";
    return false;
  }
  LocalData new_local_data = local_data;
  new_local_data.clear_nvram_policy();
  for (int i = 0; i < local_data.nvram_policy_size(); ++i) {
    // Keep only the ones that don't match |record|.
    if (local_data.nvram_policy(i).index() != record.index()) {
      *new_local_data.add_nvram_policy() = local_data.nvram_policy(i);
    }
  }
  *new_local_data.add_nvram_policy() = record;
  if (!local_data_store_->Write(new_local_data)) {
    LOG(ERROR) << "Failed to write local data.";
    return false;
  }
  return true;
}

void Tpm2NvramImpl::DeletePolicyRecord(uint32_t index) {
  LocalData local_data;
  if (local_data_store_ && local_data_store_->Read(&local_data)) {
    LocalData new_local_data = local_data;
    new_local_data.clear_nvram_policy();
    for (int i = 0; i < local_data.nvram_policy_size(); ++i) {
      // Keep only the ones that don't match |index|.
      if (local_data.nvram_policy(i).index() != index) {
        *new_local_data.add_nvram_policy() = local_data.nvram_policy(i);
      }
    }
    local_data_store_->Write(new_local_data);
  }
}

}  // namespace tpm_manager
