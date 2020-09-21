// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/suffix_array_index.h"

#include <stdlib.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace bsdiff {

class SuffixArrayIndexTest : public ::testing::Test {
 protected:
  void InitSA(const std::string& text) {
    text_.resize(text.size());
    std::copy(text.begin(), text.end(), text_.data());
    sai_ = CreateSuffixArrayIndex(text_.data(), text_.size());
  }

  void SearchPrefix(const std::string& pattern,
                    size_t* out_length,
                    uint64_t* out_pos) {
    sai_->SearchPrefix(reinterpret_cast<const uint8_t*>(pattern.data()),
                       pattern.size(), out_length, out_pos);
    // Check that the returned values are indeed a valid prefix.
    EXPECT_LE(*out_length, pattern.size());
    ASSERT_LT(*out_pos, text_.size());
    ASSERT_LE(*out_pos + *out_length, text_.size());
    EXPECT_EQ(0, memcmp(text_.data() + *out_pos, pattern.data(), *out_length));
  }

  std::vector<uint8_t> text_;
  std::unique_ptr<SuffixArrayIndexInterface> sai_;
};

// Test that the suffix array index can be initialized with an example text.
TEST_F(SuffixArrayIndexTest, MississippiTest) {
  InitSA("mississippi");
}

TEST_F(SuffixArrayIndexTest, EmptySuffixArrayTest) {
  InitSA("");
}

// Test various corner cases while searching for prefixes.
TEST_F(SuffixArrayIndexTest, SearchPrefixTest) {
  InitSA("abc1_abc2_abc3_ab_abcd");

  uint64_t pos;
  size_t length;
  // The string is not found at all.
  SearchPrefix("zzz", &length, &pos);  // lexicographically highest.
  EXPECT_EQ(0U, length);

  SearchPrefix("   ", &length, &pos);  // lexicographically lowest.
  EXPECT_EQ(0U, length);

  // The exact pattern is found multiple times.
  SearchPrefix("abc", &length, &pos);
  EXPECT_EQ(3U, length);

  // The exact pattern is found only one time, at the end of the string.
  SearchPrefix("abcd", &length, &pos);
  EXPECT_EQ(4U, length);
  EXPECT_EQ(18U, pos);

  // The string is not found, but a prefix is found.
  SearchPrefix("abcW", &length, &pos);
  EXPECT_EQ(3U, length);
}

}  // namespace bsdiff
