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

#ifndef LIBTEXTCLASSIFIER_TYPES_TEST_UTIL_H_
#define LIBTEXTCLASSIFIER_TYPES_TEST_UTIL_H_

#include <ostream>

#include "types.h"
#include "util/base/logging.h"

namespace libtextclassifier2 {

inline std::ostream& operator<<(std::ostream& stream, const Token& value) {
  logging::LoggingStringStream tmp_stream;
  tmp_stream << value;
  return stream << tmp_stream.message;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const AnnotatedSpan& value) {
  logging::LoggingStringStream tmp_stream;
  tmp_stream << value;
  return stream << tmp_stream.message;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const DatetimeParseResultSpan& value) {
  logging::LoggingStringStream tmp_stream;
  tmp_stream << value;
  return stream << tmp_stream.message;
}

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_TYPES_TEST_UTIL_H_
