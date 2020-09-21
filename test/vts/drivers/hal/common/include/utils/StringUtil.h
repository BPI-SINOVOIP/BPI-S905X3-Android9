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

#ifndef __VTS_SYSFUZZER_COMMON_UTILS_STRINGUTIL_H__
#define __VTS_SYSFUZZER_COMMON_UTILS_STRINGUTIL_H__

#include <string>

using namespace std;

namespace android {
namespace vts {

// returns true if s ends with suffix.
bool endsWith(const string& s, const string& suffix);


// custom util function to replace all occurrences of a substring.
extern void ReplaceSubString(
    string& original, const string& from, const string& to);


}  // namespace vts
}  // namespace android

#endif  // __VTS_SYSFUZZER_COMMON_UTILS_STRINGUTIL_H__
