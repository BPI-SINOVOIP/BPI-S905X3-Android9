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

#ifndef TPM_MANAGER_SERVER_TPM2_NVRAM_IMPL_H_
#define TPM_MANAGER_SERVER_TPM2_NVRAM_IMPL_H_

#include "tpm_manager/server/tpm_nvram.h"

#include <memory>
#include <string>

#include <base/macros.h>
#include <trunks/trunks_factory.h>

#include "tpm_manager/common/tpm_manager.pb.h"
#include "tpm_manager/server/local_data_store.h"

namespace tpm_manager {

// A TpmNvram implementation backed by a TPM 2.0 device. All index values are
// the 'index' portion of an NV handle and must fit in 24 bits.
class Tpm2NvramImpl : public TpmNvram {
 public:
  // Does not take ownership of arguments.
  Tpm2NvramImpl(const trunks::TrunksFactory& factory,
                LocalDataStore* local_data_store);
  ~Tpm2NvramImpl() override = default;

  // TpmNvram methods.
  NvramResult DefineSpace(uint32_t index,
                          size_t size,
                          const std::vector<NvramSpaceAttribute>& attributes,
                          const std::string& authorization_value,
                          NvramSpacePolicy policy) override;
  NvramResult DestroySpace(uint32_t index) override;
  NvramResult WriteSpace(uint32_t index,
                         const std::string& data,
                         const std::string& authorization_value) override;
  NvramResult ReadSpace(uint32_t index,
                        std::string* data,
                        const std::string& authorization_value) override;
  NvramResult LockSpace(uint32_t index,
                        bool lock_read,
                        bool lock_write,
                        const std::string& authorization_value) override;
  NvramResult ListSpaces(std::vector<uint32_t>* index_list) override;
  NvramResult GetSpaceInfo(
      uint32_t index,
      size_t* size,
      bool* is_read_locked,
      bool* is_write_locked,
      std::vector<NvramSpaceAttribute>* attributes,
      NvramSpacePolicy* policy) override;

 private:
  // Must be called before using any data members. This may be called multiple
  // times and will be very fast if already initialized.
  bool Initialize();

  // Gets the TPM owner password. Returns an empty string if not available.
  std::string GetOwnerPassword();

  // Configures |trunks_session_| with owner authorization. Returns true on
  // success.
  bool SetupOwnerSession();

  // Configures a new policy |session| for a given |policy_record|,
  // |authorization_value|, and |command_code|. Returns true on success.
  bool SetupPolicySession(const NvramPolicyRecord& policy_record,
                          const std::string& authorization_value,
                          trunks::TPM_CC command_code,
                          trunks::PolicySession* session);

  // A helper to add policies to a |session| for a particular |command_code| and
  // |policy_record|. Returns true on success.
  bool AddPoliciesForCommand(const NvramPolicyRecord& policy_record,
                             trunks::TPM_CC command_code,
                             trunks::PolicySession* session);

  // A helper to add an OR policy to |session| based on |policy_record|. Returns
  // true on success.
  bool AddPolicyOR(const NvramPolicyRecord& policy_record,
                   trunks::PolicySession* session);

  // Computes the policy |digest| for a given |policy_record| and fills the
  // policy_digests field in the |policy_record|.
  bool ComputePolicyDigest(NvramPolicyRecord* policy_record,
                           std::string* digest);

  // Gets the policy |record| for the given |index|. Returns true on success.
  bool GetPolicyRecord(uint32_t index, NvramPolicyRecord* record);

  // Saves a policy |record| in the local_data_store_.
  bool SavePolicyRecord(const NvramPolicyRecord& record);

  // Best effort delete of the policy |record| for |index|.
  void DeletePolicyRecord(uint32_t index);

  const trunks::TrunksFactory& trunks_factory_;
  LocalDataStore* local_data_store_;
  bool initialized_;
  std::unique_ptr<trunks::HmacSession> trunks_session_;
  std::unique_ptr<trunks::TpmUtility> trunks_utility_;

  friend class Tpm2NvramTest;
  DISALLOW_COPY_AND_ASSIGN(Tpm2NvramImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM2_NVRAM_IMPL_H_
