// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/utils.h"

namespace bsdiff {

int64_t ParseInt64(const uint8_t* buf) {
  int64_t result = buf[7] & 0x7F;
  for (int i = 6; i >= 0; i--) {
    result <<= 8;
    result |= buf[i];
  }

  if (buf[7] & 0x80)
    result = -result;
  return result;
}

}  // namespace bsdiff
