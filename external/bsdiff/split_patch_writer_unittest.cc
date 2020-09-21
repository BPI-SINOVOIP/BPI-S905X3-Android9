// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/split_patch_writer.h"

#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "bsdiff/fake_patch_writer.h"
#include "bsdiff/test_utils.h"

namespace bsdiff {

class SplitPatchWriterTest : public testing::Test {
 protected:
  void SetUpForSize(size_t num_chunks,
                    uint64_t new_chunk_size,
                    size_t new_size) {
    fake_patches_.resize(num_chunks);
    std::vector<PatchWriterInterface*> patches;
    for (auto& fake_patch : fake_patches_)
      patches.push_back(&fake_patch);

    patch_writer_.reset(new SplitPatchWriter(new_chunk_size, patches));
    EXPECT_TRUE(patch_writer_->Init(new_size));
  }

  std::vector<FakePatchWriter> fake_patches_;
  std::unique_ptr<SplitPatchWriter> patch_writer_;
};

TEST_F(SplitPatchWriterTest, InvalidNumberOfPatchesForSizeTest) {
  FakePatchWriter p;
  std::vector<PatchWriterInterface*> patches = {&p};
  patch_writer_.reset(new SplitPatchWriter(10, patches));
  // We should have pass two patches.
  EXPECT_FALSE(patch_writer_->Init(15));
}

// A single empty patch is allowed.
TEST_F(SplitPatchWriterTest, NonSplitEmptyPatchTest) {
  SetUpForSize(1, 100, 0);
  EXPECT_TRUE(patch_writer_->Close());

  EXPECT_TRUE(fake_patches_[0].entries().empty());
}

// Leaving patches at the end that are empty is considered an error.
TEST_F(SplitPatchWriterTest, NotAllPatchesWrittenErrorTest) {
  SetUpForSize(2, 100, 200);
  // We write less than the amount needed for two patches, which should fail.
  EXPECT_FALSE(patch_writer_->Close());
}

TEST_F(SplitPatchWriterTest, MissingDiffBytesErrorTest) {
  SetUpForSize(2, 10, 20);

  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(15, 5, 0)));
  std::vector<uint8_t> zeros(20, 0);
  // We write 12 diff bytes instead of the expected 15. This should fail on
  // Close().
  EXPECT_TRUE(patch_writer_->WriteDiffStream(zeros.data(), 12));
  EXPECT_TRUE(patch_writer_->WriteExtraStream(zeros.data(), 5));
  EXPECT_FALSE(patch_writer_->Close());
}

TEST_F(SplitPatchWriterTest, MissingExtraBytesErrorTest) {
  SetUpForSize(2, 10, 20);

  std::vector<uint8_t> zeros(20, 0);
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(5, 15, -5)));
  EXPECT_TRUE(patch_writer_->WriteDiffStream(zeros.data(), 5));
  // We write a little less than the expected 15. This operation should succeed,
  // since we could write the rest later.
  EXPECT_TRUE(patch_writer_->WriteExtraStream(zeros.data(), 10));
  EXPECT_FALSE(patch_writer_->Close());
}

// Test all sort of corner cases when splitting the ControlEntry across multiple
// patches
TEST_F(SplitPatchWriterTest, SplitControlAcrossSeveralPatchesTest) {
  SetUpForSize(4, 10, 40);
  // The middle control entry would be split in tree different patches.
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(5, 1, -5)));
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(4, 0, -4)));
  // old_pos at this point is 0. This is the end of the first patch.
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(6, 0, -1)));
  // old_pos at this point is 5.
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(1, 18, 2)));
  // old_pos at this point is 8.
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(1, 0, 1)));
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(4, 0, -5)));

  std::vector<uint8_t> zeros(40, 0);
  EXPECT_TRUE(
      patch_writer_->WriteDiffStream(zeros.data(), 5 + 4 + 6 + 1 + 1 + 4));
  EXPECT_TRUE(patch_writer_->WriteExtraStream(zeros.data(), 1 + 18));
  EXPECT_TRUE(patch_writer_->Close());

  EXPECT_EQ((std::vector<ControlEntry>{
                ControlEntry(5, 1, -5),
                ControlEntry(4, 0, 0),  // (4, 0, -4) but the -4 is not needed.
            }),
            fake_patches_[0].entries());
  EXPECT_EQ((std::vector<ControlEntry>{
                // No need for dummy entry because the old_pos is already at 0.
                ControlEntry(6, 0, -1),
                ControlEntry(1, 3, 0),  // the first part of (1, 18, 2)
            }),
            fake_patches_[1].entries());
  EXPECT_EQ((std::vector<ControlEntry>{
                // No need for dummy entry because the first entry is all in
                // the extra stream and this is the last entry.
                ControlEntry(0, 10, 0),  // the middle part of (1, 18, 2)
            }),
            fake_patches_[2].entries());
  EXPECT_EQ((std::vector<ControlEntry>{
                // No need for dummy entry because the first entry is all in
                // the extra stream, so use that.
                ControlEntry(0, 5, 8),  // the last part of (1, 18, 2), plus the
                                        // old_pos 5. 8 = 1 + 2 + 5.
                ControlEntry(1, 0, 1),  // (1, 0, 1) copied
                ControlEntry(4, 0, 0),  // (4, 0, -5) ignoring the offset.
            }),
            fake_patches_[3].entries());

  for (size_t i = 0; i < fake_patches_.size(); ++i) {
    EXPECT_EQ(10U, fake_patches_[i].new_size()) << "where i = " << i;
  }
}

TEST_F(SplitPatchWriterTest, WriteStreamsAfterControlAcrossPatchesTest) {
  std::vector<uint8_t> numbers(40);
  for (size_t i = 0; i < numbers.size(); ++i)
    numbers[i] = 'A' + i;

  SetUpForSize(4, 10, 40);
  // The sequence is 15 diff, 10 extra, 15 diff.
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(15, 10, 0)));
  EXPECT_TRUE(patch_writer_->AddControlEntry(ControlEntry(15, 0, 0)));
  // Numbers [0, 30) for the diff stream, and [30, 40) for the extra stream.
  EXPECT_TRUE(patch_writer_->WriteDiffStream(numbers.data(), 30));
  EXPECT_TRUE(patch_writer_->WriteExtraStream(numbers.data() + 30, 10));
  EXPECT_TRUE(patch_writer_->Close());

  EXPECT_EQ(std::vector<uint8_t>(numbers.begin(), numbers.begin() + 10),
            fake_patches_[0].diff_stream());
  EXPECT_TRUE(fake_patches_[0].extra_stream().empty());

  // 5 diff, then 5 extra.
  EXPECT_EQ(std::vector<uint8_t>(numbers.begin() + 10, numbers.begin() + 15),
            fake_patches_[1].diff_stream());
  EXPECT_EQ(std::vector<uint8_t>(numbers.begin() + 30, numbers.begin() + 35),
            fake_patches_[1].extra_stream());

  // 5 extra, then 5 diff.
  EXPECT_EQ(std::vector<uint8_t>(numbers.begin() + 15, numbers.begin() + 20),
            fake_patches_[2].diff_stream());
  EXPECT_EQ(std::vector<uint8_t>(numbers.begin() + 35, numbers.begin() + 40),
            fake_patches_[2].extra_stream());

  EXPECT_EQ(std::vector<uint8_t>(numbers.begin() + 20, numbers.begin() + 30),
            fake_patches_[3].diff_stream());
  EXPECT_TRUE(fake_patches_[3].extra_stream().empty());
}

}  // namespace bsdiff
