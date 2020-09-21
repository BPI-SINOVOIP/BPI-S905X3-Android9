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

// This test serves the purpose of making sure all the different implementations
// of the unspoken CalendarLib interface support the same methods.

#include "util/calendar/calendar.h"
#include "util/base/logging.h"

#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

TEST(CalendarTest, Interface) {
  CalendarLib calendar;
  int64 time;
  std::string timezone;
  bool result = calendar.InterpretParseData(
      DateParseData{0l, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    static_cast<DateParseData::Relation>(0),
                    static_cast<DateParseData::RelationType>(0), 0},
      0L, "Zurich", "en-CH", GRANULARITY_UNKNOWN, &time);
  TC_LOG(INFO) << result;
}

#ifdef LIBTEXTCLASSIFIER_UNILIB_ICU
TEST(CalendarTest, RoundingToGranularity) {
  CalendarLib calendar;
  int64 time;
  std::string timezone;
  DateParseData data;
  data.year = 2018;
  data.month = 4;
  data.day_of_month = 25;
  data.hour = 9;
  data.minute = 33;
  data.second = 59;
  data.field_set_mask = DateParseData::YEAR_FIELD | DateParseData::MONTH_FIELD |
                        DateParseData::DAY_FIELD | DateParseData::HOUR_FIELD |
                        DateParseData::MINUTE_FIELD |
                        DateParseData::SECOND_FIELD;
  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-CH",
      /*granularity=*/GRANULARITY_YEAR, &time));
  EXPECT_EQ(time, 1514761200000L /* Jan 01 2018 00:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-CH",
      /*granularity=*/GRANULARITY_MONTH, &time));
  EXPECT_EQ(time, 1522533600000L /* Apr 01 2018 00:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-CH",
      /*granularity=*/GRANULARITY_WEEK, &time));
  EXPECT_EQ(time, 1524434400000L /* Mon Apr 23 2018 00:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"*-CH",
      /*granularity=*/GRANULARITY_WEEK, &time));
  EXPECT_EQ(time, 1524434400000L /* Mon Apr 23 2018 00:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-US",
      /*granularity=*/GRANULARITY_WEEK, &time));
  EXPECT_EQ(time, 1524348000000L /* Sun Apr 22 2018 00:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"*-US",
      /*granularity=*/GRANULARITY_WEEK, &time));
  EXPECT_EQ(time, 1524348000000L /* Sun Apr 22 2018 00:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-CH",
      /*granularity=*/GRANULARITY_DAY, &time));
  EXPECT_EQ(time, 1524607200000L /* Apr 25 2018 00:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-CH",
      /*granularity=*/GRANULARITY_HOUR, &time));
  EXPECT_EQ(time, 1524639600000L /* Apr 25 2018 09:00:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-CH",
      /*granularity=*/GRANULARITY_MINUTE, &time));
  EXPECT_EQ(time, 1524641580000 /* Apr 25 2018 09:33:00 */);

  ASSERT_TRUE(calendar.InterpretParseData(
      data,
      /*reference_time_ms_utc=*/0L, /*reference_timezone=*/"Europe/Zurich",
      /*reference_locale=*/"en-CH",
      /*granularity=*/GRANULARITY_SECOND, &time));
  EXPECT_EQ(time, 1524641639000 /* Apr 25 2018 09:33:59 */);
}
#endif  // LIBTEXTCLASSIFIER_UNILIB_DUMMY

}  // namespace
}  // namespace libtextclassifier2
