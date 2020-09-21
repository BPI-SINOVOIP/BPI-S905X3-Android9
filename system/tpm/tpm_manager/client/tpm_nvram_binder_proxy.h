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

#ifndef TPM_MANAGER_CLIENT_TPM_NVRAM_BINDER_PROXY_H_
#define TPM_MANAGER_CLIENT_TPM_NVRAM_BINDER_PROXY_H_

#include <base/macros.h>

#include "android/tpm_manager/ITpmNvram.h"
#include "tpm_manager/common/export.h"
#include "tpm_manager/common/tpm_nvram_interface.h"

namespace tpm_manager {

// An implementation of TpmNvramInterface that forwards requests to tpm_managerd
// using Binder.
class TPM_MANAGER_EXPORT TpmNvramBinderProxy : public TpmNvramInterface {
 public:
  TpmNvramBinderProxy() = default;
  explicit TpmNvramBinderProxy(android::tpm_manager::ITpmNvram* binder);
  ~TpmNvramBinderProxy() override = default;

  // Initializes the client. Returns true on success.
  bool Initialize();

  // TpmNvramInterface methods. All assume a message loop and binder watcher
  // have already been configured elsewhere.
  void DefineSpace(const DefineSpaceRequest& request,
                   const DefineSpaceCallback& callback) override;
  void DestroySpace(const DestroySpaceRequest& request,
                    const DestroySpaceCallback& callback) override;
  void WriteSpace(const WriteSpaceRequest& request,
                  const WriteSpaceCallback& callback) override;
  void ReadSpace(const ReadSpaceRequest& request,
                 const ReadSpaceCallback& callback) override;
  void LockSpace(const LockSpaceRequest& request,
                 const LockSpaceCallback& callback) override;
  void ListSpaces(const ListSpacesRequest& request,
                  const ListSpacesCallback& callback) override;
  void GetSpaceInfo(const GetSpaceInfoRequest& request,
                    const GetSpaceInfoCallback& callback) override;

 private:
  android::sp<android::tpm_manager::ITpmNvram> default_binder_;
  android::tpm_manager::ITpmNvram* binder_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TpmNvramBinderProxy);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_CLIENT_TPM_NVRAM_BINDER_PROXY_H_
