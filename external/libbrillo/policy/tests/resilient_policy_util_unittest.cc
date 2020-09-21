// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include <map>
#include <string>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>

#include "policy/resilient_policy_util.h"

namespace {

const char kDefaultResilientPolicyFilePath[] = "policy";

void CreateFile(const base::FilePath& file_path) {
  base::File file(file_path, base::File::FLAG_CREATE | base::File::FLAG_WRITE);
}

}  // namespace

namespace policy {

// Test that the policy files from the folder are identified correctly.
TEST(DevicePolicyUtilTest, GetSortedResilientPolicyFilePaths) {
  // Create the temporary directory.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  base::FilePath file0(temp_dir.path().Append("policy"));
  base::FilePath file1(temp_dir.path().Append("policy.12"));
  base::FilePath file2(temp_dir.path().Append("policy.2"));
  base::FilePath file3(temp_dir.path().Append("policy.30"));
  base::FilePath invalid(temp_dir.path().Append("policy_4"));

  CreateFile(file0);
  CreateFile(file1);
  CreateFile(file2);
  CreateFile(file3);

  const base::FilePath test_file_path(
      temp_dir.path().Append(kDefaultResilientPolicyFilePath));
  std::map<int, base::FilePath> sorted_file_paths =
      GetSortedResilientPolicyFilePaths(test_file_path);

  EXPECT_EQ(4, sorted_file_paths.size());
  EXPECT_EQ(file0.value(), sorted_file_paths[0].value());
  EXPECT_EQ(file1.value(), sorted_file_paths[12].value());
  EXPECT_EQ(file2.value(), sorted_file_paths[2].value());
  EXPECT_EQ(file3.value(), sorted_file_paths[30].value());
  EXPECT_EQ("", sorted_file_paths[4].value());
}

}  // namespace policy
