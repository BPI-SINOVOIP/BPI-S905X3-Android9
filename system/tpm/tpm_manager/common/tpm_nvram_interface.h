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

#ifndef TPM_MANAGER_COMMON_TPM_NVRAM_INTERFACE_H_
#define TPM_MANAGER_COMMON_TPM_NVRAM_INTERFACE_H_

#include <base/callback.h>

#include "tpm_manager/common/export.h"
#include "tpm_manager/common/tpm_manager.pb.h"

namespace tpm_manager {

// The command interface for working with TPM NVRAM. Inherited by both IPC proxy
// and service classes. All methods are asynchronous because all TPM operations
// may take a long time to finish.
class TPM_MANAGER_EXPORT TpmNvramInterface {
 public:
  virtual ~TpmNvramInterface() = default;

  // Processes a DefineSpaceRequest and responds with a DefineSpaceReply.
  using DefineSpaceCallback = base::Callback<void(const DefineSpaceReply&)>;
  virtual void DefineSpace(const DefineSpaceRequest& request,
                           const DefineSpaceCallback& callback) = 0;

  // Processes a DestroySpaceRequest and responds with a DestroySpaceReply.
  using DestroySpaceCallback = base::Callback<void(const DestroySpaceReply&)>;
  virtual void DestroySpace(const DestroySpaceRequest& request,
                            const DestroySpaceCallback& callback) = 0;

  // Processes a WriteSpaceRequest and responds with a WriteSpaceReply.
  using WriteSpaceCallback = base::Callback<void(const WriteSpaceReply&)>;
  virtual void WriteSpace(const WriteSpaceRequest& request,
                          const WriteSpaceCallback& callback) = 0;

  // Processes a ReadSpaceRequest and responds with a ReadSpaceReply.
  using ReadSpaceCallback = base::Callback<void(const ReadSpaceReply&)>;
  virtual void ReadSpace(const ReadSpaceRequest& request,
                         const ReadSpaceCallback& callback) = 0;

  // Processes a LockSpaceRequest and responds with a LockSpaceReply.
  using LockSpaceCallback = base::Callback<void(const LockSpaceReply&)>;
  virtual void LockSpace(const LockSpaceRequest& request,
                         const LockSpaceCallback& callback) = 0;

  // Processes a ListSpacesRequest and responds with a ListSpacesReply.
  using ListSpacesCallback = base::Callback<void(const ListSpacesReply&)>;
  virtual void ListSpaces(const ListSpacesRequest& request,
                          const ListSpacesCallback& callback) = 0;

  // Processes a GetSpaceInfoRequest and responds with a GetSpaceInfoReply.
  using GetSpaceInfoCallback = base::Callback<void(const GetSpaceInfoReply&)>;
  virtual void GetSpaceInfo(const GetSpaceInfoRequest& request,
                            const GetSpaceInfoCallback& callback) = 0;
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_TPM_NVRAM_INTERFACE_H_
