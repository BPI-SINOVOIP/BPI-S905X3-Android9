// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: 2de6929

#include "picture.h"

namespace media {

PictureBuffer::PictureBuffer(int32_t id, const Size& size)
    : id_(id), size_(size) {}

PictureBuffer::PictureBuffer(int32_t id,
                             const Size& size,
                             VideoPixelFormat pixel_format)
    : id_(id),
      size_(size),
      pixel_format_(pixel_format) {}

PictureBuffer::PictureBuffer(const PictureBuffer& other) = default;

PictureBuffer::~PictureBuffer() = default;

Picture::Picture(int32_t picture_buffer_id,
                 int32_t bitstream_buffer_id,
                 const Rect& visible_rect,
                 bool allow_overlay)
    : picture_buffer_id_(picture_buffer_id),
      bitstream_buffer_id_(bitstream_buffer_id),
      visible_rect_(visible_rect),
      allow_overlay_(allow_overlay) {}

Picture::Picture(const Picture& other) = default;

Picture::~Picture() = default;

}  // namespace media
