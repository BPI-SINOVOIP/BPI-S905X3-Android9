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

#ifndef TPM_MANAGER_SERVER_BINDER_SERVICE_H_
#define TPM_MANAGER_SERVER_BINDER_SERVICE_H_

#include <brillo/binder_watcher.h>
#include <brillo/daemons/daemon.h>

#include "android/tpm_manager/BnTpmNvram.h"
#include "android/tpm_manager/BnTpmOwnership.h"
#include "tpm_manager/common/tpm_nvram_interface.h"
#include "tpm_manager/common/tpm_ownership_interface.h"

namespace tpm_manager {

// BinderService registers for and handles all incoming binder calls for the
// tpm_managerd system daemon.
//
// Example Usage:
//
// BinderService service(&nvram_service, &ownership_service);
// service.Run();
class BinderService : public brillo::Daemon {
 public:
  BinderService(TpmNvramInterface* nvram_service,
                TpmOwnershipInterface* ownership_service);
  ~BinderService() override = default;

  // Does basic setup but does not register with the binder subsystem.
  void InitForTesting();

  // Getters for binder interfaces. Callers do not take ownership. These should
  // only be used for testing.
  android::tpm_manager::ITpmNvram* GetITpmNvram();
  android::tpm_manager::ITpmOwnership* GetITpmOwnership();

 protected:
  int OnInit() override;

 private:
  friend class NvramServiceInternal;
  class NvramServiceInternal : public android::tpm_manager::BnTpmNvram {
   public:
    explicit NvramServiceInternal(TpmNvramInterface* service);
    ~NvramServiceInternal() override = default;

    // ITpmNvram interface.
    android::binder::Status DefineSpace(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status DestroySpace(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status WriteSpace(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status ReadSpace(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status ListSpaces(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status GetSpaceInfo(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status LockSpace(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;

   private:
    TpmNvramInterface* nvram_service_;
  };

  friend class OwnershipServiceInternal;
  class OwnershipServiceInternal : public android::tpm_manager::BnTpmOwnership {
   public:
    explicit OwnershipServiceInternal(TpmOwnershipInterface* service);
    ~OwnershipServiceInternal() override = default;

    // ITpmOwnership interface.
    android::binder::Status GetTpmStatus(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status TakeOwnership(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;
    android::binder::Status RemoveOwnerDependency(
        const std::vector<uint8_t>& command_proto,
        const android::sp<android::tpm_manager::ITpmManagerClient>& client)
        override;

   private:
    TpmOwnershipInterface* ownership_service_;
  };

  brillo::BinderWatcher watcher_;
  android::sp<NvramServiceInternal> nvram_binder_;
  android::sp<OwnershipServiceInternal> ownership_binder_;
  TpmNvramInterface* nvram_service_;
  TpmOwnershipInterface* ownership_service_;

  DISALLOW_COPY_AND_ASSIGN(BinderService);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_BINDER_SERVICE_H_
