// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/decompressor_interface.h"

#include "bsdiff/brotli_decompressor.h"
#include "bsdiff/bz2_decompressor.h"
#include "bsdiff/logging.h"

namespace bsdiff {

std::unique_ptr<DecompressorInterface> CreateDecompressor(CompressorType type) {
  switch (type) {
    case CompressorType::kBZ2:
      return std::unique_ptr<DecompressorInterface>(new BZ2Decompressor());
    case CompressorType::kBrotli:
      return std::unique_ptr<DecompressorInterface>(new BrotliDecompressor());
    default:
      LOG(ERROR) << "unsupported compressor type: "
                 << static_cast<uint8_t>(type);
      return nullptr;
  }
}

}  // namespace bsdiff
