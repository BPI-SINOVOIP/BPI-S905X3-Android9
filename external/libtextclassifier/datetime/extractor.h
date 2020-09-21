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

#ifndef LIBTEXTCLASSIFIER_DATETIME_EXTRACTOR_H_
#define LIBTEXTCLASSIFIER_DATETIME_EXTRACTOR_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "model_generated.h"
#include "types.h"
#include "util/strings/stringpiece.h"
#include "util/utf8/unicodetext.h"
#include "util/utf8/unilib.h"

namespace libtextclassifier2 {

struct CompiledRule {
  // The compiled regular expression.
  std::unique_ptr<const UniLib::RegexPattern> compiled_regex;

  // The uncompiled pattern and information about the pattern groups.
  const DatetimeModelPattern_::Regex* regex;

  // DatetimeModelPattern which 'regex' is part of and comes from.
  const DatetimeModelPattern* pattern;
};

// A helper class for DatetimeParser that extracts structured data
// (DateParseDate) from the current match of the passed RegexMatcher.
class DatetimeExtractor {
 public:
  DatetimeExtractor(
      const CompiledRule& rule, const UniLib::RegexMatcher& matcher,
      int locale_id, const UniLib& unilib,
      const std::vector<std::unique_ptr<const UniLib::RegexPattern>>&
          extractor_rules,
      const std::unordered_map<DatetimeExtractorType,
                               std::unordered_map<int, int>>&
          type_and_locale_to_extractor_rule)
      : rule_(rule),
        matcher_(matcher),
        locale_id_(locale_id),
        unilib_(unilib),
        rules_(extractor_rules),
        type_and_locale_to_rule_(type_and_locale_to_extractor_rule) {}
  bool Extract(DateParseData* result, CodepointSpan* result_span) const;

 private:
  bool RuleIdForType(DatetimeExtractorType type, int* rule_id) const;

  // Returns true if the rule for given extractor matched. If it matched,
  // match_result will contain the first group of the rule (if match_result not
  // nullptr).
  bool ExtractType(const UnicodeText& input,
                   DatetimeExtractorType extractor_type,
                   UnicodeText* match_result = nullptr) const;

  bool GroupTextFromMatch(int group_id, UnicodeText* result) const;

  // Updates the span to include the current match for the given group.
  bool UpdateMatchSpan(int group_id, CodepointSpan* span) const;

  // Returns true if any of the extractors from 'mapping' matched. If it did,
  // will fill 'result' with the associated value from 'mapping'.
  template <typename T>
  bool MapInput(const UnicodeText& input,
                const std::vector<std::pair<DatetimeExtractorType, T>>& mapping,
                T* result) const;

  bool ParseDigits(const UnicodeText& input, int* parsed_digits) const;
  bool ParseWrittenNumber(const UnicodeText& input, int* parsed_number) const;
  bool ParseYear(const UnicodeText& input, int* parsed_year) const;
  bool ParseMonth(const UnicodeText& input, int* parsed_month) const;
  bool ParseAMPM(const UnicodeText& input, int* parsed_ampm) const;
  bool ParseRelation(const UnicodeText& input,
                     DateParseData::Relation* parsed_relation) const;
  bool ParseRelationDistance(const UnicodeText& input,
                             int* parsed_distance) const;
  bool ParseTimeUnit(const UnicodeText& input, int* parsed_time_unit) const;
  bool ParseRelationType(
      const UnicodeText& input,
      DateParseData::RelationType* parsed_relation_type) const;
  bool ParseWeekday(const UnicodeText& input, int* parsed_weekday) const;

  const CompiledRule& rule_;
  const UniLib::RegexMatcher& matcher_;
  int locale_id_;
  const UniLib& unilib_;
  const std::vector<std::unique_ptr<const UniLib::RegexPattern>>& rules_;
  const std::unordered_map<DatetimeExtractorType, std::unordered_map<int, int>>&
      type_and_locale_to_rule_;
};

}  // namespace libtextclassifier2

#endif  // LIBTEXTCLASSIFIER_DATETIME_EXTRACTOR_H_
