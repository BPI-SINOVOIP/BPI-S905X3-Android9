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

#include "util/strings/split.h"

namespace libtextclassifier2 {
namespace strings {

std::vector<StringPiece> Split(const StringPiece &text, char delim) {
  std::vector<StringPiece> result;
  int token_start = 0;
  if (!text.empty()) {
    for (size_t i = 0; i < text.size() + 1; i++) {
      if ((i == text.size()) || (text[i] == delim)) {
        result.push_back(
            StringPiece(text.data() + token_start, i - token_start));
        token_start = i + 1;
      }
    }
  }
  return result;
}

}  // namespace strings
}  // namespace libtextclassifier2
