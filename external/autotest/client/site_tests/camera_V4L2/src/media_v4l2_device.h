// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_V4L2_DEVICE_H_
#define MEDIA_V4L2_DEVICE_H_

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <vector>

class V4L2Device {
 public:
  enum IOMethod {
    IO_METHOD_UNDEFINED,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
  };

  enum ConstantFramerate {
    DEFAULT_FRAMERATE_SETTING,
    ENABLE_CONSTANT_FRAMERATE,
    DISABLE_CONSTANT_FRAMERATE,
  };

  struct Buffer {
    void* start;
    size_t length;
  };

  V4L2Device(const char* dev_name,
             uint32_t buffers);
  virtual ~V4L2Device();

  virtual bool OpenDevice();
  virtual void CloseDevice();
  // After this function is called, the driver may adjust the settings if they
  // are unsupported.  Caller can use GetV4L2Format() and GetFrameRate() to know
  // the actual settings.  If V4L2_CAP_TIMEPERFRAME is unsupported, fps will be
  // ignored.
  virtual bool InitDevice(IOMethod io,
                          uint32_t width,
                          uint32_t height,
                          uint32_t pixfmt,
                          float fps,
                          ConstantFramerate constant_framerate,
                          uint32_t num_skip_frames);
  virtual bool UninitDevice();
  virtual bool StartCapture();
  virtual bool StopCapture();
  virtual bool Run(uint32_t time_in_sec);
  virtual int32_t ReadOneFrame(uint32_t* buffer_index, uint32_t* data_size);
  virtual bool EnqueueBuffer(uint32_t buffer_index);

  // Helper methods.
  bool EnumInput();
  bool EnumStandard();
  bool EnumControl(bool show_menu = true);
  bool EnumControlMenu(const v4l2_queryctrl& query_ctrl);
  bool EnumFormat(uint32_t* num_formats, bool show_fmt = true);
  bool EnumFrameSize(
      uint32_t pixfmt, uint32_t* num_sizes, bool show_frmsize = true);
  bool EnumFrameInterval(uint32_t pixfmt, uint32_t width, uint32_t height,
                         uint32_t* num_intervals, bool show_intervals = true);

  bool QueryControl(uint32_t id, v4l2_queryctrl* ctrl);
  bool SetControl(uint32_t id, int32_t value);
  bool ProbeCaps(v4l2_capability* cap, bool show_caps = false);
  bool GetCropCap(v4l2_cropcap* cropcap);
  bool GetCrop(v4l2_crop* crop);
  bool SetCrop(v4l2_crop* crop);
  bool GetParam(v4l2_streamparm* param);
  bool SetParam(v4l2_streamparm* param);
  bool SetFrameRate(float fps);
  bool GetPixelFormat(uint32_t index, uint32_t* pixfmt);
  bool GetFrameSize(
      uint32_t index, uint32_t pixfmt, uint32_t *width, uint32_t *height);
  bool GetFrameInterval(
      uint32_t index, uint32_t pixfmt, uint32_t width, uint32_t height,
      float* frame_rate);
  float GetFrameRate();
  bool GetV4L2Format(v4l2_format* format);
  bool Stop();

  // Getter.
  uint32_t GetNumFrames() const { return frame_timestamps_.size(); }

  const std::vector<int64_t>& GetFrameTimestamps() const {
    return frame_timestamps_;
  }

  const Buffer& GetBufferInfo(uint32_t index) {
    return v4l2_buffers_[index];
  }

  static uint32_t MapFourCC(const char* fourcc);

  virtual void ProcessImage(const void* p);

 private:
  int32_t DoIoctl(int32_t request, void* arg);
  bool InitMmapIO();
  bool InitUserPtrIO(uint32_t buffer_size);
  bool AllocateBuffer(uint32_t buffer_count);
  bool FreeBuffer();
  uint64_t Now();

  const char* dev_name_;
  IOMethod io_;
  int32_t fd_;
  Buffer* v4l2_buffers_;
  uint32_t num_buffers_;  // Actual buffers allocation.
  uint32_t min_buffers_;  // Minimum buffers requirement.
  bool stopped_;

  // Sets to true when buffers are initialized.
  bool initialized_;
  std::vector<int64_t> frame_timestamps_;
  // The number of frames should be skipped after stream on.
  uint32_t num_skip_frames_;
};

#endif  // MEDIA_V4L2_DEVICE_H_
