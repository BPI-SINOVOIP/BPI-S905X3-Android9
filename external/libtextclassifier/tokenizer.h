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

#ifndef LIBTEXTCLASSIFIER_TOKENIZER_H_
#define LIBTEXTCLASSIFIER_TOKENIZER_H_

#include <string>
#include <vector>

#include "model_generated.h"
#include "types.h"
#include "util/base/integral_types.h"
#include "util/utf8/unicodetext.h"

namespace libtextclassifier2 {

const int kInvalidScript = -1;
const int kUnknownScript = -2;

// Tokenizer splits the input string into a sequence of tokens, according to the
// configuration.
class Tokenizer {
 public:
  explicit Tokenizer(
      const std::vector<const TokenizationCodepointRange*>& codepoint_ranges,
      bool split_on_script_change);

  // Tokenizes the input string using the selected tokenization method.
  std::vector<Token> Tokenize(const std::string& text) const;

  // Same as above but takes UnicodeText.
  std::vector<Token> Tokenize(const UnicodeText& text_unicode) const;

 protected:
  // Finds the tokenization codepoint range config for given codepoint.
  // Internally uses binary search so should be O(log(# of codepoint_ranges)).
  const TokenizationCodepointRangeT* FindTokenizationRange(int codepoint) const;

  // Finds the role and script for given codepoint. If not found, DEFAULT_ROLE
  // and kUnknownScript are assigned.
  void GetScriptAndRole(char32 codepoint,
                        TokenizationCodepointRange_::Role* role,
                        int* script) const;

 private:
  // Codepoint ranges that determine how different codepoints are tokenized.
  // The ranges must not overlap.
  std::vector<std::unique_ptr<const TokenizationCodepointRangeT>>
      codepoint_ranges_;

  // If true, tokens will be additionally split when the codepoint's script_id
  // changes.
  bool split_on_script_change_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_TOKENIZER_H_
