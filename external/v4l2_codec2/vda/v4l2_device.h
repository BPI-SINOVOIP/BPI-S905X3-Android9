// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines the V4L2Device interface which is used by the
// V4L2DecodeAccelerator class to delegate/pass the device specific
// handling of any of the functionalities.
// Note: ported from Chromium commit head: fb70f64
// Note: it's also merged with generic_v4l2_device.h (head: fb70f64)

#ifndef V4L2_DEVICE_H_
#define V4L2_DEVICE_H_

#include <map>
#include <stddef.h>
#include <stdint.h>

#include "base/files/scoped_file.h"
#include "base/memory/ref_counted.h"
#include "size.h"
#include "video_codecs.h"
#include "video_decode_accelerator.h"
#include "video_pixel_format.h"
#include "videodev2.h"

// TODO(posciak): remove this once V4L2 headers are updated.
#define V4L2_PIX_FMT_MT21 v4l2_fourcc('M', 'T', '2', '1')
#ifndef V4L2_BUF_FLAG_LAST
#define V4L2_BUF_FLAG_LAST 0x00100000
#endif

namespace media {

// Implemented for decoder usage only.
class V4L2Device : public base::RefCountedThreadSafe<V4L2Device> {
 public:
  V4L2Device();
  ~V4L2Device();

  // Utility format conversion functions
  static VideoPixelFormat V4L2PixFmtToVideoPixelFormat(uint32_t format);
  static uint32_t VideoPixelFormatToV4L2PixFmt(VideoPixelFormat format);
  static uint32_t VideoCodecProfileToV4L2PixFmt(VideoCodecProfile profile,
                                                bool slice_based);
  std::vector<VideoCodecProfile> V4L2PixFmtToVideoCodecProfiles(
      uint32_t pix_fmt,
      bool is_encoder);

  enum class Type {
    kDecoder,
    kEncoder,
    kImageProcessor,
    kJpegDecoder,
  };

  // Open a V4L2 device of |type| for use with |v4l2_pixfmt|.
  // Return true on success.
  // The device will be closed in the destructor.
  bool Open(Type type, uint32_t v4l2_pixfmt);

  // Parameters and return value are the same as for the standard ioctl() system
  // call.
  int Ioctl(int request, void* arg);

  // This method sleeps until either:
  // - SetDevicePollInterrupt() is called (on another thread),
  // - |poll_device| is true, and there is new data to be read from the device,
  //   or an event from the device has arrived; in the latter case
  //   |*event_pending| will be set to true.
  // Returns false on error, true otherwise.
  // This method should be called from a separate thread.
  bool Poll(bool poll_device, bool* event_pending);

  // These methods are used to interrupt the thread sleeping on Poll() and force
  // it to return regardless of device state, which is usually when the client
  // is no longer interested in what happens with the device (on cleanup,
  // client state change, etc.). When SetDevicePollInterrupt() is called, Poll()
  // will return immediately, and any subsequent calls to it will also do so
  // until ClearDevicePollInterrupt() is called.
  bool SetDevicePollInterrupt();
  bool ClearDevicePollInterrupt();

  // Wrappers for standard mmap/munmap system calls.
  void* Mmap(void* addr,
             unsigned int len,
             int prot,
             int flags,
             unsigned int offset);
  void Munmap(void* addr, unsigned int len);

  // Return a vector of dmabuf file descriptors, exported for V4L2 buffer with
  // |index|, assuming the buffer contains |num_planes| V4L2 planes and is of
  // |type|. Return an empty vector on failure.
  // The caller is responsible for closing the file descriptors after use.
  std::vector<base::ScopedFD> GetDmabufsForV4L2Buffer(
      int index,
      size_t num_planes,
      enum v4l2_buf_type type);

  // NOTE: The below methods to query capabilities have a side effect of
  // closing the previously-open device, if any, and should not be called after
  // Open().
  // TODO(posciak): fix this.

  // Get minimum and maximum resolution for fourcc |pixelformat| and store to
  // |min_resolution| and |max_resolution|.
  void GetSupportedResolution(uint32_t pixelformat,
                              Size* min_resolution,
                              Size* max_resolution);

  // Return supported profiles for decoder, including only profiles for given
  // fourcc |pixelformats|.
  VideoDecodeAccelerator::SupportedProfiles GetSupportedDecodeProfiles(
      const size_t num_formats,
      const uint32_t pixelformats[]);

 private:
  friend class base::RefCountedThreadSafe<V4L2Device>;

  // Vector of video device node paths and corresponding pixelformats supported
  // by each device node.
  using Devices = std::vector<std::pair<std::string, std::vector<uint32_t>>>;

  VideoDecodeAccelerator::SupportedProfiles EnumerateSupportedDecodeProfiles(
      const size_t num_formats,
      const uint32_t pixelformats[]);

  std::vector<uint32_t> EnumerateSupportedPixelformats(v4l2_buf_type buf_type);

  // Open device node for |path| as a device of |type|.
  bool OpenDevicePath(const std::string& path, Type type);

  // Close the currently open device.
  void CloseDevice();

  // Enumerate all V4L2 devices on the system for |type| and store the results
  // under devices_by_type_[type].
  void EnumerateDevicesForType(Type type);

  // Return device information for all devices of |type| available in the
  // system. Enumerates and queries devices on first run and caches the results
  // for subsequent calls.
  const Devices& GetDevicesForType(Type type);

  // Return device node path for device of |type| supporting |pixfmt|, or
  // an empty string if the given combination is not supported by the system.
  std::string GetDevicePathFor(Type type, uint32_t pixfmt);

  // Stores information for all devices available on the system
  // for each device Type.
  std::map<Type, Devices> devices_by_type_;

  // The actual device fd.
  base::ScopedFD device_fd_;

  // eventfd fd to signal device poll thread when its poll() should be
  // interrupted.
  base::ScopedFD device_poll_interrupt_fd_;

  DISALLOW_COPY_AND_ASSIGN(V4L2Device);
};

}  //  namespace media

#endif  // V4L2_DEVICE_H_
