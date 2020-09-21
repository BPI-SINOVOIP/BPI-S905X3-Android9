/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "utils/StringUtil.h"

#include <string>

using namespace std;

namespace android {
namespace vts {

bool endsWith(const string& s, const string& suffix) {
  return s.size() >= suffix.size() && s.rfind(suffix) == (s.size() - suffix.size());
}

void ReplaceSubString(string& original, const string& from, const string& to) {
  size_t index = 0;
  int from_len = from.length();
  while (true) {
    index = original.find(from, index);
    if (index == std::string::npos) break;
    original.replace(index, from_len, to);
    index += from_len;
  }
}


}  // namespace vts
}  // namespace android
