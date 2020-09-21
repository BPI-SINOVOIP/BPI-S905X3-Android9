// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/bspatch.h"

#include <unistd.h>

#include <gtest/gtest.h>

#include "bsdiff/test_utils.h"

namespace bsdiff {

class BspatchTest : public testing::Test {
 protected:
  BspatchTest()
      : old_file_("bsdiff_oldfile.XXXXXX"),
        new_file_("bsdiff_newfile.XXXXXX") {}

  test_utils::ScopedTempFile old_file_;
  test_utils::ScopedTempFile new_file_;
};

TEST_F(BspatchTest, IsOverlapping) {
  const char* old_filename = old_file_.c_str();
  const char* new_filename = new_file_.c_str();
  EXPECT_FALSE(IsOverlapping(old_filename, "does-not-exist", {}, {}));
  EXPECT_FALSE(IsOverlapping(old_filename, new_filename, {}, {}));
  EXPECT_EQ(0, unlink(new_filename));
  EXPECT_EQ(0, symlink(old_filename, new_filename));
  EXPECT_TRUE(IsOverlapping(old_filename, new_filename, {}, {}));
  EXPECT_TRUE(IsOverlapping(old_filename, old_filename, {}, {}));
  EXPECT_FALSE(IsOverlapping(old_filename, old_filename, {{0, 1}}, {{1, 1}}));
  EXPECT_FALSE(IsOverlapping(old_filename, old_filename, {{2, 1}}, {{1, 1}}));
  EXPECT_TRUE(IsOverlapping(old_filename, old_filename, {{0, 1}}, {{0, 1}}));
  EXPECT_TRUE(IsOverlapping(old_filename, old_filename, {{0, 4}}, {{2, 1}}));
  EXPECT_TRUE(IsOverlapping(old_filename, old_filename, {{1, 1}}, {{0, 2}}));
  EXPECT_TRUE(IsOverlapping(old_filename, old_filename, {{3, 2}}, {{2, 2}}));
}

}  // namespace bsdiff
