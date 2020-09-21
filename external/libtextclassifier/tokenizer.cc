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

#include "tokenizer.h"

#include <algorithm>

#include "util/base/logging.h"
#include "util/strings/utf8.h"

namespace libtextclassifier2 {

Tokenizer::Tokenizer(
    const std::vector<const TokenizationCodepointRange*>& codepoint_ranges,
    bool split_on_script_change)
    : split_on_script_change_(split_on_script_change) {
  for (const TokenizationCodepointRange* range : codepoint_ranges) {
    codepoint_ranges_.emplace_back(range->UnPack());
  }

  std::sort(codepoint_ranges_.begin(), codepoint_ranges_.end(),
            [](const std::unique_ptr<const TokenizationCodepointRangeT>& a,
               const std::unique_ptr<const TokenizationCodepointRangeT>& b) {
              return a->start < b->start;
            });
}

const TokenizationCodepointRangeT* Tokenizer::FindTokenizationRange(
    int codepoint) const {
  auto it = std::lower_bound(
      codepoint_ranges_.begin(), codepoint_ranges_.end(), codepoint,
      [](const std::unique_ptr<const TokenizationCodepointRangeT>& range,
         int codepoint) {
        // This function compares range with the codepoint for the purpose of
        // finding the first greater or equal range. Because of the use of
        // std::lower_bound it needs to return true when range < codepoint;
        // the first time it will return false the lower bound is found and
        // returned.
        //
        // It might seem weird that the condition is range.end <= codepoint
        // here but when codepoint == range.end it means it's actually just
        // outside of the range, thus the range is less than the codepoint.
        return range->end <= codepoint;
      });
  if (it != codepoint_ranges_.end() && (*it)->start <= codepoint &&
      (*it)->end > codepoint) {
    return it->get();
  } else {
    return nullptr;
  }
}

void Tokenizer::GetScriptAndRole(char32 codepoint,
                                 TokenizationCodepointRange_::Role* role,
                                 int* script) const {
  const TokenizationCodepointRangeT* range = FindTokenizationRange(codepoint);
  if (range) {
    *role = range->role;
    *script = range->script_id;
  } else {
    *role = TokenizationCodepointRange_::Role_DEFAULT_ROLE;
    *script = kUnknownScript;
  }
}

std::vector<Token> Tokenizer::Tokenize(const std::string& text) const {
  UnicodeText text_unicode = UTF8ToUnicodeText(text, /*do_copy=*/false);
  return Tokenize(text_unicode);
}

std::vector<Token> Tokenizer::Tokenize(const UnicodeText& text_unicode) const {
  std::vector<Token> result;
  Token new_token("", 0, 0);
  int codepoint_index = 0;

  int last_script = kInvalidScript;
  for (auto it = text_unicode.begin(); it != text_unicode.end();
       ++it, ++codepoint_index) {
    TokenizationCodepointRange_::Role role;
    int script;
    GetScriptAndRole(*it, &role, &script);

    if (role & TokenizationCodepointRange_::Role_SPLIT_BEFORE ||
        (split_on_script_change_ && last_script != kInvalidScript &&
         last_script != script)) {
      if (!new_token.value.empty()) {
        result.push_back(new_token);
      }
      new_token = Token("", codepoint_index, codepoint_index);
    }
    if (!(role & TokenizationCodepointRange_::Role_DISCARD_CODEPOINT)) {
      new_token.value += std::string(
          it.utf8_data(),
          it.utf8_data() + GetNumBytesForNonZeroUTF8Char(it.utf8_data()));
      ++new_token.end;
    }
    if (role & TokenizationCodepointRange_::Role_SPLIT_AFTER) {
      if (!new_token.value.empty()) {
        result.push_back(new_token);
      }
      new_token = Token("", codepoint_index + 1, codepoint_index + 1);
    }

    last_script = script;
  }
  if (!new_token.value.empty()) {
    result.push_back(new_token);
  }

  return result;
}

}  // namespace libtextclassifier2
