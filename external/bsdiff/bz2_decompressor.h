// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_BZ2_DECOMPRESSOR_H_
#define _BSDIFF_BZ2_DECOMPRESSOR_H_

#include <bzlib.h>

#include "bsdiff/decompressor_interface.h"

namespace bsdiff {

class BZ2Decompressor : public DecompressorInterface {
 public:
  BZ2Decompressor() = default;

  bool SetInputData(const uint8_t* input_data, size_t size) override;

  bool Read(uint8_t* output_data, size_t bytes_to_output) override;

  bool Close() override;

 private:
  bz_stream stream_;
};

}  // namespace bsdiff

#endif
