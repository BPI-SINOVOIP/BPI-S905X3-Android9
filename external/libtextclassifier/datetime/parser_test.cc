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

#include <time.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "datetime/parser.h"
#include "model_generated.h"
#include "text-classifier.h"
#include "types-test-util.h"

using testing::ElementsAreArray;

namespace libtextclassifier2 {
namespace {

std::string GetModelPath() {
  return LIBTEXTCLASSIFIER_TEST_DATA_DIR;
}

std::string ReadFile(const std::string& file_name) {
  std::ifstream file_stream(file_name);
  return std::string(std::istreambuf_iterator<char>(file_stream), {});
}

std::string FormatMillis(int64 time_ms_utc) {
  long time_seconds = time_ms_utc / 1000;  // NOLINT
  // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
  char buffer[512];
  strftime(buffer, sizeof(buffer), "%a %Y-%m-%d %H:%M:%S %Z",
           localtime(&time_seconds));
  return std::string(buffer);
}

class ParserTest : public testing::Test {
 public:
  void SetUp() override {
    model_buffer_ = ReadFile(GetModelPath() + "test_model.fb");
    classifier_ = TextClassifier::FromUnownedBuffer(
        model_buffer_.data(), model_buffer_.size(), &unilib_);
    TC_CHECK(classifier_);
    parser_ = classifier_->DatetimeParserForTests();
  }

  bool HasNoResult(const std::string& text, bool anchor_start_end = false,
                   const std::string& timezone = "Europe/Zurich") {
    std::vector<DatetimeParseResultSpan> results;
    if (!parser_->Parse(text, 0, timezone, /*locales=*/"", ModeFlag_ANNOTATION,
                        anchor_start_end, &results)) {
      TC_LOG(ERROR) << text;
      TC_CHECK(false);
    }
    return results.empty();
  }

  bool ParsesCorrectly(const std::string& marked_text,
                       const int64 expected_ms_utc,
                       DatetimeGranularity expected_granularity,
                       bool anchor_start_end = false,
                       const std::string& timezone = "Europe/Zurich",
                       const std::string& locales = "en-US") {
    const UnicodeText marked_text_unicode =
        UTF8ToUnicodeText(marked_text, /*do_copy=*/false);
    auto brace_open_it =
        std::find(marked_text_unicode.begin(), marked_text_unicode.end(), '{');
    auto brace_end_it =
        std::find(marked_text_unicode.begin(), marked_text_unicode.end(), '}');
    TC_CHECK(brace_open_it != marked_text_unicode.end());
    TC_CHECK(brace_end_it != marked_text_unicode.end());

    std::string text;
    text +=
        UnicodeText::UTF8Substring(marked_text_unicode.begin(), brace_open_it);
    text += UnicodeText::UTF8Substring(std::next(brace_open_it), brace_end_it);
    text += UnicodeText::UTF8Substring(std::next(brace_end_it),
                                       marked_text_unicode.end());

    std::vector<DatetimeParseResultSpan> results;

    if (!parser_->Parse(text, 0, timezone, locales, ModeFlag_ANNOTATION,
                        anchor_start_end, &results)) {
      TC_LOG(ERROR) << text;
      TC_CHECK(false);
    }
    if (results.empty()) {
      TC_LOG(ERROR) << "No results.";
      return false;
    }

    const int expected_start_index =
        std::distance(marked_text_unicode.begin(), brace_open_it);
    // The -1 bellow is to account for the opening bracket character.
    const int expected_end_index =
        std::distance(marked_text_unicode.begin(), brace_end_it) - 1;

    std::vector<DatetimeParseResultSpan> filtered_results;
    for (const DatetimeParseResultSpan& result : results) {
      if (SpansOverlap(result.span,
                       {expected_start_index, expected_end_index})) {
        filtered_results.push_back(result);
      }
    }

    const std::vector<DatetimeParseResultSpan> expected{
        {{expected_start_index, expected_end_index},
         {expected_ms_utc, expected_granularity},
         /*target_classification_score=*/1.0,
         /*priority_score=*/0.0}};
    const bool matches =
        testing::Matches(ElementsAreArray(expected))(filtered_results);
    if (!matches) {
      TC_LOG(ERROR) << "Expected: " << expected[0] << " which corresponds to: "
                    << FormatMillis(expected[0].data.time_ms_utc);
      for (int i = 0; i < filtered_results.size(); ++i) {
        TC_LOG(ERROR) << "Actual[" << i << "]: " << filtered_results[i]
                      << " which corresponds to: "
                      << FormatMillis(filtered_results[i].data.time_ms_utc);
      }
    }
    return matches;
  }

