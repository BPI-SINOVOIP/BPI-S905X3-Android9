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

#ifndef TPM_MANAGER_SERVER_TPM_INITIALIZER_H_
#define TPM_MANAGER_SERVER_TPM_INITIALIZER_H_

namespace tpm_manager {

// TpmInitializer performs initialization tasks on some kind of TPM device.
class TpmInitializer {
 public:
  TpmInitializer() = default;
  virtual ~TpmInitializer() = default;

  // Initializes a TPM and returns true on success. If the TPM is already
  // initialized, this method has no effect and succeeds. If the TPM is
  // partially initialized, e.g. the process was previously interrupted, then
  // the process picks up where it left off.
  virtual bool InitializeTpm() = 0;

  // This will be called when the service is initializing. It is an early
  // opportunity to perform tasks related to verified boot.
  virtual void VerifiedBootHelper() = 0;

  // Reset the state of TPM dictionary attack protection. Returns true on
  // success.
  virtual bool ResetDictionaryAttackLock() = 0;
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_SERVER_TPM_INITIALIZER_H_
