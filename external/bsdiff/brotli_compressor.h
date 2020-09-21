// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_BROTLI_COMPRESSOR_H_
#define _BSDIFF_BROTLI_COMPRESSOR_H_

#include <stdint.h>

#include <vector>

#include <brotli/encode.h>

#include "bsdiff/compressor_buffer.h"
#include "bsdiff/compressor_interface.h"
#include "bsdiff/constants.h"

namespace bsdiff {

class BrotliCompressor : public CompressorInterface {
 public:
  // Create a brotli compressor with the compression quality |quality|. As the
  // value of quality increases, the compression becomes better but slower.
  // The valid range of quality is between BROTLI_MIN_QUALITY and
  // BROTLI_MAX_QUALITY; and the caller is responsible for the validity check.
  explicit BrotliCompressor(int quality);
  ~BrotliCompressor() override;

  // CompressorInterface overrides.
  bool Write(const uint8_t* buf, size_t size) override;
  bool Finish() override;
  const std::vector<uint8_t>& GetCompressedData() override;
  CompressorType Type() override { return CompressorType::kBrotli; }

 private:
  BrotliEncoderState* brotli_encoder_state_;

  CompressorBuffer comp_buffer_;
};

}  // namespace bsdiff

#endif  // _BSDIFF_BROTLI_COMPRESSOR_H_
