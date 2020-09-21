// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/bsdiff_arguments.h"

#include <vector>

#include <gtest/gtest.h>

namespace bsdiff {

TEST(BsdiffArgumentsTest, ParseCompressorTypeTest) {
  CompressorType type;
  EXPECT_TRUE(BsdiffArguments::ParseCompressorType("Brotli", &type));
  EXPECT_EQ(CompressorType::kBrotli, type);

  EXPECT_TRUE(BsdiffArguments::ParseCompressorType("bz2", &type));
  EXPECT_EQ(CompressorType::kBZ2, type);

  EXPECT_FALSE(BsdiffArguments::ParseCompressorType("invalid", &type));
}

TEST(BsdiffArgumentsTest, ParseBsdiffFormatTest) {
  BsdiffFormat format;
  EXPECT_TRUE(BsdiffArguments::ParseBsdiffFormat("bsdf2", &format));
  EXPECT_EQ(BsdiffFormat::kBsdf2, format);

  EXPECT_TRUE(BsdiffArguments::ParseBsdiffFormat("Legacy", &format));
  EXPECT_EQ(BsdiffFormat::kLegacy, format);

  EXPECT_TRUE(BsdiffArguments::ParseBsdiffFormat("bsdiff40", &format));
  EXPECT_EQ(BsdiffFormat::kLegacy, format);

  EXPECT_TRUE(BsdiffArguments::ParseBsdiffFormat("endsley", &format));
  EXPECT_EQ(BsdiffFormat::kEndsley, format);

  EXPECT_FALSE(BsdiffArguments::ParseBsdiffFormat("Other", &format));
}

TEST(BsdiffArgumentsTest, ParseQualityTest) {
  int quality;
  EXPECT_TRUE(BsdiffArguments::ParseQuality("9", &quality));
  EXPECT_EQ(9, quality);

  // Check the out of range quality values.
  EXPECT_FALSE(BsdiffArguments::ParseQuality("30", &quality));
  EXPECT_FALSE(BsdiffArguments::ParseQuality("1234567890", &quality));
  EXPECT_FALSE(BsdiffArguments::ParseQuality("aabb", &quality));
}

TEST(BsdiffArgumentsTest, ParseMinLengthTest) {
  size_t len;
  EXPECT_TRUE(BsdiffArguments::ParseMinLength("11", &len));
  EXPECT_EQ(11U, len);

  // Check the out of range quality values.
  EXPECT_FALSE(BsdiffArguments::ParseMinLength("-1", &len));
  EXPECT_FALSE(BsdiffArguments::ParseMinLength("aabb", &len));
}

TEST(BsdiffArgumentsTest, ArgumentsValidTest) {
  // Default arguments using BsdiffFormat::kLegacy and CompressorType::kBZ2
  // should be valid.
  EXPECT_TRUE(BsdiffArguments().IsValid());

  // brotli is not supported for BsdiffFormat::kLegacy.
  EXPECT_FALSE(
      BsdiffArguments(BsdiffFormat::kLegacy, CompressorType::kBrotli, -1)
          .IsValid());

  EXPECT_TRUE(BsdiffArguments(BsdiffFormat::kBsdf2, CompressorType::kBrotli, 9)
                  .IsValid());

  // Compression quality out of range for brotli.
  EXPECT_FALSE(
      BsdiffArguments(BsdiffFormat::kBsdf2, CompressorType::kBrotli, 20)
          .IsValid());
}

TEST(BsdiffArgumentsTest, ParseArgumentsSmokeTest) {
  std::vector<const char*> args = {"bsdiff", "--format=bsdf2", "--type=brotli",
                                   "--quality=9", "--minlen=12"};

  BsdiffArguments arguments;
  EXPECT_TRUE(
      arguments.ParseCommandLine(args.size(), const_cast<char**>(args.data())));

  EXPECT_EQ(BsdiffFormat::kBsdf2, arguments.format());
  EXPECT_EQ(CompressorType::kBrotli, arguments.compressor_type());
  EXPECT_EQ(9, arguments.compression_quality());
  EXPECT_EQ(12, arguments.min_length());
}

}  // namespace bsdiff
