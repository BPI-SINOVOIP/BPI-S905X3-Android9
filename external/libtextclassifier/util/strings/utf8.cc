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

#include "util/strings/utf8.h"

namespace libtextclassifier2 {
bool IsValidUTF8(const char *src, int size) {
  for (int i = 0; i < size;) {
    // Unexpected trail byte.
    if (IsTrailByte(src[i])) {
      return false;
    }

    const int num_codepoint_bytes = GetNumBytesForUTF8Char(&src[i]);
    if (num_codepoint_bytes <= 0 || i + num_codepoint_bytes > size) {
      return false;
    }

    // Check that remaining bytes in the codepoint are trailing bytes.
    i++;
    for (int k = 1; k < num_codepoint_bytes; k++, i++) {
      if (!IsTrailByte(src[i])) {
        return false;
      }
    }
  }
  return true;
}
}  // namespace libtextclassifier2
