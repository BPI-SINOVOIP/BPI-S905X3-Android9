// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "policy/resilient_policy_util.h"

#include <algorithm>

#include <base/files/file_enumerator.h>
#include <base/files/file_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>

namespace policy {

std::map<int, base::FilePath> GetSortedResilientPolicyFilePaths(
    const base::FilePath& default_policy_path) {
  std::map<int, base::FilePath> sorted_policy_file_paths;
  base::FilePath directory = default_policy_path.DirName();
  if (!base::PathExists(directory))
    return sorted_policy_file_paths;

  // Iterate the list of files in the folder, identifying the policy files based
  // on the name. Store in the map the absolute paths.
  const std::string default_policy_file_name =
      default_policy_path.BaseName().value();
  base::FileEnumerator file_iter(directory, false, base::FileEnumerator::FILES);
  for (base::FilePath child_file = file_iter.Next(); !child_file.empty();
       child_file = file_iter.Next()) {
    int index;
    if (ParseResilientPolicyFilePath(child_file, default_policy_path, &index)) {
      sorted_policy_file_paths[index] = child_file;
    }
  }

  return sorted_policy_file_paths;
}

bool ParseResilientPolicyFilePath(const base::FilePath& policy_path,
                                  const base::FilePath& default_policy_path,
                                  int* index_out) {
  if (!base::StartsWith(policy_path.value(), default_policy_path.value(),
                        base::CompareCase::SENSITIVE)) {
    return false;
  }

  if (policy_path == default_policy_path) {
    *index_out = 0;
    return true;
  }

  const std::string extension =
      policy_path.value().substr(default_policy_path.value().size());
  if (extension.empty() || extension[0] != '.' ||
      !base::StringToInt(extension.substr(1), index_out) || *index_out <= 0) {
    return false;
  }

  return true;
}

base::FilePath GetResilientPolicyFilePathForIndex(
    const base::FilePath& default_policy_path,
    int index) {
  if (index == 0)
    return default_policy_path;
  return base::FilePath(default_policy_path.value() + "." +
                        std::to_string(index));
}

}  // namespace policy
