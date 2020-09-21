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

#include "token-feature-extractor.h"

#include <cctype>
#include <string>

#include "util/base/logging.h"
#include "util/hash/farmhash.h"
#include "util/strings/stringpiece.h"
#include "util/utf8/unicodetext.h"

namespace libtextclassifier2 {

namespace {

std::string RemapTokenAscii(const std::string& token,
                            const TokenFeatureExtractorOptions& options) {
  if (!options.remap_digits && !options.lowercase_tokens) {
    return token;
  }

  std::string copy = token;
  for (int i = 0; i < token.size(); ++i) {
    if (options.remap_digits && isdigit(copy[i])) {
      copy[i] = '0';
    }
    if (options.lowercase_tokens) {
      copy[i] = tolower(copy[i]);
    }
  }
  return copy;
}

void RemapTokenUnicode(const std::string& token,
                       const TokenFeatureExtractorOptions& options,
                       const UniLib& unilib, UnicodeText* remapped) {
  if (!options.remap_digits && !options.lowercase_tokens) {
    // Leave remapped untouched.
    return;
  }

  UnicodeText word = UTF8ToUnicodeText(token, /*do_copy=*/false);
  remapped->clear();
  for (auto it = word.begin(); it != word.end(); ++it) {
    if (options.remap_digits && unilib.IsDigit(*it)) {
      remapped->AppendCodepoint('0');
    } else if (options.lowercase_tokens) {
      remapped->AppendCodepoint(unilib.ToLower(*it));
    } else {
      remapped->AppendCodepoint(*it);
    }
  }
}

}  // namespace

TokenFeatureExtractor::TokenFeatureExtractor(
    const TokenFeatureExtractorOptions& options, const UniLib& unilib)
    : options_(options), unilib_(unilib) {
  for (const std::string& pattern : options.regexp_features) {
    regex_patterns_.push_back(std::unique_ptr<UniLib::RegexPattern>(
        unilib_.CreateRegexPattern(UTF8ToUnicodeText(
            pattern.c_str(), pattern.size(), /*do_copy=*/false))));
  }
}

bool TokenFeatureExtractor::Extract(const Token& token, bool is_in_span,
                                    std::vector<int>* sparse_features,
                                    std::vector<float>* dense_features) const {
  if (!dense_features) {
    return false;
  }
  if (sparse_features) {
    *sparse_features = ExtractCharactergramFeatures(token);
  }
  *dense_features = ExtractDenseFeatures(token, is_in_span);
  return true;
}

std::vector<int> TokenFeatureExtractor::ExtractCharactergramFeatures(
    const Token& token) const {
  if (options_.unicode_aware_features) {
    return ExtractCharactergramFeaturesUnicode(token);
  } else {
    return ExtractCharactergramFeaturesAscii(token);
  }
}

std::vector<float> TokenFeatureExtractor::ExtractDenseFeatures(
    const Token& token, bool is_in_span) const {
  std::vector<float> dense_features;

  if (options_.extract_case_feature) {
    if (options_.unicode_aware_features) {
      UnicodeText token_unicode =
          UTF8ToUnicodeText(token.value, /*do_copy=*/false);
      const bool is_upper = unilib_.IsUpper(*token_unicode.begin());
      if (!token.value.empty() && is_upper) {
        dense_features.push_back(1.0);
      } else {
        dense_features.push_back(-1.0);
      }
    } else {
      if (!token.value.empty() && isupper(*token.value.begin())) {
        dense_features.push_back(1.0);
      } else {
        dense_features.push_back(-1.0);
      }
    }
  }

  if (options_.extract_selection_mask_feature) {
    if (is_in_span) {
      dense_features.push_back(1.0);
    } else {
      if (options_.unicode_aware_features) {
        dense_features.push_back(-1.0);
      } else {
        dense_features.push_back(0.0);
      }
    }
  }

  // Add regexp features.
  if (!regex_patterns_.empty()) {
    UnicodeText token_unicode =
        UTF8ToUnicodeText(token.value, /*do_copy=*/false);
    for (int i = 0; i < regex_patterns_.size(); ++i) {
      if (!regex_patterns_[i].get()) {
        dense_features.push_back(-1.0);
        continue;
      }
      auto matcher = regex_patterns_[i]->Matcher(token_unicode);
      int status;
      if (matcher->Matches(&status)) {
        dense_features.push_back(1.0);
      } else {
        dense_features.push_back(-1.0);
      }
    }
  }

  return dense_features;
}

int TokenFeatureExtractor::HashToken(StringPiece token) const {
  if (options_.allowed_chargrams.empty()) {
    return tc2farmhash::Fingerprint64(token) % options_.num_buckets;
  } else {
    // Padding and out-of-vocabulary tokens have extra buckets reserved because
    // they are special and important tokens, and we don't want them to share
    // embedding with other charactergrams.
    // TODO(zilka): Experimentally verify.
    const int kNumExtraBuckets = 2;
    const std::string token_string = token.ToString();
    if (token_string == "<PAD>") {
      return 1;
    } else if (options_.allowed_chargrams.find(token_string) ==
               options_.allowed_chargrams.end()) {
      return 0;  // Out-of-vocabulary.
    } else {
      return (tc2farmhash::Fingerprint64(token) %
              (options_.num_buckets - kNumExtraBuckets)) +
             kNumExtraBuckets;
    }
  }
}

std::vector<int> TokenFeatureExtractor::ExtractCharactergramFeaturesAscii(
    const Token& token) const {
  std::vector<int> result;
  if (token.is_padding || token.value.empty()) {
    result.push_back(HashToken("<PAD>"));
  } else {
    const std::string word = RemapTokenAscii(token.value, options_);

    // Trim words that are over max_word_length characters.
    const int max_word_length = options_.max_word_length;
    std::string feature_word;
    if (word.size() > max_word_length) {
      feature_word =
          "^" + word.substr(0, max_word_length / 2) + "\1" +
          word.substr(word.size() - max_word_length / 2, max_word_length / 2) +
          "$";
    } else {
      // Add a prefix and suffix to the word.
      feature_word = "^" + word + "$";
    }

    // Upper-bound the number of charactergram extracted to avoid resizing.
    result.reserve(options_.chargram_orders.size() * feature_word.size());

    if (options_.chargram_orders.empty()) {
      result.push_back(HashToken(feature_word));
    } else {
      // Generate the character-grams.
      for (int chargram_order : options_.chargram_orders) {
        if (chargram_order == 1) {
          for (int i = 1; i < feature_word.size() - 1; ++i) {
            result.push_back(
                HashToken(StringPiece(feature_word, /*offset=*/i, /*len=*/1)));
          }
        } else {
          for (int i = 0;
               i < static_cast<int>(feature_word.size()) - chargram_order + 1;
               ++i) {
            result.push_back(HashToken(StringPiece(feature_word, /*offset=*/i,
                                                   /*len=*/chargram_order)));
          }
        }
      }
    }
  }
  return result;
}

std::vector<int> TokenFeatureExtractor::ExtractCharactergramFeaturesUnicode(
    const Token& token) const {
  std::vector<int> result;
  if (token.is_padding || token.value.empty()) {
    result.push_back(HashToken("<PAD>"));
  } else {
    UnicodeText word = UTF8ToUnicodeText(token.value, /*do_copy=*/false);
    RemapTokenUnicode(token.value, options_, unilib_, &word);

    // Trim the word if needed by finding a left-cut point and right-cut point.
    auto left_cut = word.begin();
    auto right_cut = word.end();
    for (int i = 0; i < options_.max_word_length / 2; i++) {
      if (left_cut < right_cut) {
        ++left_cut;
      }
      if (left_cut < right_cut) {
        --right_cut;
      }
    }

    std::string feature_word;
    if (left_cut == right_cut) {
      feature_word = "^" + word.UTF8Substring(word.begin(), word.end()) + "$";
    } else {
      // clang-format off
      feature_word = "^" +
                     word.UTF8Substring(word.begin(), left_cut) +
                     "\1" +
                     word.UTF8Substring(right_cut, word.end()) +
                     "$";
      // clang-format on
    }

    const UnicodeText feature_word_unicode =
        UTF8ToUnicodeText(feature_word, /*do_copy=*/false);

    // Upper-bound the number of charactergram extracted to avoid resizing.
    result.reserve(options_.chargram_orders.size() * feature_word.size());

    if (options_.chargram_orders.empty()) {
      result.push_back(HashToken(feature_word));
    } else {
      // Generate the character-grams.
      for (int chargram_order : options_.chargram_orders) {
        UnicodeText::const_iterator it_start = feature_word_unicode.begin();
        UnicodeText::const_iterator it_end = feature_word_unicode.end();
        if (chargram_order == 1) {
          ++it_start;
          --it_end;
        }

        UnicodeText::const_iterator it_chargram_start = it_start;
        UnicodeText::const_iterator it_chargram_end = it_start;
        bool chargram_is_complete = true;
        for (int i = 0; i < chargram_order; ++i) {
          if (it_chargram_end == it_end) {
            chargram_is_complete = false;
            break;
          }
          ++it_chargram_end;
        }
        if (!chargram_is_complete) {
          continue;
        }

        for (; it_chargram_end <= it_end;
             ++it_chargram_start, ++it_chargram_end) {
          const int length_bytes =
              it_chargram_end.utf8_data() - it_chargram_start.utf8_data();
          result.push_back(HashToken(
              StringPiece(it_chargram_start.utf8_data(), length_bytes)));
        }
      }
    }
  }
  return result;
}

}  // namespace libtextclassifier2
