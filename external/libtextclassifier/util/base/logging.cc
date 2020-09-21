/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util/base/logging.h"

#include <stdlib.h>

#include <iostream>

#include "util/base/logging_raw.h"

namespace libtextclassifier2 {
namespace logging {

namespace {
// Returns pointer to beginning of last /-separated token from file_name.
// file_name should be a pointer to a zero-terminated array of chars.
// E.g., "foo/bar.cc" -> "bar.cc", "foo/" -> "", "foo" -> "foo".
const char *JumpToBasename(const char *file_name) {
  if (file_name == nullptr) {
    return nullptr;
  }

  // Points to the beginning of the last encountered token.
  const char *last_token_start = file_name;
  while (*file_name != '\0') {
    if (*file_name == '/') {
      // Found token separator.  A new (potentially empty) token starts after
      // this position.  Notice that if file_name is a valid zero-terminated
      // string, file_name + 1 is a valid pointer (there is at least one char
      // after address file_name, the zero terminator).
      last_token_start = file_name + 1;
    }
    file_name++;
  }
  return last_token_start;
}
}  // namespace

LogMessage::LogMessage(LogSeverity severity, const char *file_name,
                       int line_number)
    : severity_(severity) {
  stream_ << JumpToBasename(file_name) << ":" << line_number << ": ";
}

LogMessage::~LogMessage() {
  LowLevelLogging(severity_, /* tag = */ "txtClsf", stream_.message);
  if (severity_ == FATAL) {
    exit(1);
  }
}

}  // namespace logging
}  // namespace libtextclassifier2