  bool ParsesCorrectlyGerman(const std::string& marked_text,
                             const int64 expected_ms_utc,
                             DatetimeGranularity expected_granularity) {
    return ParsesCorrectly(marked_text, expected_ms_utc, expected_granularity,
                           /*anchor_start_end=*/false,
                           /*timezone=*/"Europe/Zurich", /*locales=*/"de");
  }

 protected:
  std::string model_buffer_;
  std::unique_ptr<TextClassifier> classifier_;
  const DatetimeParser* parser_;
  UniLib unilib_;
};

// Test with just a few cases to make debugging of general failures easier.
TEST_F(ParserTest, ParseShort) {
  EXPECT_TRUE(
      ParsesCorrectly("{January 1, 1988}", 567990000000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectly("{three days ago}", -262800000, GRANULARITY_DAY));
}

TEST_F(ParserTest, Parse) {
  EXPECT_TRUE(
      ParsesCorrectly("{January 1, 1988}", 567990000000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectly("{january 31 2018}", 1517353200000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectly("lorem {1 january 2018} ipsum", 1514761200000,
                              GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectly("{09/Mar/2004 22:02:40}", 1078866160000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{Dec 2, 2010 2:39:58 AM}", 1291253998000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{Jun 09 2011 15:28:14}", 1307626094000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(
      ParsesCorrectly("{Mar 16 08:12:04}", 6419524000, GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{2010-06-26 02:31:29},573", 1277512289000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{2006/01/22 04:11:05}", 1137899465000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{11:42:35}", 38555000, GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{11:42:35}.173", 38555000, GRANULARITY_SECOND));
  EXPECT_TRUE(
      ParsesCorrectly("{23/Apr 11:42:35},173", 9715355000, GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{23/Apr/2015 11:42:35}", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{23-Apr-2015 11:42:35}", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{23-Apr-2015 11:42:35}.883", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{23 Apr 2015 11:42:35}", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{23 Apr 2015 11:42:35}.883", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{04/23/15 11:42:35}", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{04/23/2015 11:42:35}", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{04/23/2015 11:42:35}.883", 1429782155000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{9/28/2011 2:23:15 PM}", 1317212595000,
                              GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly(
      "Are sentiments apartments decisively the especially alteration. "
      "Thrown shy denote ten ladies though ask saw. Or by to he going "
      "think order event music. Incommode so intention defective at "
      "convinced. Led income months itself and houses you. After nor "
      "you leave might share court balls. {19/apr/2010 06:36:15} Are "
      "sentiments apartments decisively the especially alteration. "
      "Thrown shy denote ten ladies though ask saw. Or by to he going "
      "think order event music. Incommode so intention defective at "
      "convinced. Led income months itself and houses you. After nor "
      "you leave might share court balls. ",
      1271651775000, GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectly("{january 1 2018 at 4:30}", 1514777400000,
                              GRANULARITY_MINUTE));
  EXPECT_TRUE(ParsesCorrectly("{january 1 2018 at 4:30 am}", 1514777400000,
                              GRANULARITY_MINUTE));
  EXPECT_TRUE(ParsesCorrectly("{january 1 2018 at 4pm}", 1514818800000,
                              GRANULARITY_HOUR));

  EXPECT_TRUE(ParsesCorrectly("{today}", -3600000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectly("{today}", -57600000, GRANULARITY_DAY,
                              /*anchor_start_end=*/false,
                              "America/Los_Angeles"));
  EXPECT_TRUE(ParsesCorrectly("{next week}", 255600000, GRANULARITY_WEEK));
  EXPECT_TRUE(ParsesCorrectly("{next day}", 82800000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectly("{in three days}", 255600000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectly("{in three weeks}", 1465200000, GRANULARITY_WEEK));
  EXPECT_TRUE(ParsesCorrectly("{tomorrow}", 82800000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectly("{tomorrow at 4:00}", 97200000, GRANULARITY_MINUTE));
  EXPECT_TRUE(ParsesCorrectly("{tomorrow at 4}", 97200000, GRANULARITY_HOUR));
  EXPECT_TRUE(ParsesCorrectly("{next wednesday}", 514800000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectly("{next wednesday at 4}", 529200000, GRANULARITY_HOUR));
  EXPECT_TRUE(ParsesCorrectly("last seen {today at 9:01 PM}", 72060000,
                              GRANULARITY_MINUTE));
  EXPECT_TRUE(ParsesCorrectly("{Three days ago}", -262800000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectly("{three days ago}", -262800000, GRANULARITY_DAY));
}

TEST_F(ParserTest, ParseWithAnchor) {
  EXPECT_TRUE(ParsesCorrectly("{January 1, 1988}", 567990000000,
                              GRANULARITY_DAY, /*anchor_start_end=*/false));
  EXPECT_TRUE(ParsesCorrectly("{January 1, 1988}", 567990000000,
                              GRANULARITY_DAY, /*anchor_start_end=*/true));
  EXPECT_TRUE(ParsesCorrectly("lorem {1 january 2018} ipsum", 1514761200000,
                              GRANULARITY_DAY, /*anchor_start_end=*/false));
  EXPECT_TRUE(HasNoResult("lorem 1 january 2018 ipsum",
                          /*anchor_start_end=*/true));
}

TEST_F(ParserTest, ParseGerman) {
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{Januar 1 2018}", 1514761200000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{1 2 2018}", 1517439600000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectlyGerman("lorem {1 Januar 2018} ipsum",
                                    1514761200000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectlyGerman("{19/Apr/2010:06:36:15}", 1271651775000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{09/März/2004 22:02:40}", 1078866160000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{Dez 2, 2010 2:39:58}", 1291253998000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{Juni 09 2011 15:28:14}", 1307626094000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{März 16 08:12:04}", 6419524000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{2010-06-26 02:31:29},573", 1277512289000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{2006/01/22 04:11:05}", 1137899465000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{11:42:35}", 38555000, GRANULARITY_SECOND));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{11:42:35}.173", 38555000, GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{23/Apr 11:42:35},173", 9715355000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{23/Apr/2015:11:42:35}", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{23/Apr/2015 11:42:35}", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{23-Apr-2015 11:42:35}", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{23-Apr-2015 11:42:35}.883", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{23 Apr 2015 11:42:35}", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{23 Apr 2015 11:42:35}.883", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{04/23/15 11:42:35}", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{04/23/2015 11:42:35}", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{04/23/2015 11:42:35}.883", 1429782155000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{19/apr/2010:06:36:15}", 1271651775000,
                                    GRANULARITY_SECOND));
  EXPECT_TRUE(ParsesCorrectlyGerman("{januar 1 2018 um 4:30}", 1514777400000,
                                    GRANULARITY_MINUTE));
  EXPECT_TRUE(ParsesCorrectlyGerman("{januar 1 2018 um 4:30 nachm}",
                                    1514820600000, GRANULARITY_MINUTE));
  EXPECT_TRUE(ParsesCorrectlyGerman("{januar 1 2018 um 4 nachm}", 1514818800000,
                                    GRANULARITY_HOUR));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{14.03.2017}", 1489446000000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectlyGerman("{heute}", -3600000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{nächste Woche}", 342000000, GRANULARITY_WEEK));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{nächsten Tag}", 82800000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{in drei Tagen}", 255600000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{in drei Wochen}", 1551600000, GRANULARITY_WEEK));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{vor drei Tagen}", -262800000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectlyGerman("{morgen}", 82800000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{morgen um 4:00}", 97200000, GRANULARITY_MINUTE));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{morgen um 4}", 97200000, GRANULARITY_HOUR));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{nächsten Mittwoch}", 514800000, GRANULARITY_DAY));
  EXPECT_TRUE(ParsesCorrectlyGerman("{nächsten Mittwoch um 4}", 529200000,
                                    GRANULARITY_HOUR));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{Vor drei Tagen}", -262800000, GRANULARITY_DAY));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{in einer woche}", 342000000, GRANULARITY_WEEK));
  EXPECT_TRUE(
      ParsesCorrectlyGerman("{in einer tag}", 82800000, GRANULARITY_DAY));
}

TEST_F(ParserTest, ParseNonUs) {
  EXPECT_TRUE(ParsesCorrectly("{1/5/15}", 1430431200000, GRANULARITY_DAY,
                              /*anchor_start_end=*/false,
                              /*timezone=*/"Europe/Zurich",
                              /*locales=*/"en-GB"));
  EXPECT_TRUE(ParsesCorrectly("{1/5/15}", 1430431200000, GRANULARITY_DAY,
                              /*anchor_start_end=*/false,
                              /*timezone=*/"Europe/Zurich", /*locales=*/"en"));
}

TEST_F(ParserTest, ParseUs) {
  EXPECT_TRUE(ParsesCorrectly("{1/5/15}", 1420412400000, GRANULARITY_DAY,
                              /*anchor_start_end=*/false,
                              /*timezone=*/"Europe/Zurich",
                              /*locales=*/"en-US"));
  EXPECT_TRUE(ParsesCorrectly("{1/5/15}", 1420412400000, GRANULARITY_DAY,
                              /*anchor_start_end=*/false,
                              /*timezone=*/"Europe/Zurich",
                              /*locales=*/"es-US"));
}

TEST_F(ParserTest, ParseUnknownLanguage) {
  EXPECT_TRUE(ParsesCorrectly("bylo to {31. 12. 2015} v 6 hodin", 1451516400000,
                              GRANULARITY_DAY,
                              /*anchor_start_end=*/false,
                              /*timezone=*/"Europe/Zurich", /*locales=*/"xx"));
}

class ParserLocaleTest : public testing::Test {
 public:
  void SetUp() override;
  bool HasResult(const std::string& input, const std::string& locales);

 protected:
  UniLib unilib_;
  flatbuffers::FlatBufferBuilder builder_;
  std::unique_ptr<DatetimeParser> parser_;
};

void AddPattern(const std::string& regex, int locale,
                std::vector<std::unique_ptr<DatetimeModelPatternT>>* patterns) {
  patterns->emplace_back(new DatetimeModelPatternT);
  patterns->back()->regexes.emplace_back(new DatetimeModelPattern_::RegexT);
  patterns->back()->regexes.back()->pattern = regex;
  patterns->back()->regexes.back()->groups.push_back(
      DatetimeGroupType_GROUP_UNUSED);
  patterns->back()->locales.push_back(locale);
}

void ParserLocaleTest::SetUp() {
  DatetimeModelT model;
  model.use_extractors_for_locating = false;
  model.locales.clear();
  model.locales.push_back("en-US");
  model.locales.push_back("en-CH");
  model.locales.push_back("zh-Hant");
  model.locales.push_back("en-*");
  model.locales.push_back("zh-Hant-*");
  model.locales.push_back("*-CH");
  model.locales.push_back("default");
  model.default_locales.push_back(6);

  AddPattern(/*regex=*/"en-US", /*locale=*/0, &model.patterns);
  AddPattern(/*regex=*/"en-CH", /*locale=*/1, &model.patterns);
  AddPattern(/*regex=*/"zh-Hant", /*locale=*/2, &model.patterns);
  AddPattern(/*regex=*/"en-all", /*locale=*/3, &model.patterns);
  AddPattern(/*regex=*/"zh-Hant-all", /*locale=*/4, &model.patterns);
  AddPattern(/*regex=*/"all-CH", /*locale=*/5, &model.patterns);
  AddPattern(/*regex=*/"default", /*locale=*/6, &model.patterns);

  builder_.Finish(DatetimeModel::Pack(builder_, &model));
  const DatetimeModel* model_fb =
      flatbuffers::GetRoot<DatetimeModel>(builder_.GetBufferPointer());
  ASSERT_TRUE(model_fb);

  parser_ = DatetimeParser::Instance(model_fb, unilib_,
                                     /*decompressor=*/nullptr);
  ASSERT_TRUE(parser_);
}

bool ParserLocaleTest::HasResult(const std::string& input,
                                 const std::string& locales) {
  std::vector<DatetimeParseResultSpan> results;
  EXPECT_TRUE(parser_->Parse(input, /*reference_time_ms_utc=*/0,
                             /*reference_timezone=*/"", locales,
                             ModeFlag_ANNOTATION, false, &results));
  return results.size() == 1;
}

TEST_F(ParserLocaleTest, English) {
  EXPECT_TRUE(HasResult("en-US", /*locales=*/"en-US"));
  EXPECT_FALSE(HasResult("en-CH", /*locales=*/"en-US"));
  EXPECT_FALSE(HasResult("en-US", /*locales=*/"en-CH"));
  EXPECT_TRUE(HasResult("en-CH", /*locales=*/"en-CH"));
  EXPECT_TRUE(HasResult("default", /*locales=*/"en-CH"));
}

TEST_F(ParserLocaleTest, TraditionalChinese) {
  EXPECT_TRUE(HasResult("zh-Hant-all", /*locales=*/"zh-Hant"));
  EXPECT_TRUE(HasResult("zh-Hant-all", /*locales=*/"zh-Hant-TW"));
  EXPECT_TRUE(HasResult("zh-Hant-all", /*locales=*/"zh-Hant-SG"));
  EXPECT_FALSE(HasResult("zh-Hant-all", /*locales=*/"zh-SG"));
  EXPECT_FALSE(HasResult("zh-Hant-all", /*locales=*/"zh"));
  EXPECT_TRUE(HasResult("default", /*locales=*/"zh"));
  EXPECT_TRUE(HasResult("default", /*locales=*/"zh-Hant-SG"));
}

TEST_F(ParserLocaleTest, SwissEnglish) {
  EXPECT_TRUE(HasResult("all-CH", /*locales=*/"de-CH"));
  EXPECT_TRUE(HasResult("all-CH", /*locales=*/"en-CH"));
  EXPECT_TRUE(HasResult("en-all", /*locales=*/"en-CH"));
  EXPECT_FALSE(HasResult("all-CH", /*locales=*/"de-DE"));
  EXPECT_TRUE(HasResult("default", /*locales=*/"de-CH"));
  EXPECT_TRUE(HasResult("default", /*locales=*/"en-CH"));
}

}  // namespace
}  // namespace libtextclassifier2
