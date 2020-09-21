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

#ifndef TPM_MANAGER_SERVER_TPM_NVRAM_H_
#define TPM_MANAGER_SERVER_TPM_NVRAM_H_

#include <string>
#include <vector>

#include "tpm_manager/common/tpm_manager.pb.h"

namespace tpm_manager {

// TpmNvram is an interface for working with TPM NVRAM.
class TpmNvram {
 public:
  TpmNvram() = default;
  virtual ~TpmNvram() = default;

  // Creates an NVRAM space in the TPM. Returns true on success.
  virtual NvramResult DefineSpace(
      uint32_t index,
      size_t size,
      const std::vector<NvramSpaceAttribute>& attributes,
      const std::string& authorization_value,
      NvramSpacePolicy policy) = 0;

  // Destroys an NVRAM space in the TPM. Returns true on success.
  virtual NvramResult DestroySpace(uint32_t index) = 0;

  // Writes |data| to the NVRAM space at |index|. The size of |data| must be
  // equal or less than the size of the NVRAM space. Returns true on success.
  virtual NvramResult WriteSpace(uint32_t index,
                                 const std::string& data,
                                 const std::string& authorization_value) = 0;

  // Reads all the |data| in the NVRAM space at |index|. Returns true on
  // success.
  virtual NvramResult ReadSpace(uint32_t index,
                                std::string* data,
                                const std::string& authorization_value) = 0;

  // Locks the NVRAM space at |index|. Returns true on success.
  virtual NvramResult LockSpace(uint32_t index,
                                bool lock_read,
                                bool lock_write,
                                const std::string& authorization_value) = 0;

  // Lists all existing NVRAM spaces. Returns true on success.
  virtual NvramResult ListSpaces(std::vector<uint32_t>* index_list) = 0;

  // Provides basic information about a given space. All pointer are optional
  // and may be NULL. Returns true on success.
  virtual NvramResult GetSpaceInfo(
      uint32_t index,
      size_t* size,
      bool* is_read_locked,
      bool* is_write_locked,
      std::vector<NvramSpaceAttribute>* attributes,
      NvramSpacePolicy* policy) = 0;
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_NVRAM_H_
