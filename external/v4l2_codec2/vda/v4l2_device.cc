// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Note: ported from Chromium commit head: 09ea0d2
// Note: it's also merged with generic_v4l2_device.cc (head: a9d98e6)

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "base/numerics/safe_conversions.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "v4l2_device.h"

#define DVLOGF(level) DVLOG(level) << __func__ << "(): "
#define VLOGF(level) VLOG(level) << __func__ << "(): "
#define VPLOGF(level) VPLOG(level) << __func__ << "(): "

namespace media {

V4L2Device::V4L2Device() {}

V4L2Device::~V4L2Device() {
  CloseDevice();
}

// static
VideoPixelFormat V4L2Device::V4L2PixFmtToVideoPixelFormat(uint32_t pix_fmt) {
  switch (pix_fmt) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12M:
      return PIXEL_FORMAT_NV12;

    case V4L2_PIX_FMT_MT21:
      return PIXEL_FORMAT_MT21;

    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YUV420M:
      return PIXEL_FORMAT_I420;

    case V4L2_PIX_FMT_YVU420:
      return PIXEL_FORMAT_YV12;

    case V4L2_PIX_FMT_YUV422M:
      return PIXEL_FORMAT_I422;

    case V4L2_PIX_FMT_RGB32:
      return PIXEL_FORMAT_ARGB;

    default:
      DVLOGF(1) << "Add more cases as needed";
      return PIXEL_FORMAT_UNKNOWN;
  }
}

// static
uint32_t V4L2Device::VideoPixelFormatToV4L2PixFmt(VideoPixelFormat format) {
  switch (format) {
    case PIXEL_FORMAT_NV12:
      return V4L2_PIX_FMT_NV12M;

    case PIXEL_FORMAT_MT21:
      return V4L2_PIX_FMT_MT21;

    case PIXEL_FORMAT_I420:
      return V4L2_PIX_FMT_YUV420M;

    case PIXEL_FORMAT_YV12:
      return V4L2_PIX_FMT_YVU420;

    default:
      LOG(FATAL) << "Add more cases as needed";
      return 0;
  }
}

// static
uint32_t V4L2Device::VideoCodecProfileToV4L2PixFmt(VideoCodecProfile profile,
                                                   bool slice_based) {
  if (profile >= H264PROFILE_MIN && profile <= H264PROFILE_MAX) {
    if (slice_based)
      return V4L2_PIX_FMT_H264_SLICE;
    else
      return V4L2_PIX_FMT_H264;
  } else if (profile >= VP8PROFILE_MIN && profile <= VP8PROFILE_MAX) {
    if (slice_based)
      return V4L2_PIX_FMT_VP8_FRAME;
    else
      return V4L2_PIX_FMT_VP8;
  } else if (profile >= VP9PROFILE_MIN && profile <= VP9PROFILE_MAX) {
    if (slice_based)
      return V4L2_PIX_FMT_VP9_FRAME;
    else
      return V4L2_PIX_FMT_VP9;
  } else {
    LOG(FATAL) << "Add more cases as needed";
    return 0;
  }
}

// static
std::vector<VideoCodecProfile> V4L2Device::V4L2PixFmtToVideoCodecProfiles(
    uint32_t pix_fmt,
    bool is_encoder) {
  VideoCodecProfile min_profile, max_profile;
  std::vector<VideoCodecProfile> profiles;

  switch (pix_fmt) {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H264_SLICE:
      if (is_encoder) {
        // TODO(posciak): need to query the device for supported H.264 profiles,
        // for now choose Main as a sensible default.
        min_profile = H264PROFILE_MAIN;
        max_profile = H264PROFILE_MAIN;
      } else {
        min_profile = H264PROFILE_MIN;
        max_profile = H264PROFILE_MAX;
      }
      break;

    case V4L2_PIX_FMT_VP8:
    case V4L2_PIX_FMT_VP8_FRAME:
      min_profile = VP8PROFILE_MIN;
      max_profile = VP8PROFILE_MAX;
      break;

    case V4L2_PIX_FMT_VP9:
    case V4L2_PIX_FMT_VP9_FRAME:
      min_profile = VP9PROFILE_MIN;
      max_profile = VP9PROFILE_MAX;
      break;

    default:
      VLOGF(1) << "Unhandled pixelformat " << std::hex << "0x" << pix_fmt;
      return profiles;
  }

  for (int profile = min_profile; profile <= max_profile; ++profile)
    profiles.push_back(static_cast<VideoCodecProfile>(profile));

  return profiles;
}

