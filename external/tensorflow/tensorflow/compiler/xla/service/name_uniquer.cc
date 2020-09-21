/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/compiler/xla/service/name_uniquer.h"

#include "tensorflow/compiler/xla/types.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"

namespace xla {

namespace {

bool IsAllowed(char character) {
  auto c = static_cast<unsigned char>(character);
  return (isalnum(c) != 0) || c == '_' || c == '.' || c == '-';
}

}  // namespace

NameUniquer::NameUniquer(const string& separator) {
  CHECK(std::all_of(separator.begin(), separator.end(), IsAllowed))
      << "separator should comprises allowed characters only";
  separator_ = separator;
}

/*static*/ string NameUniquer::GetSanitizedName(const string& name) {
  string result = name;
  CHECK(!result.empty()) << "name should not be empty";
  char c = static_cast<unsigned char>(result[0]);
  if (!isalpha(c) && c != '_') {
    result[0] = '_';
  }
  for (int i = 1; i < result.length(); i++) {
    if (!IsAllowed(result[i])) {
      result[i] = '_';
    }
  }
  return result;
}

string NameUniquer::GetUniqueName(tensorflow::StringPiece prefix) {
  string root = prefix.empty() ? "name" : prefix.ToString();
  root = GetSanitizedName(root);

  // Strip away numeric suffix (if any). Only recognize separator if it is in
  // the middle of the name.
  size_t separator_index = root.rfind(separator_);
  if (separator_index != string::npos && (separator_index > 0) &&
      (separator_index < root.size() - 1)) {
    string after_suffix = root.substr(separator_index + 1);
    int64 numeric_suffix;
    if (tensorflow::strings::safe_strto64(after_suffix, &numeric_suffix)) {
      // Remove numeric suffix from root.
      root = root.substr(0, separator_index);
      // Update count to at least the numeric suffix value to avoid future
      // colisions with this name.
      generated_names_[root] = std::max(generated_names_[root], numeric_suffix);
    }
  }

  int64* count = &(generated_names_[root]);
  if (*count == 0) {
    *count = 1;
    return root;
  } else {
    tensorflow::strings::StrAppend(&root, separator_, *count);
    // Increment lookup under old 'root' name.
    (*count)++;
    return root;
  }
}

}  // namespace xla
