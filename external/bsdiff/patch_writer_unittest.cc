// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/patch_writer.h"

#include <gtest/gtest.h>

#include "bsdiff/test_utils.h"

namespace {

// Generated with:
// echo 'Hello World' | hexdump -v -e '"    " 12/1 "0x%02x, " "\n"'
const uint8_t kHelloWorld[] = {
    0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x0a,
};

// Compressed empty file.
// bzip2 -9 </dev/null | hexdump -v -e '"    " 7/1 "0x%02x, " "\n"'
const uint8_t kCompressedEmpty[] = {0x42, 0x5a, 0x68, 0x39, 0x17, 0x72, 0x45,
                                    0x38, 0x50, 0x90, 0x00, 0x00, 0x00, 0x00};

// echo 'Hello World' | bzip2 -9 | hexdump -v -e '"    " 12/1 "0x%02x, " "\n"'
const uint8_t kCompressedHelloWorld[] = {
    0x42, 0x5a, 0x68, 0x39, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59, 0xd8, 0x72,
    0x01, 0x2f, 0x00, 0x00, 0x01, 0x57, 0x80, 0x00, 0x10, 0x40, 0x00, 0x00,
    0x40, 0x00, 0x80, 0x06, 0x04, 0x90, 0x00, 0x20, 0x00, 0x22, 0x06, 0x86,
    0xd4, 0x20, 0xc9, 0x88, 0xc7, 0x69, 0xe8, 0x28, 0x1f, 0x8b, 0xb9, 0x22,
    0x9c, 0x28, 0x48, 0x6c, 0x39, 0x00, 0x97, 0x80,
};

// Compressed a buffer of zeros of the same length of the kHelloWorld.
// head -c `echo 'Hello World' | wc -c` /dev/zero | bzip2 -9 |
//   hexdump -v -e '"    " 10/1 "0x%02x, " "\n"'
const uint8_t kCompressedZeros[] = {
    0x42, 0x5a, 0x68, 0x39, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59,
    0xf6, 0x63, 0xab, 0xde, 0x00, 0x00, 0x00, 0x40, 0x00, 0x40,
    0x40, 0x20, 0x00, 0x21, 0x00, 0x82, 0x83, 0x17, 0x72, 0x45,
    0x38, 0x50, 0x90, 0xf6, 0x63, 0xab, 0xde,
};

}  // namespace

namespace bsdiff {

class BsdiffPatchWriterTest : public testing::Test {
 protected:
  void SetUp() override {
    // This patch writer doesn't use the |new_size| value passed on init, so we
    // don't pass any meaningful value.
    EXPECT_TRUE(patch_writer_.Init(0));
  }

  test_utils::ScopedTempFile patch_file_{"bsdiff_newfile.XXXXXX"};
  BsdiffPatchWriter patch_writer_{patch_file_.filename()};
};

TEST_F(BsdiffPatchWriterTest, CreateEmptyPatchTest) {
  EXPECT_TRUE(patch_writer_.Close());

  test_utils::BsdiffPatchFile patch;
  EXPECT_TRUE(patch.LoadFromFile(patch_file_.filename()));
  EXPECT_TRUE(patch.IsValid());

  // An empty bz2 file will have 14 bytes.
  EXPECT_EQ(sizeof(kCompressedEmpty), static_cast<uint64_t>(patch.diff_len));
  EXPECT_EQ(sizeof(kCompressedEmpty), patch.extra_len);
}

TEST_F(BsdiffPatchWriterTest, AllInExtraStreamTest) {
  // Write to the extra stream in two parts: first 5 bytes, then the rest.
  EXPECT_TRUE(patch_writer_.AddControlEntry(ControlEntry(0, 5, 0)));
  EXPECT_TRUE(patch_writer_.AddControlEntry(
      ControlEntry(0, sizeof(kHelloWorld) - 5, 0)));
  EXPECT_TRUE(patch_writer_.WriteExtraStream(kHelloWorld, sizeof(kHelloWorld)));
  EXPECT_TRUE(patch_writer_.Close());

  test_utils::BsdiffPatchFile patch;
  EXPECT_TRUE(patch.LoadFromFile(patch_file_.filename()));
  EXPECT_TRUE(patch.IsValid());
  EXPECT_EQ(patch.bz2_diff,
            std::vector<uint8_t>(kCompressedEmpty,
                                 kCompressedEmpty + sizeof(kCompressedEmpty)));
  EXPECT_EQ(patch.bz2_extra,
            std::vector<uint8_t>(
                kCompressedHelloWorld,
                kCompressedHelloWorld + sizeof(kCompressedHelloWorld)));

  EXPECT_EQ(static_cast<int64_t>(sizeof(kHelloWorld)), patch.new_file_len);
}

TEST_F(BsdiffPatchWriterTest, AllInDiffStreamTest) {
  // Write to the extra stream in two parts: first 5 bytes, then the rest.
  EXPECT_TRUE(
      patch_writer_.AddControlEntry(ControlEntry(sizeof(kHelloWorld), 0, 0)));
  std::vector<uint8_t> zeros(sizeof(kHelloWorld), 0);
  EXPECT_TRUE(patch_writer_.WriteDiffStream(zeros.data(), zeros.size()));
  EXPECT_TRUE(patch_writer_.Close());

  test_utils::BsdiffPatchFile patch;
  EXPECT_TRUE(patch.LoadFromFile(patch_file_.filename()));
  EXPECT_TRUE(patch.IsValid());
  EXPECT_EQ(patch.bz2_extra,
            std::vector<uint8_t>(kCompressedEmpty,
                                 kCompressedEmpty + sizeof(kCompressedEmpty)));
  EXPECT_EQ(patch.bz2_diff,
            std::vector<uint8_t>(kCompressedZeros,
                                 kCompressedZeros + sizeof(kCompressedZeros)));

  EXPECT_EQ(static_cast<int64_t>(sizeof(kHelloWorld)), patch.new_file_len);
}

}  // namespace bsdiff
