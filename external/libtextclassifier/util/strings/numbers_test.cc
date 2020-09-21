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

#include "util/strings/numbers.h"

#include "util/base/integral_types.h"
#include "gtest/gtest.h"

namespace libtextclassifier2 {
namespace {

void TestParseInt32(const char *c_str, bool expected_parsing_success,
                    int32 expected_parsed_value = 0) {
  int32 parsed_value = 0;
  EXPECT_EQ(expected_parsing_success, ParseInt32(c_str, &parsed_value));
  if (expected_parsing_success) {
    EXPECT_EQ(expected_parsed_value, parsed_value);
  }
}

TEST(ParseInt32Test, Normal) {
  TestParseInt32("2", true, 2);
  TestParseInt32("-357", true, -357);
  TestParseInt32("7", true, 7);
  TestParseInt32("+7", true, 7);
  TestParseInt32("  +7", true, 7);
  TestParseInt32("-23", true, -23);
  TestParseInt32("  -23", true, -23);
}

TEST(ParseInt32Test, ErrorCases) {
  TestParseInt32("", false);
  TestParseInt32("  ", false);
  TestParseInt32("not-a-number", false);
  TestParseInt32("123a", false);
}

void TestParseInt64(const char *c_str, bool expected_parsing_success,
                    int64 expected_parsed_value = 0) {
  int64 parsed_value = 0;
  EXPECT_EQ(expected_parsing_success, ParseInt64(c_str, &parsed_value));
  if (expected_parsing_success) {
    EXPECT_EQ(expected_parsed_value, parsed_value);
  }
}

TEST(ParseInt64Test, Normal) {
  TestParseInt64("2", true, 2);
  TestParseInt64("-357", true, -357);
  TestParseInt64("7", true, 7);
  TestParseInt64("+7", true, 7);
  TestParseInt64("  +7", true, 7);
  TestParseInt64("-23", true, -23);
  TestParseInt64("  -23", true, -23);
}

TEST(ParseInt64Test, ErrorCases) {
  TestParseInt64("", false);
  TestParseInt64("  ", false);
  TestParseInt64("not-a-number", false);
  TestParseInt64("23z", false);
}

void TestParseDouble(const char *c_str, bool expected_parsing_success,
                     double expected_parsed_value = 0.0) {
  double parsed_value = 0.0;
  EXPECT_EQ(expected_parsing_success, ParseDouble(c_str, &parsed_value));
  if (expected_parsing_success) {
    EXPECT_NEAR(expected_parsed_value, parsed_value, 0.00001);
  }
}

TEST(ParseDoubleTest, Normal) {
  TestParseDouble("2", true, 2.0);
  TestParseDouble("-357.023", true, -357.023);
  TestParseDouble("7.04", true, 7.04);
  TestParseDouble("+7.2", true, 7.2);
  TestParseDouble("  +7.236", true, 7.236);
  TestParseDouble("-23.4", true, -23.4);
  TestParseDouble("  -23.4", true, -23.4);
}

TEST(ParseDoubleTest, ErrorCases) {
  TestParseDouble("", false);
  TestParseDouble("  ", false);
  TestParseDouble("not-a-number", false);
  TestParseDouble("23.5a", false);
}
}  // namespace
}  // namespace libtextclassifier2
