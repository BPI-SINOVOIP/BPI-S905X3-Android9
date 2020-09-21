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

#include "tpm_manager/server/mock_tpm_initializer.h"

using testing::Return;

namespace tpm_manager {

MockTpmInitializer::MockTpmInitializer() {
  ON_CALL(*this, InitializeTpm()).WillByDefault(Return(true));
  ON_CALL(*this, ResetDictionaryAttackLock()).WillByDefault(Return(true));
}
MockTpmInitializer::~MockTpmInitializer() {}

}  // namespace tpm_manager