int V4L2Device::Ioctl(int request, void* arg) {
  DCHECK(device_fd_.is_valid());
  return HANDLE_EINTR(ioctl(device_fd_.get(), request, arg));
}

bool V4L2Device::Poll(bool poll_device, bool* event_pending) {
  struct pollfd pollfds[2];
  nfds_t nfds;
  int pollfd = -1;

  pollfds[0].fd = device_poll_interrupt_fd_.get();
  pollfds[0].events = POLLIN | POLLERR;
  nfds = 1;

  if (poll_device) {
    DVLOGF(5) << "Poll(): adding device fd to poll() set";
    pollfds[nfds].fd = device_fd_.get();
    pollfds[nfds].events = POLLIN | POLLOUT | POLLERR | POLLPRI;
    pollfd = nfds;
    nfds++;
  }

  if (HANDLE_EINTR(poll(pollfds, nfds, -1)) == -1) {
    VPLOGF(1) << "poll() failed";
    return false;
  }
  *event_pending = (pollfd != -1 && pollfds[pollfd].revents & POLLPRI);
  return true;
}

void* V4L2Device::Mmap(void* addr,
                       unsigned int len,
                       int prot,
                       int flags,
                       unsigned int offset) {
  DCHECK(device_fd_.is_valid());
  return mmap(addr, len, prot, flags, device_fd_.get(), offset);
}

void V4L2Device::Munmap(void* addr, unsigned int len) {
  munmap(addr, len);
}

bool V4L2Device::SetDevicePollInterrupt() {
  DVLOGF(4);

  const uint64_t buf = 1;
  if (HANDLE_EINTR(write(device_poll_interrupt_fd_.get(), &buf, sizeof(buf))) ==
      -1) {
    VPLOGF(1) << "write() failed";
    return false;
  }
  return true;
}

bool V4L2Device::ClearDevicePollInterrupt() {
  DVLOGF(5);

  uint64_t buf;
  if (HANDLE_EINTR(read(device_poll_interrupt_fd_.get(), &buf, sizeof(buf))) ==
      -1) {
    if (errno == EAGAIN) {
      // No interrupt flag set, and we're reading nonblocking.  Not an error.
      return true;
    } else {
      VPLOGF(1) << "read() failed";
      return false;
    }
  }
  return true;
}

