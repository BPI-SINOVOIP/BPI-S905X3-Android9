// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: 6e70beb

#include "vp9_picture.h"

namespace media {

VP9Picture::VP9Picture() {}

VP9Picture::~VP9Picture() {}

V4L2VP9Picture* VP9Picture::AsV4L2VP9Picture() {
  return nullptr;
}

}  // namespace media
