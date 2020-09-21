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

#include "strip-unpaired-brackets.h"

#include <iterator>

#include "util/base/logging.h"
#include "util/utf8/unicodetext.h"

namespace libtextclassifier2 {
namespace {

// Returns true if given codepoint is contained in the given span in context.
bool IsCodepointInSpan(const char32 codepoint,
                       const UnicodeText& context_unicode,
                       const CodepointSpan span) {
  auto begin_it = context_unicode.begin();
  std::advance(begin_it, span.first);
  auto end_it = context_unicode.begin();
  std::advance(end_it, span.second);

  return std::find(begin_it, end_it, codepoint) != end_it;
}

// Returns the first codepoint of the span.
char32 FirstSpanCodepoint(const UnicodeText& context_unicode,
                          const CodepointSpan span) {
  auto it = context_unicode.begin();
  std::advance(it, span.first);
  return *it;
}

// Returns the last codepoint of the span.
char32 LastSpanCodepoint(const UnicodeText& context_unicode,
                         const CodepointSpan span) {
  auto it = context_unicode.begin();
  std::advance(it, span.second - 1);
  return *it;
}

}  // namespace

CodepointSpan StripUnpairedBrackets(const std::string& context,
                                    CodepointSpan span, const UniLib& unilib) {
  const UnicodeText context_unicode =
      UTF8ToUnicodeText(context, /*do_copy=*/false);
  return StripUnpairedBrackets(context_unicode, span, unilib);
}

// If the first or the last codepoint of the given span is a bracket, the
// bracket is stripped if the span does not contain its corresponding paired
// version.
CodepointSpan StripUnpairedBrackets(const UnicodeText& context_unicode,
                                    CodepointSpan span, const UniLib& unilib) {
  if (context_unicode.empty() || !ValidNonEmptySpan(span)) {
    return span;
  }

  const char32 begin_char = FirstSpanCodepoint(context_unicode, span);
  const char32 paired_begin_char = unilib.GetPairedBracket(begin_char);
  if (paired_begin_char != begin_char) {
    if (!unilib.IsOpeningBracket(begin_char) ||
        !IsCodepointInSpan(paired_begin_char, context_unicode, span)) {
      ++span.first;
    }
  }

  if (span.first == span.second) {
    return span;
  }

  const char32 end_char = LastSpanCodepoint(context_unicode, span);
  const char32 paired_end_char = unilib.GetPairedBracket(end_char);
  if (paired_end_char != end_char) {
    if (!unilib.IsClosingBracket(end_char) ||
        !IsCodepointInSpan(paired_end_char, context_unicode, span)) {
      --span.second;
    }
  }

  // Should not happen, but let's make sure.
  if (span.first > span.second) {
    TC_LOG(WARNING) << "Inverse indices result: " << span.first << ", "
                    << span.second;
    span.second = span.first;
  }

  return span;
}

}  // namespace libtextclassifier2
