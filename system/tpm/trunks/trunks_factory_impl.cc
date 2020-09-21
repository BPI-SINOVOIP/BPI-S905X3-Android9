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

#include "trunks/trunks_factory_impl.h"

#include <base/logging.h>
#include <base/memory/ptr_util.h>

#include "trunks/blob_parser.h"
#include "trunks/hmac_session_impl.h"
#include "trunks/password_authorization_delegate.h"
#include "trunks/policy_session_impl.h"
#include "trunks/session_manager_impl.h"
#include "trunks/tpm_generated.h"
#include "trunks/tpm_state_impl.h"
#include "trunks/tpm_utility_impl.h"
#if defined(USE_BINDER_IPC)
#include "trunks/trunks_binder_proxy.h"
#else
#include "trunks/trunks_dbus_proxy.h"
#endif

namespace trunks {

TrunksFactoryImpl::TrunksFactoryImpl() {
#if defined(USE_BINDER_IPC)
  default_transceiver_.reset(new TrunksBinderProxy());
#else
  default_transceiver_.reset(new TrunksDBusProxy());
#endif
  transceiver_ = default_transceiver_.get();
}

TrunksFactoryImpl::TrunksFactoryImpl(CommandTransceiver* transceiver) {
  transceiver_ = transceiver;
}

TrunksFactoryImpl::~TrunksFactoryImpl() {}

bool TrunksFactoryImpl::Initialize() {
  if (initialized_) {
    return true;
  }
  tpm_.reset(new Tpm(transceiver_));
  if (transceiver_ != default_transceiver_.get()) {
    initialized_ = true;
  } else {
    initialized_ = transceiver_->Init();
    if (!initialized_) {
      LOG(WARNING) << "Failed to initialize the trunks IPC proxy; "
                   << "trunksd is not ready.";
    }
  }
  return initialized_;
}

Tpm* TrunksFactoryImpl::GetTpm() const {
  return tpm_.get();
}

std::unique_ptr<TpmState> TrunksFactoryImpl::GetTpmState() const {
  return base::MakeUnique<TpmStateImpl>(*this);
}

std::unique_ptr<TpmUtility> TrunksFactoryImpl::GetTpmUtility() const {
  return base::MakeUnique<TpmUtilityImpl>(*this);
}

std::unique_ptr<AuthorizationDelegate>
TrunksFactoryImpl::GetPasswordAuthorization(const std::string& password) const {
  return base::MakeUnique<PasswordAuthorizationDelegate>(password);
}

std::unique_ptr<SessionManager> TrunksFactoryImpl::GetSessionManager() const {
  return base::MakeUnique<SessionManagerImpl>(*this);
}

std::unique_ptr<HmacSession> TrunksFactoryImpl::GetHmacSession() const {
  return base::MakeUnique<HmacSessionImpl>(*this);
}

std::unique_ptr<PolicySession> TrunksFactoryImpl::GetPolicySession() const {
  return base::MakeUnique<PolicySessionImpl>(*this, TPM_SE_POLICY);
}

std::unique_ptr<PolicySession> TrunksFactoryImpl::GetTrialSession() const {
  return base::MakeUnique<PolicySessionImpl>(*this, TPM_SE_TRIAL);
}

std::unique_ptr<BlobParser> TrunksFactoryImpl::GetBlobParser() const {
  return base::MakeUnique<BlobParser>();
}

}  // namespace trunks
