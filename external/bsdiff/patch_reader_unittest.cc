// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/patch_reader.h"

#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "bsdiff/brotli_compressor.h"
#include "bsdiff/bz2_compressor.h"
#include "bsdiff/utils.h"

namespace {

void EncodeInt64(int64_t x, uint8_t* buf) {
  uint64_t y = x < 0 ? (1ULL << 63ULL) - x : x;
  for (int i = 0; i < 8; ++i) {
    buf[i] = y & 0xff;
    y /= 256;
  }
}

}  // namespace

namespace bsdiff {

class PatchReaderTest : public testing::Test {
 protected:
  void CompressData() {
    for (size_t i = 0; i < diff_data_.size(); i++) {
      uint8_t buf[24];
      EncodeInt64(diff_data_[i].size(), buf);
      EncodeInt64(extra_data_[i].size(), buf + 8);
      EncodeInt64(offset_increment_[i], buf + 16);
      EXPECT_TRUE(ctrl_stream_->Write(buf, sizeof(buf)));
      EXPECT_TRUE(diff_stream_->Write(
          reinterpret_cast<const uint8_t*>(diff_data_[i].data()),
          diff_data_[i].size()));
      EXPECT_TRUE(extra_stream_->Write(
          reinterpret_cast<const uint8_t*>(extra_data_[i].data()),
          extra_data_[i].size()));
    }
    EXPECT_TRUE(ctrl_stream_->Finish());
    EXPECT_TRUE(diff_stream_->Finish());
    EXPECT_TRUE(extra_stream_->Finish());
  }

  void ConstructPatchData(std::vector<uint8_t>* patch_data) {
    EXPECT_EQ(static_cast<size_t>(8), patch_data->size());
    // Encode the header
    uint8_t buf[24];
    EncodeInt64(ctrl_stream_->GetCompressedData().size(), buf);
    EncodeInt64(diff_stream_->GetCompressedData().size(), buf + 8);
    EncodeInt64(new_file_size_, buf + 16);
    std::copy(buf, buf + sizeof(buf), std::back_inserter(*patch_data));

    // Concatenate the three streams into one patch.
    std::copy(ctrl_stream_->GetCompressedData().begin(),
              ctrl_stream_->GetCompressedData().end(),
              std::back_inserter(*patch_data));
    std::copy(diff_stream_->GetCompressedData().begin(),
              diff_stream_->GetCompressedData().end(),
              std::back_inserter(*patch_data));
    std::copy(extra_stream_->GetCompressedData().begin(),
              extra_stream_->GetCompressedData().end(),
              std::back_inserter(*patch_data));
  }

  void VerifyPatch(const std::vector<uint8_t>& patch_data) {
    BsdiffPatchReader patch_reader;
    EXPECT_TRUE(patch_reader.Init(patch_data.data(), patch_data.size()));
    EXPECT_EQ(new_file_size_, patch_reader.new_file_size());
    // Check that the decompressed data matches what we wrote.
    for (size_t i = 0; i < diff_data_.size(); i++) {
      ControlEntry control_entry(0, 0, 0);
      EXPECT_TRUE(patch_reader.ParseControlEntry(&control_entry));
      EXPECT_EQ(diff_data_[i].size(), control_entry.diff_size);
      EXPECT_EQ(extra_data_[i].size(), control_entry.extra_size);
      EXPECT_EQ(offset_increment_[i], control_entry.offset_increment);

      uint8_t buffer[128] = {};
      EXPECT_TRUE(patch_reader.ReadDiffStream(buffer, diff_data_[i].size()));
      EXPECT_EQ(0, memcmp(buffer, diff_data_[i].data(), diff_data_[i].size()));
      EXPECT_TRUE(patch_reader.ReadExtraStream(buffer, extra_data_[i].size()));
      EXPECT_EQ(0,
                memcmp(buffer, extra_data_[i].data(), extra_data_[i].size()));
    }
    EXPECT_TRUE(patch_reader.Finish());
  }

  size_t new_file_size_{500};
  std::vector<std::string> diff_data_{"HelloWorld", "BspatchPatchTest",
                                      "BspatchDiffData"};
  std::vector<std::string> extra_data_{"HelloWorld!", "BZ2PatchReaderSmoke",
                                       "BspatchExtraData"};
  std::vector<int64_t> offset_increment_{100, 200, 300};

  // The compressor streams.
  std::unique_ptr<CompressorInterface> ctrl_stream_{nullptr};
  std::unique_ptr<CompressorInterface> diff_stream_{nullptr};
  std::unique_ptr<CompressorInterface> extra_stream_{nullptr};
};

TEST_F(PatchReaderTest, PatchReaderLegacyFormatSmoke) {
  ctrl_stream_.reset(new BZ2Compressor());
  diff_stream_.reset(new BZ2Compressor());
  extra_stream_.reset(new BZ2Compressor());

  CompressData();

  std::vector<uint8_t> patch_data;
  std::copy(kLegacyMagicHeader, kLegacyMagicHeader + 8,
            std::back_inserter(patch_data));
  ConstructPatchData(&patch_data);

  VerifyPatch(patch_data);
}

TEST_F(PatchReaderTest, PatchReaderNewFormatSmoke) {
  // Compress the data with one bz2 and two brotli compressors.
  ctrl_stream_.reset(new BZ2Compressor());
  diff_stream_.reset(new BrotliCompressor(11));
  extra_stream_.reset(new BrotliCompressor(11));

  CompressData();

  std::vector<uint8_t> patch_data;
  std::copy(kBSDF2MagicHeader, kBSDF2MagicHeader + 5,
            std::back_inserter(patch_data));
  patch_data.push_back(static_cast<uint8_t>(CompressorType::kBZ2));
  patch_data.push_back(static_cast<uint8_t>(CompressorType::kBrotli));
  patch_data.push_back(static_cast<uint8_t>(CompressorType::kBrotli));
  ConstructPatchData(&patch_data);

  VerifyPatch(patch_data);
}

}  // namespace bsdiff
