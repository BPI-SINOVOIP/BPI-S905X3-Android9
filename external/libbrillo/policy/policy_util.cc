// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "policy/policy_util.h"

#include <base/files/file_util.h>
#include <base/logging.h>

namespace policy {

LoadPolicyResult LoadPolicyFromPath(
    const base::FilePath& policy_path,
    std::string* policy_data_str_out,
    enterprise_management::PolicyFetchResponse* policy_out) {
  DCHECK(policy_data_str_out);
  DCHECK(policy_out);
  if (!base::PathExists(policy_path)) {
    return LoadPolicyResult::kFileNotFound;
  }

  if (!base::ReadFileToString(policy_path, policy_data_str_out)) {
    PLOG(ERROR) << "Could not read policy off disk at " << policy_path.value();
    return LoadPolicyResult::kFailedToReadFile;
  }

  if (policy_data_str_out->empty()) {
    LOG(ERROR) << "Empty policy file at " << policy_path.value();
    return LoadPolicyResult::kEmptyFile;
  }

  if (!policy_out->ParseFromString(*policy_data_str_out)) {
    LOG(ERROR) << "Policy on disk could not be parsed, file: "
               << policy_path.value();
    return LoadPolicyResult::kInvalidPolicyData;
  }

  return LoadPolicyResult::kSuccess;
}

}  // namespace policy
