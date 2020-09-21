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

#ifndef TPM_MANAGER_SERVER_TPM_NVRAM_IMPL_H_
#define TPM_MANAGER_SERVER_TPM_NVRAM_IMPL_H_

#include "tpm_manager/server/tpm_nvram.h"

#include <stdint.h>

#include <string>

#include <base/macros.h>
#include <trousers/scoped_tss_type.h>
#include <trousers/tss.h>

#include "tpm_manager/server/tpm_connection.h"

namespace tpm_manager {

class LocalDataStore;

class TpmNvramImpl : public TpmNvram {
 public:
  explicit TpmNvramImpl(LocalDataStore* local_data_store);
  ~TpmNvramImpl() override = default;

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
  // This method creates and initializes the nvram object associated with
  // |handle| at |index|. Returns true on success, else false.
  bool InitializeNvramHandle(uint32_t index,
                             trousers::ScopedTssNvStore* nv_handle,
                             TpmConnection* connection);

  // This method injects a tpm policy with the owner password. Returns true
  // on success.
  bool SetOwnerPolicy(trousers::ScopedTssNvStore* nv_handle);

  // Set a usage policy for the handle with the given authorization_value.
  bool SetUsagePolicy(const std::string& authorization_value,
                      trousers::ScopedTssNvStore* nv_handle,
                      TpmConnection* connection);

  // This method sets up the composite pcr provided by |pcr_handle| with the
  // value of PCR0 at locality 1. Returns true on success.
  bool SetCompositePcr0(trousers::ScopedTssPcrs* pcr_handle,
                        TpmConnection* connection);

  // This method gets the owner password stored on disk and returns it via the
  // out argument |owner_password|. Returns true if we were able to read a
  // non empty owner_password off disk, else false.
  bool GetOwnerPassword(std::string* owner_password);

  LocalDataStore* local_data_store_;
  // A default non-owner connection.
  TpmConnection tpm_connection_;

  DISALLOW_COPY_AND_ASSIGN(TpmNvramImpl);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_NVRAM_IMPL_H_
