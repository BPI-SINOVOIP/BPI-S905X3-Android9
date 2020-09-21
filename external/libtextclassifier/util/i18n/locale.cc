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

#include "util/i18n/locale.h"

#include "util/strings/split.h"

namespace libtextclassifier2 {

namespace {

bool CheckLanguage(StringPiece language) {
  if (language.size() != 2 && language.size() != 3) {
    return false;
  }

  // Needs to be all lowercase.
  for (int i = 0; i < language.size(); ++i) {
    if (!std::islower(language[i])) {
      return false;
    }
  }

  return true;
}

bool CheckScript(StringPiece script) {
  if (script.size() != 4) {
    return false;
  }

  if (!std::isupper(script[0])) {
    return false;
  }

  // Needs to be all lowercase.
  for (int i = 1; i < script.size(); ++i) {
    if (!std::islower(script[i])) {
      return false;
    }
  }

  return true;
}

bool CheckRegion(StringPiece region) {
  if (region.size() == 2) {
    return std::isupper(region[0]) && std::isupper(region[1]);
  } else if (region.size() == 3) {
    return std::isdigit(region[0]) && std::isdigit(region[1]) &&
           std::isdigit(region[2]);
  } else {
    return false;
  }
}

}  // namespace

Locale Locale::FromBCP47(const std::string& locale_tag) {
  std::vector<StringPiece> parts = strings::Split(locale_tag, '-');
  if (parts.empty()) {
    return Locale::Invalid();
  }

  auto parts_it = parts.begin();
  StringPiece language = *parts_it;
  if (!CheckLanguage(language)) {
    return Locale::Invalid();
  }
  ++parts_it;

  StringPiece script;
  if (parts_it != parts.end()) {
    script = *parts_it;
    if (!CheckScript(script)) {
      script = "";
    } else {
      ++parts_it;
    }
  }

  StringPiece region;
  if (parts_it != parts.end()) {
    region = *parts_it;
    if (!CheckRegion(region)) {
      region = "";
    } else {
      ++parts_it;
    }
  }

  // NOTE: We don't parse the rest of the BCP47 tag here even if specified.

  return Locale(language.ToString(), script.ToString(), region.ToString());
}

}  // namespace libtextclassifier2