bool V4L2Device::Open(Type type, uint32_t v4l2_pixfmt) {
  VLOGF(2);
  std::string path = GetDevicePathFor(type, v4l2_pixfmt);

  if (path.empty()) {
    VLOGF(1) << "No devices supporting " << std::hex << "0x" << v4l2_pixfmt
             << " for type: " << static_cast<int>(type);
    return false;
  }

  if (!OpenDevicePath(path, type)) {
    VLOGF(1) << "Failed opening " << path;
    return false;
  }

  device_poll_interrupt_fd_.reset(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
  if (!device_poll_interrupt_fd_.is_valid()) {
    VLOGF(1) << "Failed creating a poll interrupt fd";
    return false;
  }

  return true;
}

std::vector<base::ScopedFD> V4L2Device::GetDmabufsForV4L2Buffer(
    int index,
    size_t num_planes,
    enum v4l2_buf_type buf_type) {
  VLOGF(2);
  DCHECK(V4L2_TYPE_IS_MULTIPLANAR(buf_type));

  std::vector<base::ScopedFD> dmabuf_fds;
  for (size_t i = 0; i < num_planes; ++i) {
    struct v4l2_exportbuffer expbuf;
    memset(&expbuf, 0, sizeof(expbuf));
    expbuf.type = buf_type;
    expbuf.index = index;
    expbuf.plane = i;
    expbuf.flags = O_CLOEXEC;
    if (Ioctl(VIDIOC_EXPBUF, &expbuf) != 0) {
      dmabuf_fds.clear();
      break;
    }

    dmabuf_fds.push_back(base::ScopedFD(expbuf.fd));
  }

  return dmabuf_fds;
}

VideoDecodeAccelerator::SupportedProfiles
V4L2Device::GetSupportedDecodeProfiles(const size_t num_formats,
                                       const uint32_t pixelformats[]) {
  VideoDecodeAccelerator::SupportedProfiles supported_profiles;

  Type type = Type::kDecoder;
  const auto& devices = GetDevicesForType(type);
  for (const auto& device : devices) {
    if (!OpenDevicePath(device.first, type)) {
      VLOGF(1) << "Failed opening " << device.first;
      continue;
    }

    const auto& profiles =
        EnumerateSupportedDecodeProfiles(num_formats, pixelformats);
    supported_profiles.insert(supported_profiles.end(), profiles.begin(),
                              profiles.end());
    CloseDevice();
  }

  return supported_profiles;
}

void V4L2Device::GetSupportedResolution(uint32_t pixelformat,
                                        Size* min_resolution,
                                        Size* max_resolution) {
  max_resolution->SetSize(0, 0);
  min_resolution->SetSize(0, 0);
  v4l2_frmsizeenum frame_size;
  memset(&frame_size, 0, sizeof(frame_size));
  frame_size.pixel_format = pixelformat;
  for (; Ioctl(VIDIOC_ENUM_FRAMESIZES, &frame_size) == 0; ++frame_size.index) {
    if (frame_size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
      if (frame_size.discrete.width >=
              base::checked_cast<uint32_t>(max_resolution->width()) &&
          frame_size.discrete.height >=
              base::checked_cast<uint32_t>(max_resolution->height())) {
        max_resolution->SetSize(frame_size.discrete.width,
                                frame_size.discrete.height);
      }
      if (min_resolution->IsEmpty() ||
          (frame_size.discrete.width <=
               base::checked_cast<uint32_t>(min_resolution->width()) &&
           frame_size.discrete.height <=
               base::checked_cast<uint32_t>(min_resolution->height()))) {
        min_resolution->SetSize(frame_size.discrete.width,
                                frame_size.discrete.height);
      }
    } else if (frame_size.type == V4L2_FRMSIZE_TYPE_STEPWISE ||
               frame_size.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
      max_resolution->SetSize(frame_size.stepwise.max_width,
                              frame_size.stepwise.max_height);
      min_resolution->SetSize(frame_size.stepwise.min_width,
                              frame_size.stepwise.min_height);
      break;
    }
  }
  if (max_resolution->IsEmpty()) {
    max_resolution->SetSize(1920, 1088);
    VLOGF(1) << "GetSupportedResolution failed to get maximum resolution for "
             << "fourcc " << std::hex << pixelformat
             << ", fall back to " << max_resolution->ToString();
  }
  if (min_resolution->IsEmpty()) {
    min_resolution->SetSize(16, 16);
    VLOGF(1) << "GetSupportedResolution failed to get minimum resolution for "
             << "fourcc " << std::hex << pixelformat
             << ", fall back to " << min_resolution->ToString();
  }
}

std::vector<uint32_t> V4L2Device::EnumerateSupportedPixelformats(
    v4l2_buf_type buf_type) {
  std::vector<uint32_t> pixelformats;

  v4l2_fmtdesc fmtdesc;
  memset(&fmtdesc, 0, sizeof(fmtdesc));
  fmtdesc.type = buf_type;

  for (; Ioctl(VIDIOC_ENUM_FMT, &fmtdesc) == 0; ++fmtdesc.index) {
    DVLOGF(3) << "Found " << fmtdesc.description << std::hex << " (0x"
              << fmtdesc.pixelformat << ")";
    pixelformats.push_back(fmtdesc.pixelformat);
  }

  return pixelformats;
}

VideoDecodeAccelerator::SupportedProfiles
V4L2Device::EnumerateSupportedDecodeProfiles(const size_t num_formats,
                                             const uint32_t pixelformats[]) {
  VideoDecodeAccelerator::SupportedProfiles profiles;

  const auto& supported_pixelformats =
      EnumerateSupportedPixelformats(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);

  for (uint32_t pixelformat : supported_pixelformats) {
    if (std::find(pixelformats, pixelformats + num_formats, pixelformat) ==
        pixelformats + num_formats)
      continue;

    VideoDecodeAccelerator::SupportedProfile profile;
    GetSupportedResolution(pixelformat, &profile.min_resolution,
                           &profile.max_resolution);

    const auto video_codec_profiles =
        V4L2PixFmtToVideoCodecProfiles(pixelformat, false);

    for (const auto& video_codec_profile : video_codec_profiles) {
      profile.profile = video_codec_profile;
      profiles.push_back(profile);

      DVLOGF(3) << "Found decoder profile " << GetProfileName(profile.profile)
                << ", resolutions: " << profile.min_resolution.ToString() << " "
                << profile.max_resolution.ToString();
    }
  }

  return profiles;
}

bool V4L2Device::OpenDevicePath(const std::string& path, Type type) {
  DCHECK(!device_fd_.is_valid());

  device_fd_.reset(
      HANDLE_EINTR(open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC)));
  if (!device_fd_.is_valid())
    return false;

  return true;
}

