// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/brotli_compressor.h"

#include <gtest/gtest.h>

#include "bsdiff/brotli_decompressor.h"

namespace {

constexpr uint8_t kHelloWorld[] = {
    0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x0a,
};
}  // namespace

namespace bsdiff {

TEST(BrotliCompressorTest, BrotliCompressorSmoke) {
  BrotliCompressor brotli_compressor(11);
  EXPECT_TRUE(brotli_compressor.Write(kHelloWorld, sizeof(kHelloWorld)));
  EXPECT_TRUE(brotli_compressor.Finish());
  std::vector<uint8_t> compressed_data = brotli_compressor.GetCompressedData();
  EXPECT_GT(compressed_data.size(), static_cast<size_t>(0));

  // Run decompressor and check we can get the exact same data as kHelloWorld.
  std::vector<uint8_t> decompressed_data(sizeof(kHelloWorld));
  BrotliDecompressor brotli_decompressor;
  EXPECT_TRUE(brotli_decompressor.SetInputData(compressed_data.data(),
                                               compressed_data.size()));
  EXPECT_TRUE(
      brotli_decompressor.Read(decompressed_data.data(), sizeof(kHelloWorld)));
  EXPECT_TRUE(brotli_decompressor.Close());
  EXPECT_EQ(
      std::vector<uint8_t>(kHelloWorld, kHelloWorld + sizeof(kHelloWorld)),
      decompressed_data);
}

TEST(BrotliCompressorTest, BrotliCompressorEmptyStream) {
  uint8_t empty_buffer[] = {};
  BrotliCompressor brotli_compressor(11);
  EXPECT_TRUE(brotli_compressor.Write(empty_buffer, sizeof(empty_buffer)));
  EXPECT_TRUE(brotli_compressor.Finish());

  std::vector<uint8_t> compressed_data = brotli_compressor.GetCompressedData();

  // Check that we can close the decompressor without errors.
  BrotliDecompressor brotli_decompressor;
  EXPECT_TRUE(brotli_decompressor.SetInputData(compressed_data.data(),
                                               compressed_data.size()));
  EXPECT_TRUE(brotli_decompressor.Close());
}

}  // namespace bsdiff
