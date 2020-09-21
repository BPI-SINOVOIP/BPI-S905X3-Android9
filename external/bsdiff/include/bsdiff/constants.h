// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_CONSTANTS_H_
#define _BSDIFF_CONSTANTS_H_

#include <stdint.h>

namespace bsdiff {

enum class CompressorType : uint8_t {
  kNoCompression = 0,
  kBZ2 = 1,
  kBrotli = 2,
};

// The header of the upstream's "BSDIFF40" format using BZ2 as compressor.
constexpr uint8_t kLegacyMagicHeader[] = "BSDIFF40";

// The header of the new BSDF2 format. This format supports both bz2 and
// brotli compressor.
constexpr uint8_t kBSDF2MagicHeader[] = "BSDF2";

}  // namespace bsdiff

#endif  // _BSDIFF_CONSTANTS_H_
