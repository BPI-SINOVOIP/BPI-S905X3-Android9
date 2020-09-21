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

#include "tpm_manager/server/tpm2_initializer_impl.h"

#include <string>

#include <base/logging.h>
#include <trunks/error_codes.h>
#include <trunks/tpm_utility.h>
#include <trunks/trunks_factory_impl.h>

#include "tpm_manager/common/tpm_manager.pb.h"
#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/server/openssl_crypto_util_impl.h"

using trunks::TPM_RC;
using trunks::TPM_RC_SUCCESS;

namespace {
const size_t kDefaultPasswordSize = 20;
}  // namespace

namespace tpm_manager {

Tpm2InitializerImpl::Tpm2InitializerImpl(const trunks::TrunksFactory& factory,
                                         LocalDataStore* local_data_store,
                                         TpmStatus* tpm_status)
    : trunks_factory_(factory),
      openssl_util_(new OpensslCryptoUtilImpl()),
      local_data_store_(local_data_store),
      tpm_status_(tpm_status) {}

Tpm2InitializerImpl::Tpm2InitializerImpl(const trunks::TrunksFactory& factory,
                                         OpensslCryptoUtil* openssl_util,
                                         LocalDataStore* local_data_store,
                                         TpmStatus* tpm_status)
    : trunks_factory_(factory),
      openssl_util_(openssl_util),
      local_data_store_(local_data_store),
      tpm_status_(tpm_status) {}

bool Tpm2InitializerImpl::InitializeTpm() {
  if (!SeedTpmRng()) {
    return false;
  }
  if (tpm_status_->IsTpmOwned()) {
    // Tpm is already owned, so we do not need to do anything.
    VLOG(1) << "Tpm already owned.";
    return true;
  }
  // First we read the local data. If we did not finish removing owner
  // dependencies or if TakeOwnership failed, we want to retake ownership
  // with the same passwords.
  LocalData local_data;
  if (!local_data_store_->Read(&local_data)) {
    LOG(ERROR) << "Error reading local data.";
    return false;
  }
  std::string owner_password;
  std::string endorsement_password;
  std::string lockout_password;
  // If there are valid owner dependencies, we need to reuse the old passwords.
  if (local_data.owner_dependency_size() > 0) {
    owner_password.assign(local_data.owner_password());
    endorsement_password.assign(local_data.endorsement_password());
    lockout_password.assign(local_data.lockout_password());
  } else {
    if (!GetTpmRandomData(kDefaultPasswordSize, &owner_password)) {
      LOG(ERROR) << "Error generating a random owner password.";
      return false;
    }
    if (!GetTpmRandomData(kDefaultPasswordSize, &endorsement_password)) {
      LOG(ERROR) << "Error generating a random endorsement password.";
      return false;
    }
    if (!GetTpmRandomData(kDefaultPasswordSize, &lockout_password)) {
      LOG(ERROR) << "Error generating a random lockout password.";
      return false;
    }
  }
  // We write the passwords to disk, in case there is an error while taking
  // ownership.
  local_data.clear_owner_dependency();
  for (auto dependency : kInitialTpmOwnerDependencies) {
    local_data.add_owner_dependency(dependency);
  }
  local_data.set_owner_password(owner_password);
  local_data.set_endorsement_password(endorsement_password);
  local_data.set_lockout_password(lockout_password);
  if (!local_data_store_->Write(local_data)) {
    LOG(ERROR) << "Error saving local data.";
    return false;
  }
  TPM_RC result = trunks_factory_.GetTpmUtility()->TakeOwnership(
      owner_password, endorsement_password, lockout_password);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error taking ownership of TPM2.0";
    return false;
  }

  return true;
}

void Tpm2InitializerImpl::VerifiedBootHelper() {
  constexpr char kVerifiedBootLateStageTag[] = "BOOT_PCR_LATE_STAGE";
  std::unique_ptr<trunks::TpmUtility> tpm_utility =
      trunks_factory_.GetTpmUtility();
  // Make sure PCRs 0-3 can't be spoofed from this point forward.
  for (int pcr : {0, 1, 2, 3}) {
    std::string value;
    TPM_RC result = tpm_utility->ReadPCR(pcr, &value);
    if (result) {
      LOG(ERROR) << "Failed to read PCR " << pcr << ": "
                 << trunks::GetErrorString(result);
      continue;
    }
    if (value == std::string(32, 0)) {
      LOG(WARNING) << "WARNING: Verified boot PCR " << pcr
                   << " is not initialized.";
      result = tpm_utility->ExtendPCR(pcr, kVerifiedBootLateStageTag, nullptr);
      if (result) {
        LOG(ERROR) << "Failed to extend PCR " << pcr << ": "
                   << trunks::GetErrorString(result);
      }
    }
  }
}

bool Tpm2InitializerImpl::ResetDictionaryAttackLock() {
  LocalData local_data;
  if (!local_data_store_->Read(&local_data)) {
    LOG(ERROR) << __func__ << ": Error reading local data.";
    return false;
  }
  if (!local_data.has_lockout_password()) {
    LOG(ERROR) << __func__ << ": Lockout password not available.";
    return false;
  }
  std::unique_ptr<trunks::HmacSession> session =
      trunks_factory_.GetHmacSession();
  TPM_RC result = session->StartUnboundSession(false);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << __func__ << ": Error initializing AuthorizationSession: "
               << trunks::GetErrorString(result);
    return false;
  }
  session->SetEntityAuthorizationValue(local_data.lockout_password());
  std::unique_ptr<trunks::TpmUtility> tpm_utility =
      trunks_factory_.GetTpmUtility();
  result =
      tpm_utility->ResetDictionaryAttackLock(session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << __func__ << ": Error resetting lock: "
               << trunks::GetErrorString(result);
    return false;
  }
  return true;
}

bool Tpm2InitializerImpl::SeedTpmRng() {
  std::string random_bytes;
  if (!openssl_util_->GetRandomBytes(kDefaultPasswordSize, &random_bytes)) {
    return false;
  }
  TPM_RC result = trunks_factory_.GetTpmUtility()->StirRandom(
      random_bytes, nullptr /* No Authorization */);
  if (result != TPM_RC_SUCCESS) {
    return false;
  }
  return true;
}

bool Tpm2InitializerImpl::GetTpmRandomData(size_t num_bytes,
                                           std::string* random_data) {
  TPM_RC result = trunks_factory_.GetTpmUtility()->GenerateRandom(
      num_bytes, nullptr /* No Authorization */, random_data);
  if (result != TPM_RC_SUCCESS) {
    return false;
  }
  return true;
}

}  // namespace tpm_manager