void V4L2Device::CloseDevice() {
  VLOGF(2);
  device_fd_.reset();
}

void V4L2Device::EnumerateDevicesForType(Type type) {
  static const std::string kDecoderDevicePattern = "/dev/video-dec";
  std::string device_pattern;
  v4l2_buf_type buf_type;
  switch (type) {
    case Type::kDecoder:
      device_pattern = kDecoderDevicePattern;
      buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
      break;
    default:
      LOG(ERROR) << "Only decoder type is supported!!";
      return;
  }

  std::vector<std::string> candidate_paths;

  // TODO(posciak): Remove this legacy unnumbered device once
  // all platforms are updated to use numbered devices.
  candidate_paths.push_back(device_pattern);

  // We are sandboxed, so we can't query directory contents to check which
  // devices are actually available. Try to open the first 10; if not present,
  // we will just fail to open immediately.
  for (int i = 0; i < 10; ++i) {
    candidate_paths.push_back(
        base::StringPrintf("%s%d", device_pattern.c_str(), i));
  }

  Devices devices;
  for (const auto& path : candidate_paths) {
    if (!OpenDevicePath(path, type))
      continue;

    const auto& supported_pixelformats =
        EnumerateSupportedPixelformats(buf_type);
    if (!supported_pixelformats.empty()) {
      DVLOGF(3) << "Found device: " << path;
      devices.push_back(std::make_pair(path, supported_pixelformats));
    }

    CloseDevice();
  }

  DCHECK_EQ(devices_by_type_.count(type), 0u);
  devices_by_type_[type] = devices;
}

const V4L2Device::Devices& V4L2Device::GetDevicesForType(Type type) {
  if (devices_by_type_.count(type) == 0)
    EnumerateDevicesForType(type);

  DCHECK_NE(devices_by_type_.count(type), 0u);
  return devices_by_type_[type];
}

std::string V4L2Device::GetDevicePathFor(Type type, uint32_t pixfmt) {
  const Devices& devices = GetDevicesForType(type);

  for (const auto& device : devices) {
    if (std::find(device.second.begin(), device.second.end(), pixfmt) !=
        device.second.end())
      return device.first;
  }

  return std::string();
}

}  //  namespace media
