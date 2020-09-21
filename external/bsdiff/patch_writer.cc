// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/patch_writer.h"

#include <string.h>

#include "bsdiff/brotli_compressor.h"
#include "bsdiff/bz2_compressor.h"
#include "bsdiff/constants.h"
#include "bsdiff/control_entry.h"
#include "bsdiff/logging.h"

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

BsdiffPatchWriter::BsdiffPatchWriter(const std::string& patch_filename)
    : patch_filename_(patch_filename), format_(BsdiffFormat::kLegacy) {
  ctrl_stream_.reset(new BZ2Compressor());
  diff_stream_.reset(new BZ2Compressor());
  extra_stream_.reset(new BZ2Compressor());
}

BsdiffPatchWriter::BsdiffPatchWriter(const std::string& patch_filename,
                                     CompressorType type,
                                     int quality)
    : patch_filename_(patch_filename), format_(BsdiffFormat::kBsdf2) {
  if (type == CompressorType::kBZ2) {
    ctrl_stream_.reset(new BZ2Compressor());
    diff_stream_.reset(new BZ2Compressor());
    extra_stream_.reset(new BZ2Compressor());
  } else if (type == CompressorType::kBrotli) {
    ctrl_stream_.reset(new BrotliCompressor(quality));
    diff_stream_.reset(new BrotliCompressor(quality));
    extra_stream_.reset(new BrotliCompressor(quality));
  }
}

bool BsdiffPatchWriter::Init(size_t /* new_size */) {
  if (!(ctrl_stream_ && diff_stream_ && extra_stream_)) {
    LOG(ERROR) << "uninitialized compressor stream";
    return false;
  }

  fp_ = fopen(patch_filename_.c_str(), "w");
  if (!fp_) {
    LOG(ERROR) << "Opening " << patch_filename_;
    return false;
  }
  return true;
}

bool BsdiffPatchWriter::WriteDiffStream(const uint8_t* data, size_t size) {
  return diff_stream_->Write(data, size);
}

bool BsdiffPatchWriter::WriteExtraStream(const uint8_t* data, size_t size) {
  return extra_stream_->Write(data, size);
}

bool BsdiffPatchWriter::AddControlEntry(const ControlEntry& entry) {
  // Generate the 24 byte control entry.
  uint8_t buf[24];
  EncodeInt64(entry.diff_size, buf);
  EncodeInt64(entry.extra_size, buf + 8);
  EncodeInt64(entry.offset_increment, buf + 16);
  if (!ctrl_stream_->Write(buf, sizeof(buf)))
    return false;
  written_output_ += entry.diff_size + entry.extra_size;
  return true;
}

bool BsdiffPatchWriter::Close() {
  if (!fp_) {
    LOG(ERROR) << "File not open.";
    return false;
  }

  if (!ctrl_stream_->Finish() || !diff_stream_->Finish() ||
      !extra_stream_->Finish()) {
    LOG(ERROR) << "Finalizing compressed streams.";
    return false;
  }

  auto ctrl_data = ctrl_stream_->GetCompressedData();
  auto diff_data = diff_stream_->GetCompressedData();
  auto extra_data = extra_stream_->GetCompressedData();

  if (!WriteHeader(ctrl_data.size(), diff_data.size()))
    return false;

  if (fwrite(ctrl_data.data(), 1, ctrl_data.size(), fp_) != ctrl_data.size()) {
    LOG(ERROR) << "Writing ctrl_data.";
    return false;
  }
  if (fwrite(diff_data.data(), 1, diff_data.size(), fp_) != diff_data.size()) {
    LOG(ERROR) << "Writing diff_data.";
    return false;
  }
  if (fwrite(extra_data.data(), 1, extra_data.size(), fp_) !=
      extra_data.size()) {
    LOG(ERROR) << "Writing extra_data.";
    return false;
  }
  if (fclose(fp_) != 0) {
    LOG(ERROR) << "Closing the patch file.";
    return false;
  }
  fp_ = nullptr;
  return true;
}

bool BsdiffPatchWriter::WriteHeader(uint64_t ctrl_size, uint64_t diff_size) {
  /* Header format is
   * 0 8 magic header
   * 8 8 length of compressed ctrl block
   * 16  8 length of compressed diff block
   * 24  8 length of new file
   *
   * File format is
   * 0 32  Header
   * 32  ??  compressed ctrl block
   * ??  ??  compressed diff block
   * ??  ??  compressed extra block
   */
  uint8_t header[32];
  if (format_ == BsdiffFormat::kLegacy) {
    // The magic header is "BSDIFF40" for legacy format.
    memcpy(header, kLegacyMagicHeader, 8);
  } else if (format_ == BsdiffFormat::kBsdf2) {
    // The magic header for BSDF2 format:
    // 0 5 BSDF2
    // 5 1 compressed type for control stream
    // 6 1 compressed type for diff stream
    // 7 1 compressed type for extra stream
    memcpy(header, kBSDF2MagicHeader, 5);
    header[5] = static_cast<uint8_t>(ctrl_stream_->Type());
    header[6] = static_cast<uint8_t>(diff_stream_->Type());
    header[7] = static_cast<uint8_t>(extra_stream_->Type());
  } else {
    LOG(ERROR) << "Unsupported bsdiff format.";
    return false;
  }

  EncodeInt64(ctrl_size, header + 8);
  EncodeInt64(diff_size, header + 16);
  EncodeInt64(written_output_, header + 24);
  if (fwrite(header, sizeof(header), 1, fp_) != 1) {
    LOG(ERROR) << "writing to the patch file";
    return false;
  }
  return true;
}

}  // namespace bsdiff
