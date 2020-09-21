// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_POLICY_POLICY_UTIL_H_
#define LIBBRILLO_POLICY_POLICY_UTIL_H_

#include <string>

#include <base/files/file_path.h>
#include <brillo/brillo_export.h>

#include "bindings/device_management_backend.pb.h"

namespace policy {

// The detailed information of the result from loading the policy data.
enum class BRILLO_EXPORT LoadPolicyResult {
  kSuccess = 0,
  kFileNotFound = 1,
  kFailedToReadFile = 2,
  kEmptyFile = 3,
  kInvalidPolicyData = 4
};

// Reads and parses the policy data from |policy_path|. Returns the details
// in LoadPolicyResult. In case response is |kSuccess|, |policy_data_str_out|
// contains the raw data from the file, while |policy_out| contains the parsed
// policy data. Otherwise the contents of |policy_data_str_out| and |policy_out|
// is undefined.
BRILLO_EXPORT LoadPolicyResult LoadPolicyFromPath(
    const base::FilePath& policy_path,
    std::string* policy_data_str_out,
    enterprise_management::PolicyFetchResponse* policy_out);

}  // namespace policy

#endif  // LIBBRILLO_POLICY_POLICY_UTIL_H_
