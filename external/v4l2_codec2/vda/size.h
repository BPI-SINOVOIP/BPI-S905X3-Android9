// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: a8e9f71
// Note: only necessary functions are ported from gfx::Size

#ifndef SIZE_H_
#define SIZE_H_

#include <string>

#include "base/strings/stringprintf.h"

namespace media {

// Helper struct for size to replace gfx::size usage from original code.
// Only partial functions of gfx::size is implemented here.
struct Size {
 public:
  Size() : width_(0), height_(0) {}
  Size(int width, int height)
      : width_(width < 0 ? 0 : width), height_(height < 0 ? 0 : height) {}

  constexpr int width() const { return width_; }
  constexpr int height() const { return height_; }

  void set_width(int width) { width_ = width < 0 ? 0 : width; }
  void set_height(int height) { height_ = height < 0 ? 0 : height; }

  void SetSize(int width, int height) {
    set_width(width);
    set_height(height);
  }

  bool IsEmpty() const { return !width() || !height(); }

  std::string ToString() const {
    return base::StringPrintf("%dx%d", width(), height());
  }

  Size& operator=(const Size& ps) {
    set_width(ps.width());
    set_height(ps.height());
    return *this;
  }

 private:
  int width_;
  int height_;
};

inline bool operator==(const Size& lhs, const Size& rhs) {
  return lhs.width() == rhs.width() && lhs.height() == rhs.height();
}

inline bool operator!=(const Size& lhs, const Size& rhs) {
  return !(lhs == rhs);
}

}  // namespace media

#endif  // SIZE_H_
