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

#ifndef LIBTEXTCLASSIFIER_UTIL_I18N_LOCALE_H_
#define LIBTEXTCLASSIFIER_UTIL_I18N_LOCALE_H_

#include <string>

#include "util/base/integral_types.h"

namespace libtextclassifier2 {

class Locale {
 public:
  // Constructs the object from a valid BCP47 tag. If the tag is invalid,
  // an object is created that gives false when IsInvalid() is called.
  static Locale FromBCP47(const std::string& locale_tag);

  // Creates a prototypical invalid locale object.
  static Locale Invalid() {
    Locale locale(/*language=*/"", /*script=*/"", /*region=*/"");
    locale.is_valid_ = false;
    return locale;
  }

  std::string Language() const { return language_; }

  std::string Script() const { return script_; }

  std::string Region() const { return region_; }

  bool IsValid() const { return is_valid_; }

 private:
  Locale(const std::string& language, const std::string& script,
         const std::string& region)
      : language_(language),
        script_(script),
        region_(region),
        is_valid_(true) {}

  std::string language_;
  std::string script_;
  std::string region_;
  bool is_valid_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_UTIL_I18N_LOCALE_H_
