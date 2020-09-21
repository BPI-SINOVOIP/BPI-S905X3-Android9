// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_BZ2_COMPRESSOR_H_
#define _BSDIFF_BZ2_COMPRESSOR_H_

#include <bzlib.h>
#include <stdint.h>

#include <vector>

#include "bsdiff/compressor_buffer.h"
#include "bsdiff/compressor_interface.h"

namespace bsdiff {

// An in-memory class to wrap the low-level bzip2 compress functions. This class
// allows to stream uncompressed data to it and then retrieve all the compressed
// data at the end of the compression step. For that, all the compressed data
// is stored in memory.

class BZ2Compressor : public CompressorInterface {
 public:
  BZ2Compressor();
  ~BZ2Compressor() override;

  // CompressorInterface overrides.
  bool Write(const uint8_t* buf, size_t size) override;
  bool Finish() override;
  const std::vector<uint8_t>& GetCompressedData() override;
  CompressorType Type() override { return CompressorType::kBZ2; }

 private:
  // The low-level bzip2 stream.
  bz_stream bz_strm_;

  // Whether the bz_strm_ is initialized.
  bool bz_strm_initialized_{false};

  CompressorBuffer comp_buffer_;
};

}  // namespace bsdiff

#endif  // _BSDIFF_BZ2_COMPRESSOR_H_
