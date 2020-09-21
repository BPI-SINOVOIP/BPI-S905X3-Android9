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

#ifndef LIBTEXTCLASSIFIER_UTIL_STRINGS_NUMBERS_H_
#define LIBTEXTCLASSIFIER_UTIL_STRINGS_NUMBERS_H_

#include <string>

#include "util/base/integral_types.h"

namespace libtextclassifier2 {

// Parses an int32 from a C-style string.
//
// c_str should point to a zero-terminated array of chars that contains the
// number representation as (a) "<radix-10-number>" (e.g., "721"), (b)
// "0x<radix-16-number>" (e.g., "0xa1"), or (c) "0<radix-8-number>" (e.g.,
// "017201").
//
// Stores parsed number into *value.  Returns true on success, false on error.
// Note: presence of extra characters after the number counts as an error: e.g.,
// parsing "123a" will return false due to the extra "a" (which is not a valid
// radix-10 digit).  Parsing a string that does not contain any digit (e.g., "")
// is treated as an error: this function returns false.
bool ParseInt32(const char *c_str, int32 *value);

// Like ParseInt32, but for int64.
bool ParseInt64(const char *c_str, int64 *value);

// Like ParseInt32, but for double.
bool ParseDouble(const char *c_str, double *value);

// Converts an integer to string.  Accepts (via implicit conversions) all common
// int types.
std::string IntToString(int64 input);

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_STRINGS_NUMBERS_H_
