/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "compiler_filter.h"

#include <gtest/gtest.h>

namespace art {

static void TestCompilerFilterName(CompilerFilter::Filter filter, const std::string& name) {
  CompilerFilter::Filter parsed;
  EXPECT_TRUE(CompilerFilter::ParseCompilerFilter(name.c_str(), &parsed));
  EXPECT_EQ(filter, parsed);

  EXPECT_EQ(name, CompilerFilter::NameOfFilter(filter));
}

static void TestSafeModeFilter(CompilerFilter::Filter expected, const std::string& name) {
  CompilerFilter::Filter parsed;
  EXPECT_TRUE(CompilerFilter::ParseCompilerFilter(name.c_str(), &parsed));
  EXPECT_EQ(expected, CompilerFilter::GetSafeModeFilterFrom(parsed));
}


// Verify the dexopt status values from dalvik.system.DexFile
// match the OatFileAssistant::DexOptStatus values.
TEST(CompilerFilterTest, ParseCompilerFilter) {
  CompilerFilter::Filter filter;

  TestCompilerFilterName(CompilerFilter::kAssumeVerified, "assume-verified");
  TestCompilerFilterName(CompilerFilter::kExtract, "extract");
  TestCompilerFilterName(CompilerFilter::kVerify, "verify");
  TestCompilerFilterName(CompilerFilter::kQuicken, "quicken");
  TestCompilerFilterName(CompilerFilter::kSpaceProfile, "space-profile");
  TestCompilerFilterName(CompilerFilter::kSpace, "space");
  TestCompilerFilterName(CompilerFilter::kSpeedProfile, "speed-profile");
  TestCompilerFilterName(CompilerFilter::kSpeed, "speed");
  TestCompilerFilterName(CompilerFilter::kEverythingProfile, "everything-profile");
  TestCompilerFilterName(CompilerFilter::kEverything, "everything");

  EXPECT_FALSE(CompilerFilter::ParseCompilerFilter("super-awesome-filter", &filter));
}

TEST(CompilerFilterTest, SafeModeFilter) {
  TestSafeModeFilter(CompilerFilter::kAssumeVerified, "assume-verified");
  TestSafeModeFilter(CompilerFilter::kExtract, "extract");
  TestSafeModeFilter(CompilerFilter::kVerify, "verify");
  TestSafeModeFilter(CompilerFilter::kQuicken, "quicken");
  TestSafeModeFilter(CompilerFilter::kQuicken, "space-profile");
  TestSafeModeFilter(CompilerFilter::kQuicken, "space");
  TestSafeModeFilter(CompilerFilter::kQuicken, "speed-profile");
  TestSafeModeFilter(CompilerFilter::kQuicken, "speed");
  TestSafeModeFilter(CompilerFilter::kQuicken, "everything-profile");
  TestSafeModeFilter(CompilerFilter::kQuicken, "everything");
}

}  // namespace art
