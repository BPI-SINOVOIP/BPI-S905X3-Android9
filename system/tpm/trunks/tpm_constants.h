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

#ifndef TRUNKS_TPM_CONSTANTS_H_
#define TRUNKS_TPM_CONSTANTS_H_

#include "trunks/tpm_generated.h"

namespace trunks {

// TPM Object Attributes.
constexpr TPMA_OBJECT kFixedTPM = 1U << 1;
constexpr TPMA_OBJECT kFixedParent = 1U << 4;
constexpr TPMA_OBJECT kSensitiveDataOrigin = 1U << 5;
constexpr TPMA_OBJECT kUserWithAuth = 1U << 6;
constexpr TPMA_OBJECT kAdminWithPolicy = 1U << 7;
constexpr TPMA_OBJECT kNoDA = 1U << 10;
constexpr TPMA_OBJECT kRestricted = 1U << 16;
constexpr TPMA_OBJECT kDecrypt = 1U << 17;
constexpr TPMA_OBJECT kSign = 1U << 18;

// TPM NV Index Attributes, defined in TPM Spec Part 2 section 13.2.
constexpr TPMA_NV TPMA_NV_PPWRITE = 1U << 0;
constexpr TPMA_NV TPMA_NV_OWNERWRITE = 1U << 1;
constexpr TPMA_NV TPMA_NV_AUTHWRITE = 1U << 2;
constexpr TPMA_NV TPMA_NV_POLICYWRITE = 1U << 3;
constexpr TPMA_NV TPMA_NV_COUNTER = 1U << 4;
constexpr TPMA_NV TPMA_NV_BITS = 1U << 5;
constexpr TPMA_NV TPMA_NV_EXTEND = 1U << 6;
constexpr TPMA_NV TPMA_NV_POLICY_DELETE = 1U << 10;
constexpr TPMA_NV TPMA_NV_WRITELOCKED = 1U << 11;
constexpr TPMA_NV TPMA_NV_WRITEALL = 1U << 12;
constexpr TPMA_NV TPMA_NV_WRITEDEFINE = 1U << 13;
constexpr TPMA_NV TPMA_NV_WRITE_STCLEAR = 1U << 14;
constexpr TPMA_NV TPMA_NV_GLOBALLOCK = 1U << 15;
constexpr TPMA_NV TPMA_NV_PPREAD = 1U << 16;
constexpr TPMA_NV TPMA_NV_OWNERREAD = 1U << 17;
constexpr TPMA_NV TPMA_NV_AUTHREAD = 1U << 18;
constexpr TPMA_NV TPMA_NV_POLICYREAD = 1U << 19;
constexpr TPMA_NV TPMA_NV_NO_DA = 1U << 25;
constexpr TPMA_NV TPMA_NV_ORDERLY = 1U << 26;
constexpr TPMA_NV TPMA_NV_CLEAR_STCLEAR = 1U << 27;
constexpr TPMA_NV TPMA_NV_READLOCKED = 1U << 28;
constexpr TPMA_NV TPMA_NV_WRITTEN = 1U << 29;
constexpr TPMA_NV TPMA_NV_PLATFORMCREATE = 1U << 30;
constexpr TPMA_NV TPMA_NV_READ_STCLEAR = 1U << 31;

}  // namespace trunks

#endif  // TRUNKS_TPM_CONSTANTS_H_
