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

#ifndef TPM_MANAGER_COMMON_MOCK_TPM_NVRAM_INTERFACE_H_
#define TPM_MANAGER_COMMON_MOCK_TPM_NVRAM_INTERFACE_H_

#include <gmock/gmock.h>

#include "tpm_manager/common/tpm_nvram_interface.h"

namespace tpm_manager {

class MockTpmNvramInterface : public TpmNvramInterface {
 public:
  MockTpmNvramInterface();
  ~MockTpmNvramInterface() override;

  MOCK_METHOD2(DefineSpace,
               void(const DefineSpaceRequest& request,
                    const DefineSpaceCallback& callback));
  MOCK_METHOD2(DestroySpace,
               void(const DestroySpaceRequest& request,
                    const DestroySpaceCallback& callback));
  MOCK_METHOD2(WriteSpace,
               void(const WriteSpaceRequest& request,
                    const WriteSpaceCallback& callback));
  MOCK_METHOD2(ReadSpace,
               void(const ReadSpaceRequest& request,
                    const ReadSpaceCallback& callback));
  MOCK_METHOD2(LockSpace,
               void(const LockSpaceRequest& request,
                    const LockSpaceCallback& callback));
  MOCK_METHOD2(ListSpaces,
               void(const ListSpacesRequest& request,
                    const ListSpacesCallback& callback));
  MOCK_METHOD2(GetSpaceInfo,
               void(const GetSpaceInfoRequest& request,
                    const GetSpaceInfoCallback& callback));
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_COMMON_MOCK_TPM_NVRAM_INTERFACE_H_
