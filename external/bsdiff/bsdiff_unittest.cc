// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/bsdiff.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "bsdiff/fake_patch_writer.h"

namespace {

// Generate deterministic random data in the output buffer. The buffer must be
// already allocated with the desired size. The data generated depends on the
// selected size.
void GenerateRandomBuffer(std::vector<uint8_t>* buffer) {
  std::minstd_rand prng(1234 + buffer->size());
  std::generate(buffer->begin(), buffer->end(), prng);
}

}  // namespace

namespace bsdiff {

class BsdiffTest : public testing::Test {
 protected:
  BsdiffTest() = default;
  ~BsdiffTest() override = default;

  void RunBsdiff() {
    EXPECT_EQ(0, bsdiff(old_file_.data(), old_file_.size(), new_file_.data(),
                        new_file_.size(), min_len_, &patch_writer_, nullptr));
  }

  std::vector<uint8_t> old_file_;
  std::vector<uint8_t> new_file_;
  size_t min_len_ = 0;  // 0 means the default.
  FakePatchWriter patch_writer_;
};

// Check that a file with no changes has a very small patch (no extra data).
TEST_F(BsdiffTest, EqualEmptyFiles) {
  // Empty old and new files.
  RunBsdiff();

  // No entries should be generated on an empty new file.
  EXPECT_TRUE(patch_writer_.entries().empty());
}

TEST_F(BsdiffTest, EqualSmallFiles) {
  std::string some_text = "Hello world!";
  old_file_.insert(old_file_.begin(), some_text.begin(), some_text.end());
  new_file_.insert(new_file_.begin(), some_text.begin(), some_text.end());
  RunBsdiff();

  EXPECT_EQ(1U, patch_writer_.entries().size());
  ControlEntry entry = patch_writer_.entries()[0];
  EXPECT_EQ(some_text.size(), entry.diff_size);
  EXPECT_EQ(0U, entry.extra_size);
}

TEST_F(BsdiffTest, FileWithSmallErrorsTest) {
  old_file_.resize(100);
  GenerateRandomBuffer(&old_file_);
  new_file_ = old_file_;
  // Break a few bytes somewhere in the middle.
  new_file_[20]++;
  new_file_[30] += 2;
  new_file_[31] += 2;

  RunBsdiff();

  // We expect that the result has only one entry with all in the diff stream
  // since the two files are very similar.
  EXPECT_EQ(1U, patch_writer_.entries().size());
  ControlEntry entry = patch_writer_.entries()[0];
  EXPECT_EQ(100U, entry.diff_size);
  EXPECT_EQ(0U, entry.extra_size);
}

TEST_F(BsdiffTest, MinLengthConsideredTest) {
  old_file_.resize(100);
  GenerateRandomBuffer(&old_file_);
  new_file_ = old_file_;
  // Copy the first 10 bytes to the middle.
  for (size_t i = 0; i < 10; i++) {
    new_file_[50 + i] = old_file_[i];
  }

  min_len_ = 12;
  RunBsdiff();

  // We expect that the 10 bytes in the middle that match the beginning are
  // ignored and just emitted as diff data because the min_len is bigger than
  // 10.
  EXPECT_EQ(1U, patch_writer_.entries().size());
  ControlEntry entry = patch_writer_.entries()[0];
  EXPECT_EQ(100U, entry.diff_size);
  EXPECT_EQ(0U, entry.extra_size);
}

}  // namespace bsdiff
