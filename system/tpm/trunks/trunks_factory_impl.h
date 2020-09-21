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

#ifndef TRUNKS_TRUNKS_FACTORY_IMPL_H_
#define TRUNKS_TRUNKS_FACTORY_IMPL_H_

#include "trunks/trunks_factory.h"

#include <memory>
#include <string>

#include <base/macros.h>

#include "trunks/command_transceiver.h"
#include "trunks/trunks_export.h"

namespace trunks {

// TrunksFactoryImpl is the default TrunksFactory implementation. This class is
// thread-safe with the exception of Initialize() but created objects are not
// necessarily thread-safe. Example usage:
//
// TrunksFactoryImpl factory;
// factory.Initialize(true /*failure_is_fatal*/);
// Tpm* tpm = factory.GetTpm();
class TRUNKS_EXPORT TrunksFactoryImpl : public TrunksFactory {
 public:
  // Uses an IPC proxy as the default CommandTransceiver.
  TrunksFactoryImpl();
  // TrunksFactoryImpl does not take ownership of |transceiver|. This
  // transceiver is forwarded down to the Tpm instance maintained by
  // this factory. It is assumed that the |transceiver| is already initialized.
  explicit TrunksFactoryImpl(CommandTransceiver* transceiver);
  ~TrunksFactoryImpl() override;

  // Initialize the factory. This must be called before any other methods.
  // Returns true on success.
  bool Initialize();

  // TrunksFactory methods.
  Tpm* GetTpm() const override;
  std::unique_ptr<TpmState> GetTpmState() const override;
  std::unique_ptr<TpmUtility> GetTpmUtility() const override;
  std::unique_ptr<AuthorizationDelegate> GetPasswordAuthorization(
      const std::string& password) const override;
  std::unique_ptr<SessionManager> GetSessionManager() const override;
  std::unique_ptr<HmacSession> GetHmacSession() const override;
  std::unique_ptr<PolicySession> GetPolicySession() const override;
  std::unique_ptr<PolicySession> GetTrialSession() const override;
  std::unique_ptr<BlobParser> GetBlobParser() const override;

 private:
  std::unique_ptr<CommandTransceiver> default_transceiver_;
  CommandTransceiver* transceiver_;
  std::unique_ptr<Tpm> tpm_;
  bool initialized_ = false;

  DISALLOW_COPY_AND_ASSIGN(TrunksFactoryImpl);
};

}  // namespace trunks

#endif  // TRUNKS_TRUNKS_FACTORY_IMPL_H_
