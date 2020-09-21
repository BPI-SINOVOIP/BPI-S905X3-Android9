// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_BROTLI_DECOMPRESSOR_H_
#define _BSDIFF_BROTLI_DECOMPRESSOR_H_

#include <brotli/decode.h>

#include "bsdiff/decompressor_interface.h"

namespace bsdiff {

class BrotliDecompressor : public DecompressorInterface {
 public:
  BrotliDecompressor()
      : brotli_decoder_state_(nullptr), next_in_(nullptr), available_in_(0) {}

  // DecompressorInterface overrides.
  bool SetInputData(const uint8_t* input_data, size_t size) override;
  bool Read(uint8_t* output_data, size_t bytes_to_output) override;
  bool Close() override;

 private:
  BrotliDecoderState* brotli_decoder_state_;
  const uint8_t* next_in_;
  size_t available_in_;
};

}  // namespace bsdiff

#endif  // _BSDIFF_BROTLI_DECOMPRESSOR_H_
