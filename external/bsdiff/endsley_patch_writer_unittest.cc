// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/endsley_patch_writer.h"

#include <algorithm>

#include <gtest/gtest.h>

namespace {

std::vector<uint8_t> VectorFromString(const std::string& s) {
  return std::vector<uint8_t>(s.data(), s.data() + s.size());
}

}  // namespace

namespace bsdiff {

class EndsleyPatchWriterTest : public testing::Test {
 protected:
  // Return a subvector from |data_| starting at |start| of size at most |size|.
  std::vector<uint8_t> DataSubvector(size_t start, size_t size) {
    if (start > data_.size())
      return std::vector<uint8_t>();

    size = std::min(size, data_.size() - start);
    return std::vector<uint8_t>(data_.begin() + start,
                                data_.begin() + start + size);
  }

  std::vector<uint8_t> data_;
  EndsleyPatchWriter patch_writer_{&data_, CompressorType::kNoCompression, 0};
};

// Smoke check that a patch includes the new_size and magic header.
TEST_F(EndsleyPatchWriterTest, CreateEmptyPatchTest) {
  EXPECT_TRUE(patch_writer_.Init(0));
  EXPECT_TRUE(patch_writer_.Close());

  // The empty header is set to 24 bytes.
  EXPECT_EQ(24U, data_.size());

  std::vector<uint8_t> empty_patch = {
      // Magic header.
      'E', 'N', 'D', 'S', 'L', 'E', 'Y', '/', 'B', 'S', 'D', 'I', 'F', 'F', '4',
      '3',
      // 8 zeros for the |new_size| of zero bytes.
      0, 0, 0, 0, 0, 0, 0, 0};
  EXPECT_EQ(empty_patch, data_);
}

TEST_F(EndsleyPatchWriterTest, CreateCompressedPatchTest) {
  EndsleyPatchWriter compressed_writer(&data_, CompressorType::kBZ2, 9);

  auto text = VectorFromString("HelloWorld");
  EXPECT_TRUE(compressed_writer.Init(text.size()));

  EXPECT_TRUE(compressed_writer.AddControlEntry(ControlEntry(5, 5, -2)));
  EXPECT_TRUE(compressed_writer.WriteDiffStream(text.data(), 5));
  EXPECT_TRUE(compressed_writer.WriteExtraStream(text.data() + 5, 5));

  // Check that the output patch had no data written to it before Close() is
  // called, since we are still compressing it.
  EXPECT_TRUE(data_.empty());

  EXPECT_TRUE(compressed_writer.Close());

  // Check that the whole file is compressed with BZ2 by looking at the header.
  const auto bz2_header = VectorFromString("BZh9");
  data_.resize(4);
  EXPECT_EQ(bz2_header, data_);
}

TEST_F(EndsleyPatchWriterTest, CreateEmptyBrotliPatchTest) {
  EndsleyPatchWriter compressed_writer(&data_, CompressorType::kBrotli, 9);
  EXPECT_TRUE(compressed_writer.Init(0));
  EXPECT_TRUE(compressed_writer.Close());
}

// Test we generate the right patch when the control, diff and extra stream come
// in the right order.
TEST_F(EndsleyPatchWriterTest, DataInNiceOrderTest) {
  auto text = VectorFromString("abcdeFGHIJ");
  EXPECT_TRUE(patch_writer_.Init(10));

  EXPECT_TRUE(patch_writer_.AddControlEntry(ControlEntry(2, 3, -2)));
  EXPECT_TRUE(patch_writer_.WriteDiffStream(text.data(), 2));
  EXPECT_TRUE(patch_writer_.WriteExtraStream(text.data() + 2, 3));

  // Check that we are actually writing to the output vector as soon as we can.
  EXPECT_EQ(24U + 24U + 2U + 3U, data_.size());

  EXPECT_TRUE(patch_writer_.AddControlEntry(ControlEntry(0, 5, 1024)));
  EXPECT_TRUE(patch_writer_.WriteExtraStream(text.data() + 5, 5));

  EXPECT_TRUE(patch_writer_.Close());

  // We have a header, 2 control entries and a total of 10 bytes of data.
  EXPECT_EQ(24U + 24U * 2 + 10U, data_.size());

  // Verify that control entry values are encoded properly in little-endian.
  EXPECT_EQ((std::vector<uint8_t>{10, 0, 0, 0, 0, 0, 0, 0}),
            DataSubvector(16U, 8));  // new_size

  // Negative numbers are encoded with the sign bit in the most significant bit
  // of the 8-byte number.
  EXPECT_EQ((std::vector<uint8_t>{2, 0, 0, 0, 0, 0, 0, 0x80}),
            DataSubvector(24U + 16, 8));

  // The second member on the last control entry (1024) encoded in
  // little-endian.
  EXPECT_EQ((std::vector<uint8_t>{0, 4, 0, 0, 0, 0, 0, 0}),
            DataSubvector(24U + 24U + 5U + 16U, 8));

  // Check that the diff and extra data are sent one after the other in the
  // right order.
  EXPECT_EQ(VectorFromString("abcde"), DataSubvector(24U + 24U, 5));
}

// When we send first the diff or extra data it shouldn't be possible to
// write it to the patch, but at the end of the patch we should be able to
// write it all.
TEST_F(EndsleyPatchWriterTest, DataInBadOrderTest) {
  auto text = VectorFromString("abcdeFGHIJ");
  EXPECT_TRUE(patch_writer_.Init(10));
  EXPECT_TRUE(patch_writer_.WriteDiffStream(text.data(), 5));
  EXPECT_TRUE(patch_writer_.WriteExtraStream(text.data() + 5, 5));

  // Writ all the control entries at the end, only the header should have been
  // sent so far.
  EXPECT_EQ(24U, data_.size());

  EXPECT_TRUE(patch_writer_.AddControlEntry(ControlEntry(2, 3, -2)));
  EXPECT_TRUE(patch_writer_.AddControlEntry(ControlEntry(2, 1, 1024)));
  EXPECT_TRUE(patch_writer_.AddControlEntry(ControlEntry(1, 1, 1024)));

  EXPECT_TRUE(patch_writer_.Close());

  // We have a header, 3 control entries and a total of 10 bytes of data.
  EXPECT_EQ(24U + 24U * 3 + 10U, data_.size());

  // The data from the first and second control entries:
  EXPECT_EQ(VectorFromString("abFGH"), DataSubvector(24U + 24U, 5));
  EXPECT_EQ(VectorFromString("cdI"), DataSubvector(24U + 24U * 2 + 5, 3));
  EXPECT_EQ(VectorFromString("eJ"), DataSubvector(24U + 24U * 3 + 8, 2));
}

TEST_F(EndsleyPatchWriterTest, FlushOnlyWhenWorthItTest) {
  size_t kEntrySize = 1000;  // must be even for this test.
  size_t kNumEntries = 3000;
  size_t kNewSize = kEntrySize * kNumEntries;  // 3 MB

  EXPECT_TRUE(patch_writer_.Init(kNewSize));
  // Write all the extra and diff data first.
  std::vector<uint8_t> zeros(kNewSize / 2, 0);
  EXPECT_TRUE(patch_writer_.WriteDiffStream(zeros.data(), zeros.size()));
  EXPECT_TRUE(patch_writer_.WriteExtraStream(zeros.data(), zeros.size()));

  // No patch data flushed so far, only the header.
  EXPECT_EQ(24U, data_.size());

  ControlEntry entry(kEntrySize / 2, kEntrySize / 2, -1);
  for (size_t i = 0; i < 10; i++) {
    EXPECT_TRUE(patch_writer_.AddControlEntry(entry));
  }

  // Even if all the diff and extra data is available and some control entries
  // are also available no information should have been flushed yet because we
  // don't want the overhead of updating the diff_data_ and extra_data_ vectors.
  EXPECT_EQ(24U, data_.size());

  // Write the remaining entries.
  for (size_t i = 0; i < kNumEntries - 10; i++) {
    EXPECT_TRUE(patch_writer_.AddControlEntry(entry));
  }

  // Even before Close() is called, we have enough control entries to make it
  // worth it calling flush at some point.
  EXPECT_LT(24U, data_.size());

  EXPECT_TRUE(patch_writer_.Close());
}

}  // namespace bsdiff
