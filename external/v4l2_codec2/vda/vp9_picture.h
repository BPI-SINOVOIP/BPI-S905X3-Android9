// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: 70340ce

#ifndef VP9_PICTURE_H_
#define VP9_PICTURE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "rect.h"
#include "vp9_parser.h"

namespace media {

class V4L2VP9Picture;

class VP9Picture : public base::RefCountedThreadSafe<VP9Picture> {
 public:
  VP9Picture();

  virtual V4L2VP9Picture* AsV4L2VP9Picture();

  std::unique_ptr<Vp9FrameHeader> frame_hdr;

  // The visible size of picture. This could be either parsed from frame
  // header, or set to Rect(0, 0) for indicating invalid values or
  // not available.
  Rect visible_rect;

 protected:
  friend class base::RefCountedThreadSafe<VP9Picture>;
  virtual ~VP9Picture();

  DISALLOW_COPY_AND_ASSIGN(VP9Picture);
};

}  // namespace media

#endif  // VP9_PICTURE_H_
