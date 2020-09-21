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

#include "datetime/parser.h"

#include <set>
#include <unordered_set>

#include "datetime/extractor.h"
#include "util/calendar/calendar.h"
#include "util/i18n/locale.h"
#include "util/strings/split.h"

namespace libtextclassifier2 {
std::unique_ptr<DatetimeParser> DatetimeParser::Instance(
    const DatetimeModel* model, const UniLib& unilib,
    ZlibDecompressor* decompressor) {
  std::unique_ptr<DatetimeParser> result(
      new DatetimeParser(model, unilib, decompressor));
  if (!result->initialized_) {
    result.reset();
  }
  return result;
}

DatetimeParser::DatetimeParser(const DatetimeModel* model, const UniLib& unilib,
                               ZlibDecompressor* decompressor)
    : unilib_(unilib) {
  initialized_ = false;

  if (model == nullptr) {
    return;
  }

  if (model->patterns() != nullptr) {
    for (const DatetimeModelPattern* pattern : *model->patterns()) {
      if (pattern->regexes()) {
        for (const DatetimeModelPattern_::Regex* regex : *pattern->regexes()) {
          std::unique_ptr<UniLib::RegexPattern> regex_pattern =
              UncompressMakeRegexPattern(unilib, regex->pattern(),
                                         regex->compressed_pattern(),
                                         decompressor);
          if (!regex_pattern) {
            TC_LOG(ERROR) << "Couldn't create rule pattern.";
            return;
          }
          rules_.push_back({std::move(regex_pattern), regex, pattern});
          if (pattern->locales()) {
            for (int locale : *pattern->locales()) {
              locale_to_rules_[locale].push_back(rules_.size() - 1);
            }
          }
        }
      }
    }
  }

  if (model->extractors() != nullptr) {
    for (const DatetimeModelExtractor* extractor : *model->extractors()) {
      std::unique_ptr<UniLib::RegexPattern> regex_pattern =
          UncompressMakeRegexPattern(unilib, extractor->pattern(),
                                     extractor->compressed_pattern(),
                                     decompressor);
      if (!regex_pattern) {
        TC_LOG(ERROR) << "Couldn't create extractor pattern";
        return;
      }
      extractor_rules_.push_back(std::move(regex_pattern));

      if (extractor->locales()) {
        for (int locale : *extractor->locales()) {
          type_and_locale_to_extractor_rule_[extractor->extractor()][locale] =
              extractor_rules_.size() - 1;
        }
      }
    }
  }

  if (model->locales() != nullptr) {
    for (int i = 0; i < model->locales()->Length(); ++i) {
      locale_string_to_id_[model->locales()->Get(i)->str()] = i;
    }
  }

  if (model->default_locales() != nullptr) {
    for (const int locale : *model->default_locales()) {
      default_locale_ids_.push_back(locale);
    }
  }

  use_extractors_for_locating_ = model->use_extractors_for_locating();

  initialized_ = true;
}

bool DatetimeParser::Parse(
    const std::string& input, const int64 reference_time_ms_utc,
    const std::string& reference_timezone, const std::string& locales,
    ModeFlag mode, bool anchor_start_end,
    std::vector<DatetimeParseResultSpan>* results) const {
  return Parse(UTF8ToUnicodeText(input, /*do_copy=*/false),
               reference_time_ms_utc, reference_timezone, locales, mode,
               anchor_start_end, results);
}

bool DatetimeParser::FindSpansUsingLocales(
    const std::vector<int>& locale_ids, const UnicodeText& input,
    const int64 reference_time_ms_utc, const std::string& reference_timezone,
    ModeFlag mode, bool anchor_start_end, const std::string& reference_locale,
    std::unordered_set<int>* executed_rules,
    std::vector<DatetimeParseResultSpan>* found_spans) const {
  for (const int locale_id : locale_ids) {
    auto rules_it = locale_to_rules_.find(locale_id);
    if (rules_it == locale_to_rules_.end()) {
      continue;
    }

    for (const int rule_id : rules_it->second) {
      // Skip rules that were already executed in previous locales.
      if (executed_rules->find(rule_id) != executed_rules->end()) {
        continue;
      }

      if (!(rules_[rule_id].pattern->enabled_modes() & mode)) {
        continue;
      }

      executed_rules->insert(rule_id);

      if (!ParseWithRule(rules_[rule_id], input, reference_time_ms_utc,
                         reference_timezone, reference_locale, locale_id,
                         anchor_start_end, found_spans)) {
        return false;
      }
    }
  }
  return true;
}

bool DatetimeParser::Parse(
    const UnicodeText& input, const int64 reference_time_ms_utc,
    const std::string& reference_timezone, const std::string& locales,
    ModeFlag mode, bool anchor_start_end,
    std::vector<DatetimeParseResultSpan>* results) const {
  std::vector<DatetimeParseResultSpan> found_spans;
  std::unordered_set<int> executed_rules;
  std::string reference_locale;
  const std::vector<int> requested_locales =
      ParseAndExpandLocales(locales, &reference_locale);
  if (!FindSpansUsingLocales(requested_locales, input, reference_time_ms_utc,
                             reference_timezone, mode, anchor_start_end,
                             reference_locale, &executed_rules, &found_spans)) {
    return false;
  }

  std::vector<std::pair<DatetimeParseResultSpan, int>> indexed_found_spans;
  int counter = 0;
  for (const auto& found_span : found_spans) {
    indexed_found_spans.push_back({found_span, counter});
    counter++;
  }

  // Resolve conflicts by always picking the longer span and breaking ties by
  // selecting the earlier entry in the list for a given locale.
  std::sort(indexed_found_spans.begin(), indexed_found_spans.end(),
            [](const std::pair<DatetimeParseResultSpan, int>& a,
               const std::pair<DatetimeParseResultSpan, int>& b) {
              if ((a.first.span.second - a.first.span.first) !=
                  (b.first.span.second - b.first.span.first)) {
                return (a.first.span.second - a.first.span.first) >
                       (b.first.span.second - b.first.span.first);
              } else {
                return a.second < b.second;
              }
            });

  found_spans.clear();
  for (auto& span_index_pair : indexed_found_spans) {
    found_spans.push_back(span_index_pair.first);
  }

  std::set<int, std::function<bool(int, int)>> chosen_indices_set(
      [&found_spans](int a, int b) {
        return found_spans[a].span.first < found_spans[b].span.first;
      });
  for (int i = 0; i < found_spans.size(); ++i) {
    if (!DoesCandidateConflict(i, found_spans, chosen_indices_set)) {
      chosen_indices_set.insert(i);
      results->push_back(found_spans[i]);
    }
  }

  return true;
}

bool DatetimeParser::HandleParseMatch(
    const CompiledRule& rule, const UniLib::RegexMatcher& matcher,
    int64 reference_time_ms_utc, const std::string& reference_timezone,
    const std::string& reference_locale, int locale_id,
    std::vector<DatetimeParseResultSpan>* result) const {
  int status = UniLib::RegexMatcher::kNoError;
  const int start = matcher.Start(&status);
  if (status != UniLib::RegexMatcher::kNoError) {
    return false;
  }

  const int end = matcher.End(&status);
  if (status != UniLib::RegexMatcher::kNoError) {
    return false;
  }

  DatetimeParseResultSpan parse_result;
  if (!ExtractDatetime(rule, matcher, reference_time_ms_utc, reference_timezone,
                       reference_locale, locale_id, &(parse_result.data),
                       &parse_result.span)) {
    return false;
  }
  if (!use_extractors_for_locating_) {
    parse_result.span = {start, end};
  }
  if (parse_result.span.first != kInvalidIndex &&
      parse_result.span.second != kInvalidIndex) {
    parse_result.target_classification_score =
        rule.pattern->target_classification_score();
    parse_result.priority_score = rule.pattern->priority_score();
    result->push_back(parse_result);
  }
  return true;
}

bool DatetimeParser::ParseWithRule(
    const CompiledRule& rule, const UnicodeText& input,
    const int64 reference_time_ms_utc, const std::string& reference_timezone,
    const std::string& reference_locale, const int locale_id,
    bool anchor_start_end, std::vector<DatetimeParseResultSpan>* result) const {
  std::unique_ptr<UniLib::RegexMatcher> matcher =
      rule.compiled_regex->Matcher(input);
  int status = UniLib::RegexMatcher::kNoError;
  if (anchor_start_end) {
    if (matcher->Matches(&status) && status == UniLib::RegexMatcher::kNoError) {
      if (!HandleParseMatch(rule, *matcher, reference_time_ms_utc,
                            reference_timezone, reference_locale, locale_id,
                            result)) {
        return false;
      }
    }
  } else {
    while (matcher->Find(&status) && status == UniLib::RegexMatcher::kNoError) {
      if (!HandleParseMatch(rule, *matcher, reference_time_ms_utc,
                            reference_timezone, reference_locale, locale_id,
                            result)) {
        return false;
      }
    }
  }
  return true;
}

std::vector<int> DatetimeParser::ParseAndExpandLocales(
    const std::string& locales, std::string* reference_locale) const {
  std::vector<StringPiece> split_locales = strings::Split(locales, ',');
  if (!split_locales.empty()) {
    *reference_locale = split_locales[0].ToString();
  } else {
    *reference_locale = "";
  }

  std::vector<int> result;
  for (const StringPiece& locale_str : split_locales) {
    auto locale_it = locale_string_to_id_.find(locale_str.ToString());
    if (locale_it != locale_string_to_id_.end()) {
      result.push_back(locale_it->second);
    }

    const Locale locale = Locale::FromBCP47(locale_str.ToString());
    if (!locale.IsValid()) {
      continue;
    }

    const std::string language = locale.Language();
    const std::string script = locale.Script();
    const std::string region = locale.Region();

    // First, try adding *-region locale.
    if (!region.empty()) {
      locale_it = locale_string_to_id_.find("*-" + region);
      if (locale_it != locale_string_to_id_.end()) {
        result.push_back(locale_it->second);
      }
    }
    // Second, try adding language-script-* locale.
    if (!script.empty()) {
      locale_it = locale_string_to_id_.find(language + "-" + script + "-*");
      if (locale_it != locale_string_to_id_.end()) {
        result.push_back(locale_it->second);
      }
    }
    // Third, try adding language-* locale.
    if (!language.empty()) {
      locale_it = locale_string_to_id_.find(language + "-*");
      if (locale_it != locale_string_to_id_.end()) {
        result.push_back(locale_it->second);
      }
    }
  }

  // Add the default locales if they haven't been added already.
  const std::unordered_set<int> result_set(result.begin(), result.end());
  for (const int default_locale_id : default_locale_ids_) {
    if (result_set.find(default_locale_id) == result_set.end()) {
      result.push_back(default_locale_id);
    }
  }

  return result;
}

namespace {

DatetimeGranularity GetGranularity(const DateParseData& data) {
  DatetimeGranularity granularity = DatetimeGranularity::GRANULARITY_YEAR;
  if ((data.field_set_mask & DateParseData::YEAR_FIELD) ||
      (data.field_set_mask & DateParseData::RELATION_TYPE_FIELD &&
       (data.relation_type == DateParseData::RelationType::YEAR))) {
    granularity = DatetimeGranularity::GRANULARITY_YEAR;
  }
  if ((data.field_set_mask & DateParseData::MONTH_FIELD) ||
      (data.field_set_mask & DateParseData::RELATION_TYPE_FIELD &&
       (data.relation_type == DateParseData::RelationType::MONTH))) {
    granularity = DatetimeGranularity::GRANULARITY_MONTH;
  }
  if (data.field_set_mask & DateParseData::RELATION_TYPE_FIELD &&
      (data.relation_type == DateParseData::RelationType::WEEK)) {
    granularity = DatetimeGranularity::GRANULARITY_WEEK;
  }
  if (data.field_set_mask & DateParseData::DAY_FIELD ||
      (data.field_set_mask & DateParseData::RELATION_FIELD &&
       (data.relation == DateParseData::Relation::NOW ||
        data.relation == DateParseData::Relation::TOMORROW ||
        data.relation == DateParseData::Relation::YESTERDAY)) ||
      (data.field_set_mask & DateParseData::RELATION_TYPE_FIELD &&
       (data.relation_type == DateParseData::RelationType::MONDAY ||
        data.relation_type == DateParseData::RelationType::TUESDAY ||
        data.relation_type == DateParseData::RelationType::WEDNESDAY ||
        data.relation_type == DateParseData::RelationType::THURSDAY ||
        data.relation_type == DateParseData::RelationType::FRIDAY ||
        data.relation_type == DateParseData::RelationType::SATURDAY ||
        data.relation_type == DateParseData::RelationType::SUNDAY ||
        data.relation_type == DateParseData::RelationType::DAY))) {
    granularity = DatetimeGranularity::GRANULARITY_DAY;
  }
  if (data.field_set_mask & DateParseData::HOUR_FIELD) {
    granularity = DatetimeGranularity::GRANULARITY_HOUR;
  }
  if (data.field_set_mask & DateParseData::MINUTE_FIELD) {
    granularity = DatetimeGranularity::GRANULARITY_MINUTE;
  }
  if (data.field_set_mask & DateParseData::SECOND_FIELD) {
    granularity = DatetimeGranularity::GRANULARITY_SECOND;
  }
  return granularity;
}

}  // namespace

bool DatetimeParser::ExtractDatetime(const CompiledRule& rule,
                                     const UniLib::RegexMatcher& matcher,
                                     const int64 reference_time_ms_utc,
                                     const std::string& reference_timezone,
                                     const std::string& reference_locale,
                                     int locale_id, DatetimeParseResult* result,
                                     CodepointSpan* result_span) const {
  DateParseData parse;
  DatetimeExtractor extractor(rule, matcher, locale_id, unilib_,
                              extractor_rules_,
                              type_and_locale_to_extractor_rule_);
  if (!extractor.Extract(&parse, result_span)) {
    return false;
  }

  result->granularity = GetGranularity(parse);

  if (!calendar_lib_.InterpretParseData(
          parse, reference_time_ms_utc, reference_timezone, reference_locale,
          result->granularity, &(result->time_ms_utc))) {
    return false;
  }

  return true;
}

}  // namespace libtextclassifier2
