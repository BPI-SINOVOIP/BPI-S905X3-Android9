// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_POLICY_RESILIENT_POLICY_UTIL_H_
#define LIBBRILLO_POLICY_RESILIENT_POLICY_UTIL_H_

#include <map>
#include <string>

#include <base/files/file_path.h>
#include <brillo/brillo_export.h>

namespace policy {

// Returns a map from policy file index to absolute path. The default policy
// file is included at index 0 if present (despite not having an index in its
// filename).
BRILLO_EXPORT std::map<int, base::FilePath> GetSortedResilientPolicyFilePaths(
    const base::FilePath& default_policy_path);

// Returns the path to policy file corresponding to |index| value, based on
// the path of the default policy given by |default_policy_path|. Doesn't check
// the existence of the file on disk.
BRILLO_EXPORT base::FilePath GetResilientPolicyFilePathForIndex(
    const base::FilePath& default_policy_path,
    int index);

// Returns whether the |policy_path| file is a resilient file based on the name
// of the file, assuming the |default_policy_path| is the path of the default
// policy file. If successful, the |index_out| contains the index of the file as
// deducted from the name. No parsing of file contents is done here.
BRILLO_EXPORT bool ParseResilientPolicyFilePath(
    const base::FilePath& policy_path,
    const base::FilePath& default_policy_path,
    int* index_out);

}  // namespace policy

#endif  // LIBBRILLO_POLICY_RESILIENT_POLICY_UTIL_H_
