// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _BSDIFF_UTILS_H_
#define _BSDIFF_UTILS_H_

#include <stdint.h>

namespace bsdiff {

int64_t ParseInt64(const uint8_t* buf);

}  // namespace bsdiff

#endif
