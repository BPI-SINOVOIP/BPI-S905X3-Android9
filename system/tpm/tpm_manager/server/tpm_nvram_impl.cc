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

#include "tpm_manager/server/tpm_nvram_impl.h"

#include <arpa/inet.h>

#include <string>

#include <base/logging.h>
#include <base/stl_util.h>
#include <trousers/scoped_tss_type.h>

#include "tpm_manager/common/tpm_manager.pb.h"
#include "tpm_manager/server/local_data_store.h"
#include "tpm_manager/server/tpm_util.h"

namespace tpm_manager {

using trousers::ScopedTssMemory;
using trousers::ScopedTssNvStore;
using trousers::ScopedTssPcrs;

namespace {

// PCR0 at locality 1 is used to differentiate between developed and normal
// mode. Restricting nvram to the PCR0 value in locality 1 prevents nvram from
// persisting across mode switch.
const unsigned int kTpmBootPCR = 0;
const unsigned int kTpmPCRLocality = 1;

void MapAttributesFromTpm(TPM_NV_PER_ATTRIBUTES tpm_flags,
                          std::vector<NvramSpaceAttribute>* attributes) {
  if (tpm_flags & TPM_NV_PER_WRITEDEFINE)
    attributes->push_back(NVRAM_PERSISTENT_WRITE_LOCK);
  if (tpm_flags & TPM_NV_PER_WRITE_STCLEAR)
    attributes->push_back(NVRAM_BOOT_WRITE_LOCK);
  if (tpm_flags & TPM_NV_PER_READ_STCLEAR)
    attributes->push_back(NVRAM_BOOT_READ_LOCK);
  if (tpm_flags & TPM_NV_PER_AUTHWRITE)
    attributes->push_back(NVRAM_WRITE_AUTHORIZATION);
  if (tpm_flags & TPM_NV_PER_AUTHREAD)
    attributes->push_back(NVRAM_READ_AUTHORIZATION);
  if (tpm_flags & TPM_NV_PER_GLOBALLOCK)
    attributes->push_back(NVRAM_GLOBAL_LOCK);
  if (tpm_flags & TPM_NV_PER_PPWRITE)
    attributes->push_back(NVRAM_PLATFORM_WRITE);
  if (tpm_flags & TPM_NV_PER_OWNERWRITE)
    attributes->push_back(NVRAM_OWNER_WRITE);
  if (tpm_flags & TPM_NV_PER_OWNERREAD)
    attributes->push_back(NVRAM_OWNER_READ);
}

TPM_NV_PER_ATTRIBUTES MapAttributesToTpm(
    const std::vector<NvramSpaceAttribute>& attributes) {
  TPM_NV_PER_ATTRIBUTES tpm_flags = 0;
  for (auto attribute : attributes) {
    switch (attribute) {
      case NVRAM_PERSISTENT_WRITE_LOCK:
        tpm_flags |= TPM_NV_PER_WRITEDEFINE;
        break;
      case NVRAM_BOOT_WRITE_LOCK:
        tpm_flags |= TPM_NV_PER_WRITE_STCLEAR;
        break;
      case NVRAM_BOOT_READ_LOCK:
        tpm_flags |= TPM_NV_PER_READ_STCLEAR;
        break;
      case NVRAM_WRITE_AUTHORIZATION:
        tpm_flags |= TPM_NV_PER_AUTHWRITE;
        break;
      case NVRAM_READ_AUTHORIZATION:
        tpm_flags |= TPM_NV_PER_AUTHREAD;
        break;
      case NVRAM_GLOBAL_LOCK:
        tpm_flags |= TPM_NV_PER_GLOBALLOCK;
        break;
      case NVRAM_PLATFORM_WRITE:
        tpm_flags |= TPM_NV_PER_PPWRITE;
        break;
      case NVRAM_OWNER_WRITE:
        tpm_flags |= TPM_NV_PER_OWNERWRITE;
        break;
      case NVRAM_OWNER_READ:
        tpm_flags |= TPM_NV_PER_OWNERREAD;
        break;
      default:
        break;
    }
  }
  return tpm_flags;
}

NvramResult MapTpmError(TSS_RESULT tpm_error) {
  switch (TPM_ERROR(tpm_error)) {
    case TPM_SUCCESS:
      return NVRAM_RESULT_SUCCESS;
    case TPM_E_BAD_PARAMETER:
    case TPM_E_PER_NOWRITE:
    case TPM_E_AUTH_CONFLICT:
      return NVRAM_RESULT_INVALID_PARAMETER;
    case TPM_E_AREA_LOCKED:
    case TPM_E_READ_ONLY:
    case TPM_E_WRITE_LOCKED:
    case TPM_E_DISABLED_CMD:
      return NVRAM_RESULT_OPERATION_DISABLED;
    case TPM_E_AUTHFAIL:
    case TPM_E_NO_NV_PERMISSION:
    case TPM_E_WRONGPCRVAL:
      return NVRAM_RESULT_ACCESS_DENIED;
    case TPM_E_NOSPACE:
    case TPM_E_RESOURCES:
    case TPM_E_SIZE:
      return NVRAM_RESULT_INSUFFICIENT_SPACE;
    case TPM_E_BADINDEX:
    case TPM_E_BAD_HANDLE:
      return NVRAM_RESULT_SPACE_DOES_NOT_EXIST;
  }
  return NVRAM_RESULT_DEVICE_ERROR;
}

}  // namespace

TpmNvramImpl::TpmNvramImpl(LocalDataStore* local_data_store)
    : local_data_store_(local_data_store) {}

NvramResult TpmNvramImpl::DefineSpace(
    uint32_t index,
    size_t size,
    const std::vector<NvramSpaceAttribute>& attributes,
    const std::string& authorization_value,
    NvramSpacePolicy policy) {
  std::string owner_password;
  if (!GetOwnerPassword(&owner_password)) {
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  TpmConnection owner_connection(owner_password);
  ScopedTssNvStore nv_handle(owner_connection.GetContext());
  if (!InitializeNvramHandle(index, &nv_handle, &owner_connection)) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  TSS_RESULT result;
  result = Tspi_SetAttribUint32(nv_handle, TSS_TSPATTRIB_NV_DATASIZE, 0, size);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set size on NVRAM object: " << size;
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  // Set permissions attributes.
  result = Tspi_SetAttribUint32(nv_handle, TSS_TSPATTRIB_NV_PERMISSIONS, 0,
                                MapAttributesToTpm(attributes));
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set permissions on NVRAM object";
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  // Set authorization.
  if (!authorization_value.empty() &&
      !SetUsagePolicy(authorization_value, &nv_handle, &owner_connection)) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  // Bind to PCR0.
  TSS_HPCRS pcr_handle = 0;
  ScopedTssPcrs scoped_pcr_handle(owner_connection.GetContext());
  if (policy == NVRAM_POLICY_PCR0) {
    if (!SetCompositePcr0(&scoped_pcr_handle, &owner_connection)) {
      return NVRAM_RESULT_DEVICE_ERROR;
    }
    pcr_handle = scoped_pcr_handle;
  }
  result = Tspi_NV_DefineSpace(nv_handle, pcr_handle, /*Read*/
                               pcr_handle /*Write*/);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not define NVRAM space: " << index;
    return MapTpmError(result);
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult TpmNvramImpl::DestroySpace(uint32_t index) {
  std::string owner_password;
  if (!GetOwnerPassword(&owner_password)) {
    return NVRAM_RESULT_OPERATION_DISABLED;
  }
  TpmConnection owner_connection(owner_password);
  ScopedTssNvStore nv_handle(owner_connection.GetContext());
  if (!InitializeNvramHandle(index, &nv_handle, &owner_connection)) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  TSS_RESULT result = Tspi_NV_ReleaseSpace(nv_handle);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not release NVRAM space: " << index;
    return MapTpmError(result);
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult TpmNvramImpl::WriteSpace(uint32_t index,
                                     const std::string& data,
                                     const std::string& authorization_value) {
  std::vector<NvramSpaceAttribute> attributes;
  NvramResult result =
      GetSpaceInfo(index, nullptr, nullptr, nullptr, &attributes, nullptr);
  if (result != NVRAM_RESULT_SUCCESS) {
    return result;
  }
  ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
  if (!InitializeNvramHandle(index, &nv_handle, &tpm_connection_)) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  for (const auto& attribute : attributes) {
    if (attribute == NVRAM_OWNER_WRITE) {
      if (!SetOwnerPolicy(&nv_handle)) {
        return NVRAM_RESULT_OPERATION_DISABLED;
      }
      break;
    }
    if (attribute == NVRAM_WRITE_AUTHORIZATION) {
      if (!SetUsagePolicy(authorization_value, &nv_handle, &tpm_connection_)) {
        return NVRAM_RESULT_DEVICE_ERROR;
      }
      break;
    }
  }
  TSS_RESULT tpm_result = Tspi_NV_WriteValue(
      nv_handle, 0 /* offset */, data.size(),
      reinterpret_cast<BYTE*>(const_cast<char*>(data.data())));
  if (TPM_ERROR(tpm_result)) {
    TPM_LOG(ERROR, tpm_result) << "Could not write to NVRAM space: " << index;
    return MapTpmError(tpm_result);
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult TpmNvramImpl::ReadSpace(uint32_t index,
                                    std::string* data,
                                    const std::string& authorization_value) {
  CHECK(data);
  size_t nvram_size;
  std::vector<NvramSpaceAttribute> attributes;
  NvramResult result =
      GetSpaceInfo(index, &nvram_size, nullptr, nullptr, &attributes, nullptr);
  if (result != NVRAM_RESULT_SUCCESS) {
    return result;
  }
  ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
  if (!InitializeNvramHandle(index, &nv_handle, &tpm_connection_)) {
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  for (const auto& attribute : attributes) {
    if (attribute == NVRAM_OWNER_READ) {
      if (!SetOwnerPolicy(&nv_handle)) {
        return NVRAM_RESULT_OPERATION_DISABLED;
      }
      break;
    }
    if (attribute == NVRAM_READ_AUTHORIZATION) {
      if (!SetUsagePolicy(authorization_value, &nv_handle, &tpm_connection_)) {
        return NVRAM_RESULT_DEVICE_ERROR;
      }
      break;
    }
  }
  data->resize(nvram_size);
  // The Tpm1.2 Specification defines the maximum read size of 128 bytes.
  // Therefore we have to loop through the data returned.
  const size_t kMaxDataSize = 128;
  uint32_t offset = 0;
  while (offset < nvram_size) {
    uint32_t chunk_size = std::min(nvram_size - offset, kMaxDataSize);
    ScopedTssMemory space_data(tpm_connection_.GetContext());
    TSS_RESULT tpm_result =
        Tspi_NV_ReadValue(nv_handle, offset, &chunk_size, space_data.ptr());
    if (TPM_ERROR(tpm_result)) {
      TPM_LOG(ERROR, tpm_result) << "Could not read from NVRAM space: "
                                 << index;
      data->clear();
      return MapTpmError(tpm_result);
    }
    if (!space_data.value()) {
      LOG(ERROR) << "No data read from NVRAM space: " << index;
      data->clear();
      return NVRAM_RESULT_DEVICE_ERROR;
    }
    CHECK_LE((offset + chunk_size), data->size());
    data->replace(offset, chunk_size,
                  reinterpret_cast<char*>(space_data.value()), chunk_size);
    offset += chunk_size;
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult TpmNvramImpl::LockSpace(uint32_t index,
                                    bool lock_read,
                                    bool lock_write,
                                    const std::string& authorization_value) {
  std::vector<NvramSpaceAttribute> attributes;
  NvramResult result =
      GetSpaceInfo(index, nullptr, nullptr, nullptr, &attributes, nullptr);
  if (result != NVRAM_RESULT_SUCCESS) {
    return result;
  }
  if (lock_read) {
    ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
    if (!InitializeNvramHandle(index, &nv_handle, &tpm_connection_)) {
      return NVRAM_RESULT_DEVICE_ERROR;
    }
    for (const auto& attribute : attributes) {
      if (attribute == NVRAM_OWNER_READ) {
        if (!SetOwnerPolicy(&nv_handle)) {
          return NVRAM_RESULT_OPERATION_DISABLED;
        }
        break;
      }
      if (attribute == NVRAM_READ_AUTHORIZATION) {
        if (!SetUsagePolicy(authorization_value, &nv_handle,
                            &tpm_connection_)) {
          return NVRAM_RESULT_DEVICE_ERROR;
        }
        break;
      }
    }
    uint32_t size = 0;
    ScopedTssMemory space_data(tpm_connection_.GetContext());
    TSS_RESULT tpm_result =
        Tspi_NV_ReadValue(nv_handle, 0, &size, space_data.ptr());
    if (TPM_ERROR(tpm_result)) {
      TPM_LOG(ERROR, tpm_result) << "Could not lock read for NVRAM space: "
                                 << index;
      return MapTpmError(tpm_result);
    }
  }
  if (lock_write) {
    ScopedTssNvStore nv_handle(tpm_connection_.GetContext());
    if (!InitializeNvramHandle(index, &nv_handle, &tpm_connection_)) {
      return NVRAM_RESULT_DEVICE_ERROR;
    }
    for (const auto& attribute : attributes) {
      if (attribute == NVRAM_OWNER_WRITE) {
        if (!SetOwnerPolicy(&nv_handle)) {
          return NVRAM_RESULT_OPERATION_DISABLED;
        }
        break;
      }
      if (attribute == NVRAM_WRITE_AUTHORIZATION) {
        if (!SetUsagePolicy(authorization_value, &nv_handle,
                            &tpm_connection_)) {
          return NVRAM_RESULT_DEVICE_ERROR;
        }
        break;
      }
    }
    BYTE not_used;
    TSS_RESULT tpm_result = Tspi_NV_WriteValue(nv_handle, 0, 0, &not_used);
    if (TPM_ERROR(tpm_result)) {
      TPM_LOG(ERROR, tpm_result) << "Could not lock write for NVRAM space: "
                                 << index;
      return MapTpmError(tpm_result);
    }
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult TpmNvramImpl::ListSpaces(std::vector<uint32_t>* index_list) {
  uint32_t nv_list_data_length = 0;
  ScopedTssMemory nv_list_data(tpm_connection_.GetContext());
  TSS_RESULT result =
      Tspi_TPM_GetCapability(tpm_connection_.GetTpm(), TSS_TPMCAP_NV_LIST, 0,
                             nullptr, &nv_list_data_length, nv_list_data.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_GetCapability";
    return MapTpmError(result);
  }
  // Walk the list and check if the index exists.
  uint32_t* nv_list = reinterpret_cast<uint32_t*>(nv_list_data.value());
  uint32_t nv_list_length = nv_list_data_length / sizeof(uint32_t);
  for (uint32_t i = 0; i < nv_list_length; ++i) {
    // TPM data is network byte order.
    index_list->push_back(ntohl(nv_list[i]));
  }
  return NVRAM_RESULT_SUCCESS;
}

NvramResult TpmNvramImpl::GetSpaceInfo(
    uint32_t index,
    size_t* size,
    bool* is_read_locked,
    bool* is_write_locked,
    std::vector<NvramSpaceAttribute>* attributes,
    NvramSpacePolicy* policy) {
  UINT32 nv_index_data_length = 0;
  ScopedTssMemory nv_index_data(tpm_connection_.GetContext());
  TSS_RESULT result =
      Tspi_TPM_GetCapability(tpm_connection_.GetTpm(), TSS_TPMCAP_NV_INDEX,
                             sizeof(index), reinterpret_cast<BYTE*>(&index),
                             &nv_index_data_length, nv_index_data.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_TPM_GetCapability";
    return MapTpmError(result);
  }
  UINT64 offset = 0;
  Trspi_UnloadBlob_NV_DATA_PUBLIC(&offset, nv_index_data.value(), nullptr);
  if (nv_index_data_length < offset) {
    LOG(ERROR) << "Not enough data from Tspi_TPM_GetCapability.";
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  TPM_NV_DATA_PUBLIC info;
  offset = 0;
  result =
      Trspi_UnloadBlob_NV_DATA_PUBLIC(&offset, nv_index_data.value(), &info);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Trspi_UnloadBlob_NV_DATA_PUBLIC";
    return NVRAM_RESULT_DEVICE_ERROR;
  }
  if (size) {
    *size = info.dataSize;
  }
  if (is_read_locked) {
    *is_read_locked = info.bReadSTClear;
  }
  if (is_write_locked) {
    *is_write_locked = info.bWriteSTClear || info.bWriteDefine;
  }
  if (attributes) {
    MapAttributesFromTpm(info.permission.attributes, attributes);
  }
  if (policy) {
    if (info.pcrInfoWrite.pcrSelection.sizeOfSelect > 0 &&
        (info.pcrInfoWrite.pcrSelection.pcrSelect[0] & 1) != 0) {
      *policy = NVRAM_POLICY_PCR0;
    } else {
      *policy = NVRAM_POLICY_NONE;
    }
  }
  return NVRAM_RESULT_SUCCESS;
}

bool TpmNvramImpl::InitializeNvramHandle(uint32_t index,
                                         ScopedTssNvStore* nv_handle,
                                         TpmConnection* connection) {
  TSS_RESULT result = Tspi_Context_CreateObject(
      connection->GetContext(), TSS_OBJECT_TYPE_NV, 0, nv_handle->ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not acquire an NVRAM object handle";
    return false;
  }
  result = Tspi_SetAttribUint32(nv_handle->value(), TSS_TSPATTRIB_NV_INDEX, 0,
                                index);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set index on NVRAM object: " << index;
    return false;
  }
  return true;
}

bool TpmNvramImpl::SetOwnerPolicy(ScopedTssNvStore* nv_handle) {
  std::string owner_password;
  if (!GetOwnerPassword(&owner_password)) {
    return false;
  }
  return SetUsagePolicy(owner_password, nv_handle, &tpm_connection_);
}

bool TpmNvramImpl::SetUsagePolicy(const std::string& authorization_value,
                                  trousers::ScopedTssNvStore* nv_handle,
                                  TpmConnection* connection) {
  trousers::ScopedTssPolicy policy_handle(connection->GetContext());
  TSS_RESULT result;
  result = Tspi_Context_CreateObject(connection->GetContext(),
                                     TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
                                     policy_handle.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Context_CreateObject";
    return false;
  }
  result = Tspi_Policy_SetSecret(
      policy_handle, TSS_SECRET_MODE_PLAIN, authorization_value.size(),
      reinterpret_cast<BYTE*>(const_cast<char*>(authorization_value.data())));
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Error calling Tspi_Policy_SetSecret";
    return false;
  }
  result =
      Tspi_Policy_AssignToObject(policy_handle.value(), nv_handle->value());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set NVRAM object policy.";
    return false;
  }
  return true;
}

bool TpmNvramImpl::SetCompositePcr0(ScopedTssPcrs* pcr_handle,
                                    TpmConnection* connection) {
  TSS_RESULT result = Tspi_Context_CreateObject(
      connection->GetContext(), TSS_OBJECT_TYPE_PCRS,
      TSS_PCRS_STRUCT_INFO_SHORT, pcr_handle->ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not acquire PCR object handle";
    return false;
  }
  uint32_t pcr_len;
  ScopedTssMemory pcr_value(connection->GetContext());
  result = Tspi_TPM_PcrRead(connection->GetTpm(), kTpmBootPCR, &pcr_len,
                            pcr_value.ptr());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not read PCR0 value";
    return false;
  }
  result = Tspi_PcrComposite_SetPcrValue(pcr_handle->value(), kTpmBootPCR,
                                         pcr_len, pcr_value.value());
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set value for PCR0 in PCR handle";
    return false;
  }
  result =
      Tspi_PcrComposite_SetPcrLocality(pcr_handle->value(), kTpmPCRLocality);
  if (TPM_ERROR(result)) {
    TPM_LOG(ERROR, result) << "Could not set locality for PCR0 in PCR handle";
    return false;
  }
  return true;
}

bool TpmNvramImpl::GetOwnerPassword(std::string* owner_password) {
  LocalData local_data;
  if (!local_data_store_->Read(&local_data)) {
    LOG(ERROR) << "Error reading local data for owner password.";
    return false;
  }
  if (local_data.owner_password().empty()) {
    LOG(ERROR) << "No owner password present in tpm local_data.";
    return false;
  }
  owner_password->assign(local_data.owner_password());
  return true;
}

}  // namespace tpm_manager
