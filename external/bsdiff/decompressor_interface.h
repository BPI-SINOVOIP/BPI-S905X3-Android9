// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_DECOMPRESSOR_INTERFACE_H_
#define _BSDIFF_DECOMPRESSOR_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "bsdiff/constants.h"

namespace bsdiff {

class DecompressorInterface {
 public:
  virtual ~DecompressorInterface() {}

  // Set the buffer starting from |input_data| with length |size| as the input
  // compressed stream. This buffer is owned by the caller and should stay
  // valid until the Close() function is called.
  virtual bool SetInputData(const uint8_t* input_data, size_t size) = 0;

  // Decompress the stream and output |bytes_to_output| bytes of data to
  // |output_data|. Returns false if not all bytes_to_output can be
  // decompressed from the stream due to a decompressor error or EOF.
  virtual bool Read(uint8_t* output_data, size_t bytes_to_output) = 0;

  // Close the decompression stream.
  virtual bool Close() = 0;
};

std::unique_ptr<DecompressorInterface> CreateDecompressor(CompressorType type);

}  // namespace bsdiff
#endif  // _BSDIFF_DECOMPRESSOR_INTERFACE_H_
