// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: 6e70beb

#include "vp8_picture.h"

namespace media {

VP8Picture::VP8Picture() {}

VP8Picture::~VP8Picture() {}

V4L2VP8Picture* VP8Picture::AsV4L2VP8Picture() {
  return nullptr;
}

}  // namespace media
