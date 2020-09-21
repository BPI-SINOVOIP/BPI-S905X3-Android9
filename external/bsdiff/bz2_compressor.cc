// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/bz2_compressor.h"

#include <string.h>

#include "bsdiff/logging.h"

namespace {

// The BZ2 compression level used. Smaller compression levels are nowadays
// pointless.
const int kCompressionLevel = 9;

}  // namespace

namespace bsdiff {

BZ2Compressor::BZ2Compressor() : comp_buffer_(1024 * 1024) {
  memset(&bz_strm_, 0, sizeof(bz_strm_));
  int bz_err = BZ2_bzCompressInit(&bz_strm_, kCompressionLevel,
                                  0 /* verbosity */, 0 /* workFactor */);
  if (bz_err != BZ_OK) {
    LOG(ERROR) << "Initializing bz_strm, bz_error=" << bz_err;
  } else {
    bz_strm_initialized_ = true;
  }
}

BZ2Compressor::~BZ2Compressor() {
  if (!bz_strm_initialized_)
    return;
  int bz_err = BZ2_bzCompressEnd(&bz_strm_);
  if (bz_err != BZ_OK) {
    LOG(ERROR) << "Deleting the compressor stream, bz_error=" << bz_err;
  }
}

bool BZ2Compressor::Write(const uint8_t* buf, size_t size) {
  if (!bz_strm_initialized_)
    return false;

  // The bz_stream struct defines the next_in as a non-const data pointer,
  // although the documentation says it won't modify it.
  bz_strm_.next_in = reinterpret_cast<char*>(const_cast<uint8_t*>(buf));
  bz_strm_.avail_in = size;

  while (bz_strm_.avail_in) {
    bz_strm_.next_out = reinterpret_cast<char*>(comp_buffer_.buffer_data());
    bz_strm_.avail_out = comp_buffer_.buffer_size();
    int bz_err = BZ2_bzCompress(&bz_strm_, BZ_RUN);
    if (bz_err != BZ_RUN_OK) {
      LOG(ERROR) << "Compressing data, bz_error=" << bz_err;
      return false;
    }

    uint64_t output_bytes = comp_buffer_.buffer_size() - bz_strm_.avail_out;
    if (output_bytes) {
      comp_buffer_.AddDataToChunks(output_bytes);
    }
  }
  return true;
}

bool BZ2Compressor::Finish() {
  if (!bz_strm_initialized_)
    return false;
  bz_strm_.next_in = nullptr;
  bz_strm_.avail_in = 0;

  int bz_err = BZ_FINISH_OK;
  while (bz_err == BZ_FINISH_OK) {
    bz_strm_.next_out = reinterpret_cast<char*>(comp_buffer_.buffer_data());
    bz_strm_.avail_out = comp_buffer_.buffer_size();
    bz_err = BZ2_bzCompress(&bz_strm_, BZ_FINISH);

    uint64_t output_bytes = comp_buffer_.buffer_size() - bz_strm_.avail_out;
    if (output_bytes) {
      comp_buffer_.AddDataToChunks(output_bytes);
    }
  }
  if (bz_err != BZ_STREAM_END) {
    LOG(ERROR) << "Finishing compressing data, bz_error=" << bz_err;
    return false;
  }
  bz_err = BZ2_bzCompressEnd(&bz_strm_);
  bz_strm_initialized_ = false;
  if (bz_err != BZ_OK) {
    LOG(ERROR) << "Deleting the compressor stream, bz_error=" << bz_err;
    return false;
  }
  return true;
}

const std::vector<uint8_t>& BZ2Compressor::GetCompressedData() {
  return comp_buffer_.GetCompressedData();
}

}  // namespace bsdiff
