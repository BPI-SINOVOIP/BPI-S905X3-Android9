// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>

#include "policy/policy_util.h"

namespace em = enterprise_management;

namespace policy {

// Test LoadPolicyFromPath returns correct values and has policy data when
// successful.
TEST(DevicePolicyUtilTest, LoadPolicyFromPath) {
  // Create the temporary directory.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath invalid_policy_data_path(temp_dir.path().Append("policy"));
  base::FilePath inexistent_file(temp_dir.path().Append("policy.1"));
  base::FilePath good_policy_data_path(temp_dir.path().Append("policy.2"));

  // Create the file with invalid data.
  std::string data = "invalid data";
  ASSERT_TRUE(
      base::WriteFile(invalid_policy_data_path, data.data(), data.size()));

  // Create the file with good policy data.
  em::PolicyData policy_data;
  policy_data.set_username("user@example.com");
  policy_data.set_management_mode(em::PolicyData::LOCAL_OWNER);
  policy_data.set_request_token("codepath-must-ignore-dmtoken");
  std::string policy_blob;
  policy_data.SerializeToString(&policy_blob);
  ASSERT_TRUE(base::WriteFile(good_policy_data_path, policy_blob.data(),
                              policy_blob.size()));

  std::string policy_data_str;
  enterprise_management::PolicyFetchResponse policy;
  EXPECT_EQ(
      LoadPolicyResult::kInvalidPolicyData,
      LoadPolicyFromPath(invalid_policy_data_path, &policy_data_str, &policy));
  EXPECT_EQ(LoadPolicyResult::kFileNotFound,
            LoadPolicyFromPath(inexistent_file, &policy_data_str, &policy));
  EXPECT_EQ(
      LoadPolicyResult::kSuccess,
      LoadPolicyFromPath(good_policy_data_path, &policy_data_str, &policy));
  ASSERT_TRUE(policy.has_policy_data());
}

}  // namespace policy
