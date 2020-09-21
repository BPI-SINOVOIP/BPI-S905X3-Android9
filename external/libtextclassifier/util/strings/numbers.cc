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

#include "util/strings/numbers.h"

#ifdef COMPILER_MSVC
#include <sstream>
#endif  // COMPILER_MSVC

#include <stdlib.h>

namespace libtextclassifier2 {

bool ParseInt32(const char *c_str, int32 *value) {
  char *temp;

  // Short version of man strtol:
  //
  // strtol parses some optional whitespaces, an optional +/- sign, and next a
  // succession of digits.  If it finds some digits, it sets temp to point to
  // the first character after that succession of digits and returns the parsed
  // integer.
  //
  // If there were no digits at all, strtol() sets temp to be c_str (the start
  // address) and returns 0.
  *value = strtol(c_str, &temp, 0);  // NOLINT

  // temp != c_str means that the input string contained at least one digit (see
  // above).  *temp == '\0' means the input string does not contain any random
  // chars after the number.
  return (temp != c_str) && (*temp == '\0');
}

bool ParseInt64(const char *c_str, int64 *value) {
  char *temp;
  *value = strtoll(c_str, &temp, 0);  // NOLINT

  // See comments inside ParseInt32.
  return (temp != c_str) && (*temp == '\0');
}

bool ParseDouble(const char *c_str, double *value) {
  char *temp;
  *value = strtod(c_str, &temp);

  // See comments inside ParseInt32.
  return (temp != c_str) && (*temp == '\0');
}

#ifdef COMPILER_MSVC
std::string IntToString(int64 input) {
  std::stringstream stream;
  stream << input;
  return stream.str();
}
#else
std::string IntToString(int64 input) {
  return std::to_string(input);
}
#endif  // COMPILER_MSVC

}  // namespace libtextclassifier2
