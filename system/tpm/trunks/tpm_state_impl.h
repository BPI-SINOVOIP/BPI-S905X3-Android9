//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef TRUNKS_TPM_STATE_IMPL_H_
#define TRUNKS_TPM_STATE_IMPL_H_

#include "trunks/tpm_state.h"

#include <map>

#include <base/callback.h>
#include <base/macros.h>

#include "trunks/tpm_generated.h"
#include "trunks/trunks_export.h"

namespace trunks {

class TrunksFactory;

// TpmStateImpl is the default implementation of the TpmState interface.
class TRUNKS_EXPORT TpmStateImpl : public TpmState {
 public:
  explicit TpmStateImpl(const TrunksFactory& factory);
  ~TpmStateImpl() override = default;

  // TpmState methods.
  TPM_RC Initialize() override;
  bool IsOwnerPasswordSet() override;
  bool IsEndorsementPasswordSet() override;
  bool IsLockoutPasswordSet() override;
  bool IsOwned() override;
  bool IsInLockout() override;
  bool IsPlatformHierarchyEnabled() override;
  bool IsStorageHierarchyEnabled() override;
  bool IsEndorsementHierarchyEnabled() override;
  bool IsEnabled() override;
  bool WasShutdownOrderly() override;
  bool IsRSASupported() override;
  bool IsECCSupported() override;
  uint32_t GetLockoutCounter() override;
  uint32_t GetLockoutThreshold() override;
  uint32_t GetLockoutInterval() override;
  uint32_t GetLockoutRecovery() override;
  uint32_t GetMaxNVSize() override;
  bool GetTpmProperty(TPM_PT property, uint32_t* value) override;
  bool GetAlgorithmProperties(TPM_ALG_ID algorithm,
                              TPMA_ALGORITHM* properties) override;

 private:
  // This helper method calls TPM2_GetCapability in a loop until all available
  // capabilities of the given type are sent to the |callback|. The callback
  // returns the next property value to query if there is more data available or
  // 0 if the capability data was empty.
  using CapabilityCallback = base::Callback<uint32_t(const TPMU_CAPABILITIES&)>;
  TPM_RC GetCapability(const CapabilityCallback& callback,
                       TPM_CAP capability,
                       uint32_t property,
                       uint32_t max_properties_per_call);
  // Queries TPM properties and populates tpm_properties_.
  TPM_RC CacheTpmProperties();
  // Queries algorithm properties and populates algorithm_properties_.
  TPM_RC CacheAlgorithmProperties();

  const TrunksFactory& factory_;
  bool initialized_{false};
  std::map<TPM_PT, uint32_t> tpm_properties_;
  std::map<TPM_ALG_ID, TPMA_ALGORITHM> algorithm_properties_;

  DISALLOW_COPY_AND_ASSIGN(TpmStateImpl);
};

}  // namespace trunks

#endif  // TRUNKS_TPM_STATE_IMPL_H_
