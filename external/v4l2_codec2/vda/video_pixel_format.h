// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: 006301b
// Note: only necessary functions are ported from video_types.h

#ifndef VIDEO_PIXEL_FORMAT_H_
#define VIDEO_PIXEL_FORMAT_H_

namespace media {

// Pixel formats roughly based on FOURCC labels, see:
// http://www.fourcc.org/rgb.php and http://www.fourcc.org/yuv.php
// Logged to UMA, so never reuse values. Leave gaps if necessary.
// Ordered as planar, semi-planar, YUV-packed, and RGB formats.
enum VideoPixelFormat {
  PIXEL_FORMAT_UNKNOWN = 0,  // Unknown or unspecified format value.
  PIXEL_FORMAT_I420 =
      1,  // 12bpp YUV planar 1x1 Y, 2x2 UV samples, a.k.a. YU12.

  // Note: Chrome does not actually support YVU compositing, so you probably
  // don't actually want to use this. See http://crbug.com/784627.
  PIXEL_FORMAT_YV12 = 2,  // 12bpp YVU planar 1x1 Y, 2x2 VU samples.

  PIXEL_FORMAT_I422 = 3,   // 16bpp YUV planar 1x1 Y, 2x1 UV samples.
  PIXEL_FORMAT_I420A = 4,  // 20bpp YUVA planar 1x1 Y, 2x2 UV, 1x1 A samples.
  PIXEL_FORMAT_I444 = 5,   // 24bpp YUV planar, no subsampling.
  PIXEL_FORMAT_NV12 =
      6,  // 12bpp with Y plane followed by a 2x2 interleaved UV plane.
  PIXEL_FORMAT_NV21 =
      7,  // 12bpp with Y plane followed by a 2x2 interleaved VU plane.
  PIXEL_FORMAT_UYVY =
      8,  // 16bpp interleaved 2x1 U, 1x1 Y, 2x1 V, 1x1 Y samples.
  PIXEL_FORMAT_YUY2 =
      9,  // 16bpp interleaved 1x1 Y, 2x1 U, 1x1 Y, 2x1 V samples.
  PIXEL_FORMAT_ARGB = 10,   // 32bpp ARGB, 1 plane.
  PIXEL_FORMAT_XRGB = 11,   // 24bpp XRGB, 1 plane.
  PIXEL_FORMAT_RGB24 = 12,  // 24bpp BGR, 1 plane.
  PIXEL_FORMAT_RGB32 = 13,  // 32bpp BGRA, 1 plane.
  PIXEL_FORMAT_MJPEG = 14,  // MJPEG compressed.
  // MediaTek proprietary format. MT21 is similar to NV21 except the memory
  // layout and pixel layout (swizzles). 12bpp with Y plane followed by a 2x2
  // interleaved VU plane. Each image contains two buffers -- Y plane and VU
  // plane. Two planes can be non-contiguous in memory. The starting addresses
  // of Y plane and VU plane are 4KB alignment.
  // Suppose image dimension is (width, height). For both Y plane and VU plane:
  // Row pitch = ((width+15)/16) * 16.
  // Plane size = Row pitch * (((height+31)/32)*32)
  PIXEL_FORMAT_MT21 = 15,

  // The P* in the formats below designates the number of bits per pixel. I.e.
  // P9 is 9-bits per pixel, P10 is 10-bits per pixel, etc.
  PIXEL_FORMAT_YUV420P9 = 16,
  PIXEL_FORMAT_YUV420P10 = 17,
  PIXEL_FORMAT_YUV422P9 = 18,
  PIXEL_FORMAT_YUV422P10 = 19,
  PIXEL_FORMAT_YUV444P9 = 20,
  PIXEL_FORMAT_YUV444P10 = 21,

  PIXEL_FORMAT_YUV420P12 = 22,
  PIXEL_FORMAT_YUV422P12 = 23,
  PIXEL_FORMAT_YUV444P12 = 24,

  /* PIXEL_FORMAT_Y8 = 25, Deprecated */
  PIXEL_FORMAT_Y16 = 26,  // single 16bpp plane.

  // Please update UMA histogram enumeration when adding new formats here.
  PIXEL_FORMAT_MAX =
      PIXEL_FORMAT_Y16,  // Must always be equal to largest entry logged.
};

}  // namespace media

#endif  // VIDEO_PIXEL_FORMAT_H_
