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

#ifndef LIBTEXTCLASSIFIER_UTIL_STRINGS_STRINGPIECE_H_
#define LIBTEXTCLASSIFIER_UTIL_STRINGS_STRINGPIECE_H_

#include <stddef.h>

#include <string>

namespace libtextclassifier2 {

// Read-only "view" of a piece of data.  Does not own the underlying data.
class StringPiece {
 public:
  StringPiece() : StringPiece(nullptr, 0) {}

  StringPiece(const char *str)  // NOLINT(runtime/explicit)
      : start_(str), size_(strlen(str)) {}

  StringPiece(const char *start, size_t size)
      : start_(start), size_(size) {}

  // Intentionally no "explicit" keyword: in function calls, we want strings to
  // be converted to StringPiece implicitly.
  StringPiece(const std::string &s)  // NOLINT(runtime/explicit)
      : StringPiece(s.data(), s.size()) {}

  StringPiece(const std::string &s, int offset, int len)
      : StringPiece(s.data() + offset, len) {}

  char operator[](size_t i) const { return start_[i]; }

  // Returns start address of underlying data.
  const char *data() const { return start_; }

  // Returns number of bytes of underlying data.
  size_t size() const { return size_; }
  size_t length() const { return size_; }

  bool empty() const { return size_ == 0; }

  // Returns a std::string containing a copy of the underlying data.
  std::string ToString() const {
    return std::string(data(), size());
  }

 private:
  const char *start_;  // Not owned.
  size_t size_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_STRINGS_STRINGPIECE_H_
