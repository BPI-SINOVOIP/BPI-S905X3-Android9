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

#ifndef TRUNKS_TRUNKS_FACTORY_H_
#define TRUNKS_TRUNKS_FACTORY_H_

#include <memory>
#include <string>

#include <base/macros.h>

#include "trunks/authorization_delegate.h"
#include "trunks/blob_parser.h"
#include "trunks/hmac_session.h"
#include "trunks/policy_session.h"
#include "trunks/session_manager.h"
#include "trunks/tpm_state.h"
#include "trunks/tpm_utility.h"
#include "trunks/trunks_export.h"

namespace trunks {

class Tpm;

// TrunksFactory is an interface to act as a factory for trunks objects. This
// mechanism assists in injecting mocks for testing.
class TRUNKS_EXPORT TrunksFactory {
 public:
  TrunksFactory() {}
  virtual ~TrunksFactory() {}

  // Returns a Tpm instance. The caller does not take ownership. All calls to
  // this method on a given TrunksFactory instance will return the same value.
  virtual Tpm* GetTpm() const = 0;

  // Returns an uninitialized TpmState instance. The caller takes ownership.
  virtual std::unique_ptr<TpmState> GetTpmState() const = 0;

  // Returns a TpmUtility instance. The caller takes ownership.
  virtual std::unique_ptr<TpmUtility> GetTpmUtility() const = 0;

  // Returns an AuthorizationDelegate instance for basic password authorization.
  // The caller takes ownership.
  virtual std::unique_ptr<AuthorizationDelegate> GetPasswordAuthorization(
      const std::string& password) const = 0;

  // Returns a SessionManager instance. The caller takes ownership.
  virtual std::unique_ptr<SessionManager> GetSessionManager() const = 0;

  // Returns a HmacSession instance. The caller takes ownership.
  virtual std::unique_ptr<HmacSession> GetHmacSession() const = 0;

  // Returns a PolicySession instance. The caller takes ownership.
  virtual std::unique_ptr<PolicySession> GetPolicySession() const = 0;

  // Returns a TrialSession instance. The caller takes ownership.
  virtual std::unique_ptr<PolicySession> GetTrialSession() const = 0;

  // Returns a BlobParser instance. The caller takes ownership.
  virtual std::unique_ptr<BlobParser> GetBlobParser() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TrunksFactory);
};

}  // namespace trunks

#endif  // TRUNKS_TRUNKS_FACTORY_H_
