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

#include "util/utf8/unilib-icu.h"

#include <utility>

namespace libtextclassifier2 {

bool UniLib::ParseInt32(const UnicodeText& text, int* result) const {
  UErrorCode status = U_ZERO_ERROR;
  UNumberFormat* format_alias =
      unum_open(UNUM_DECIMAL, nullptr, 0, "en_US_POSIX", nullptr, &status);
  if (U_FAILURE(status)) {
    return false;
  }
  icu::UnicodeString utf8_string = icu::UnicodeString::fromUTF8(
      icu::StringPiece(text.data(), text.size_bytes()));
  int parse_index = 0;
  const int32 integer = unum_parse(format_alias, utf8_string.getBuffer(),
                                   utf8_string.length(), &parse_index, &status);
  *result = integer;
  unum_close(format_alias);
  if (U_FAILURE(status) || parse_index != utf8_string.length()) {
    return false;
  }
  return true;
}

bool UniLib::IsOpeningBracket(char32 codepoint) const {
  return u_getIntPropertyValue(codepoint, UCHAR_BIDI_PAIRED_BRACKET_TYPE) ==
         U_BPT_OPEN;
}

bool UniLib::IsClosingBracket(char32 codepoint) const {
  return u_getIntPropertyValue(codepoint, UCHAR_BIDI_PAIRED_BRACKET_TYPE) ==
         U_BPT_CLOSE;
}

bool UniLib::IsWhitespace(char32 codepoint) const {
  return u_isWhitespace(codepoint);
}

bool UniLib::IsDigit(char32 codepoint) const { return u_isdigit(codepoint); }

bool UniLib::IsUpper(char32 codepoint) const { return u_isupper(codepoint); }

char32 UniLib::ToLower(char32 codepoint) const { return u_tolower(codepoint); }

char32 UniLib::GetPairedBracket(char32 codepoint) const {
  return u_getBidiPairedBracket(codepoint);
}

UniLib::RegexMatcher::RegexMatcher(icu::RegexPattern* pattern,
                                   icu::UnicodeString text)
    : text_(std::move(text)),
      last_find_offset_(0),
      last_find_offset_codepoints_(0),
      last_find_offset_dirty_(true) {
  UErrorCode status = U_ZERO_ERROR;
  matcher_.reset(pattern->matcher(text_, status));
  if (U_FAILURE(status)) {
    matcher_.reset(nullptr);
  }
}

std::unique_ptr<UniLib::RegexMatcher> UniLib::RegexPattern::Matcher(
    const UnicodeText& input) const {
  return std::unique_ptr<UniLib::RegexMatcher>(new UniLib::RegexMatcher(
      pattern_.get(), icu::UnicodeString::fromUTF8(
                          icu::StringPiece(input.data(), input.size_bytes()))));
}

constexpr int UniLib::RegexMatcher::kError;
constexpr int UniLib::RegexMatcher::kNoError;

bool UniLib::RegexMatcher::Matches(int* status) const {
  if (!matcher_) {
    *status = kError;
    return false;
  }

  UErrorCode icu_status = U_ZERO_ERROR;
  const bool result = matcher_->matches(/*startIndex=*/0, icu_status);
  if (U_FAILURE(icu_status)) {
    *status = kError;
    return false;
  }
  *status = kNoError;
  return result;
}

bool UniLib::RegexMatcher::ApproximatelyMatches(int* status) {
  if (!matcher_) {
    *status = kError;
    return false;
  }

  matcher_->reset();
  *status = kNoError;
  if (!Find(status) || *status != kNoError) {
    return false;
  }
  const int found_start = Start(status);
  if (*status != kNoError) {
    return false;
  }
  const int found_end = End(status);
  if (*status != kNoError) {
    return false;
  }
  if (found_start != 0 || found_end != text_.countChar32()) {
    return false;
  }
  return true;
}

bool UniLib::RegexMatcher::UpdateLastFindOffset() const {
  if (!last_find_offset_dirty_) {
    return true;
  }

  // Update the position of the match.
  UErrorCode icu_status = U_ZERO_ERROR;
  const int find_offset = matcher_->start(0, icu_status);
  if (U_FAILURE(icu_status)) {
    return false;
  }
  last_find_offset_codepoints_ +=
      text_.countChar32(last_find_offset_, find_offset - last_find_offset_);
  last_find_offset_ = find_offset;
  last_find_offset_dirty_ = false;

  return true;
}

bool UniLib::RegexMatcher::Find(int* status) {
  if (!matcher_) {
    *status = kError;
    return false;
  }
  UErrorCode icu_status = U_ZERO_ERROR;
  const bool result = matcher_->find(icu_status);
  if (U_FAILURE(icu_status)) {
    *status = kError;
    return false;
  }

  last_find_offset_dirty_ = true;
  *status = kNoError;
  return result;
}

int UniLib::RegexMatcher::Start(int* status) const {
  return Start(/*group_idx=*/0, status);
}

int UniLib::RegexMatcher::Start(int group_idx, int* status) const {
  if (!matcher_ || !UpdateLastFindOffset()) {
    *status = kError;
    return kError;
  }

  UErrorCode icu_status = U_ZERO_ERROR;
  const int result = matcher_->start(group_idx, icu_status);
  if (U_FAILURE(icu_status)) {
    *status = kError;
    return kError;
  }
  *status = kNoError;

  // If the group didn't participate in the match the result is -1 and is
  // incompatible with the caching logic bellow.
  if (result == -1) {
    return -1;
  }

  return last_find_offset_codepoints_ +
         text_.countChar32(/*start=*/last_find_offset_,
                           /*length=*/result - last_find_offset_);
}

int UniLib::RegexMatcher::End(int* status) const {
  return End(/*group_idx=*/0, status);
}

int UniLib::RegexMatcher::End(int group_idx, int* status) const {
  if (!matcher_ || !UpdateLastFindOffset()) {
    *status = kError;
    return kError;
  }
  UErrorCode icu_status = U_ZERO_ERROR;
  const int result = matcher_->end(group_idx, icu_status);
  if (U_FAILURE(icu_status)) {
    *status = kError;
    return kError;
  }
  *status = kNoError;

  // If the group didn't participate in the match the result is -1 and is
  // incompatible with the caching logic bellow.
  if (result == -1) {
    return -1;
  }

  return last_find_offset_codepoints_ +
         text_.countChar32(/*start=*/last_find_offset_,
                           /*length=*/result - last_find_offset_);
}

UnicodeText UniLib::RegexMatcher::Group(int* status) const {
  return Group(/*group_idx=*/0, status);
}

UnicodeText UniLib::RegexMatcher::Group(int group_idx, int* status) const {
  if (!matcher_) {
    *status = kError;
    return UTF8ToUnicodeText("", /*do_copy=*/false);
  }
  std::string result = "";
  UErrorCode icu_status = U_ZERO_ERROR;
  const icu::UnicodeString result_icu = matcher_->group(group_idx, icu_status);
  if (U_FAILURE(icu_status)) {
    *status = kError;
    return UTF8ToUnicodeText("", /*do_copy=*/false);
  }
  result_icu.toUTF8String(result);
  *status = kNoError;
  return UTF8ToUnicodeText(result, /*do_copy=*/true);
}

constexpr int UniLib::BreakIterator::kDone;

UniLib::BreakIterator::BreakIterator(const UnicodeText& text)
    : text_(icu::UnicodeString::fromUTF8(
          icu::StringPiece(text.data(), text.size_bytes()))),
      last_break_index_(0),
      last_unicode_index_(0) {
  icu::ErrorCode status;
  break_iterator_.reset(
      icu::BreakIterator::createWordInstance(icu::Locale("en"), status));
  if (!status.isSuccess()) {
    break_iterator_.reset();
    return;
  }
  break_iterator_->setText(text_);
}

int UniLib::BreakIterator::Next() {
  const int break_index = break_iterator_->next();
  if (break_index == icu::BreakIterator::DONE) {
    return BreakIterator::kDone;
  }
  last_unicode_index_ +=
      text_.countChar32(last_break_index_, break_index - last_break_index_);
  last_break_index_ = break_index;
  return last_unicode_index_;
}

std::unique_ptr<UniLib::RegexPattern> UniLib::CreateRegexPattern(
    const UnicodeText& regex) const {
  UErrorCode status = U_ZERO_ERROR;
  std::unique_ptr<icu::RegexPattern> pattern(
      icu::RegexPattern::compile(icu::UnicodeString::fromUTF8(icu::StringPiece(
                                     regex.data(), regex.size_bytes())),
                                 /*flags=*/UREGEX_MULTILINE, status));
  if (U_FAILURE(status) || !pattern) {
    return nullptr;
  }
  return std::unique_ptr<UniLib::RegexPattern>(
      new UniLib::RegexPattern(std::move(pattern)));
}

std::unique_ptr<UniLib::BreakIterator> UniLib::CreateBreakIterator(
    const UnicodeText& text) const {
  return std::unique_ptr<UniLib::BreakIterator>(
      new UniLib::BreakIterator(text));
}

}  // namespace libtextclassifier2
