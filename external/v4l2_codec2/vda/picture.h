// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: d264e47

#ifndef PICTURE_H_
#define PICTURE_H_

#include <stdint.h>

#include <vector>

#include "rect.h"
#include "size.h"
#include "video_pixel_format.h"

namespace media {

// A picture buffer that has size and pixel format information.
class PictureBuffer {
 public:
  PictureBuffer(int32_t id, const Size& size);
  PictureBuffer(int32_t id,
                const Size& size,
                VideoPixelFormat pixel_format);
  PictureBuffer(const PictureBuffer& other);
  ~PictureBuffer();

  // Returns the client-specified id of the buffer.
  int32_t id() const { return id_; }

  // Returns the size of the buffer.
  Size size() const { return size_; }

  void set_size(const Size& size) { size_ = size; }

  VideoPixelFormat pixel_format() const { return pixel_format_; }

 private:
  int32_t id_;
  Size size_;
  VideoPixelFormat pixel_format_ = PIXEL_FORMAT_UNKNOWN;
};

// A decoded picture frame.
class Picture {
 public:
  Picture(int32_t picture_buffer_id,
          int32_t bitstream_buffer_id,
          const Rect& visible_rect,
          bool allow_overlay);
  Picture(const Picture&);
  ~Picture();

  // Returns the id of the picture buffer where this picture is contained.
  int32_t picture_buffer_id() const { return picture_buffer_id_; }

  // Returns the id of the bitstream buffer from which this frame was decoded.
  int32_t bitstream_buffer_id() const { return bitstream_buffer_id_; }

  void set_bitstream_buffer_id(int32_t bitstream_buffer_id) {
    bitstream_buffer_id_ = bitstream_buffer_id;
  }

  // Returns the visible rectangle of the picture. Its size may be smaller
  // than the size of the PictureBuffer, as it is the only visible part of the
  // Picture contained in the PictureBuffer.
  Rect visible_rect() const { return visible_rect_; }

  bool allow_overlay() const { return allow_overlay_; }

 private:
  int32_t picture_buffer_id_;
  int32_t bitstream_buffer_id_;
  Rect visible_rect_;
  bool allow_overlay_;
};

}  // namespace media

#endif  // PICTURE_H_
