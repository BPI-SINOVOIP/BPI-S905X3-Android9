// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_COMPRESSOR_INTERFACE_H_
#define _BSDIFF_COMPRESSOR_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "bsdiff/constants.h"

namespace bsdiff {

class CompressorInterface {
 public:
  virtual ~CompressorInterface() = default;

  // Compress and write |size| bytes of input data starting from |buf|.
  virtual bool Write(const uint8_t* buf, size_t size) = 0;

  // Finish the compression step.
  virtual bool Finish() = 0;

  // Return the compressed data. This method must be only called after Finish().
  virtual const std::vector<uint8_t>& GetCompressedData() = 0;

  // Return the type of the current compressor.
  virtual CompressorType Type() = 0;

 protected:
  CompressorInterface() = default;
};

}  // namespace bsdiff

#endif  // _BSDIFF_COMPRESSOR_INTERFACE_H_
