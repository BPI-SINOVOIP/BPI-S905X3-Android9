// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>
#include <libyuv.h>
#include <math.h>

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "camera_characteristics.h"
#include "common_types.h"
#include "media_v4l2_device.h"

struct TestProfile {
  std::string dev_name;
  bool check_1280x960 = false;
  bool check_1600x1200 = false;
  bool support_constant_framerate = false;
  bool check_minimum_resolution = false;
  uint32_t skip_frames = 0;
  uint32_t lens_facing;
};

/* Test lists:
 * default: for devices without ARC++, and devices with ARC++ which use
 *          camera HAL v1.
 * halv3: for devices with ARC++ which use camera HAL v3.
 * external-camera: for third-party labs to verify new camera modules.
 */
static const char kDefaultTestList[] = "default";
static const char kHalv3TestList[] = "halv3";
static const char kExternalCameraTestList[] = "external-camera";

/* Camera Facing */
static const char kFrontCamera[] = "user";
static const char kBackCamera[] = "world";

static void PrintUsage(int argc, char** argv) {
  printf("Usage: %s [options]\n\n"
         "Options:\n"
         "--help               Print usage\n"
         "--device=DEVICE_NAME Video device name [/dev/video]\n"
         "--usb-info=VID:PID   Device vendor id and product id\n"
         "--test-list=TEST     Select different test list\n"
         "                     [%s | %s | %s]\n",
         argv[0], kDefaultTestList, kHalv3TestList,
         kExternalCameraTestList);
}

static const char short_options[] = "?d:u:t:";
static const struct option
long_options[] = {
        { "help",      no_argument,       NULL, '?' },
        { "device",    required_argument, NULL, 'd' },
        { "usb-info",  required_argument, NULL, 'u' },
        { "test-list", required_argument, NULL, 't' },
        { 0, 0, 0, 0 }
};

int RunTest(V4L2Device* device, V4L2Device::IOMethod io,
            uint32_t buffers, uint32_t capture_time_in_sec, uint32_t width,
            uint32_t height, uint32_t pixfmt, float fps,
            V4L2Device::ConstantFramerate constant_framerate,
            uint32_t skip_frames) {
  int32_t retcode = 0;
  if (!device->InitDevice(io, width, height, pixfmt, fps, constant_framerate,
                          skip_frames))
    retcode = 1;

  if (!retcode && !device->StartCapture())
    retcode = 2;

  if (!retcode && !device->Run(capture_time_in_sec))
    retcode = 3;

  if (!retcode && !device->StopCapture())
    retcode = 4;

  if (!retcode && !device->UninitDevice())
    retcode = 5;

  return retcode;
}

bool GetSupportedFormats(
    V4L2Device* device, SupportedFormats* supported_formats) {
  supported_formats->clear();

  SupportedFormat format;
  uint32_t num_format = 0;
  device->EnumFormat(&num_format, false);
  for (uint32_t i = 0; i < num_format; ++i) {
    if (!device->GetPixelFormat(i, &format.fourcc)) {
      printf("[Error] Get format error\n");
      return false;
    }
    uint32_t num_frame_size;
    if (!device->EnumFrameSize(format.fourcc, &num_frame_size, false)) {
      printf("[Error] Enumerate frame size error\n");
      return false;
    };

    for (uint32_t j = 0; j < num_frame_size; ++j) {
      if (!device->GetFrameSize(j, format.fourcc, &format.width,
                                &format.height)) {
        printf("[Error] Get frame size error\n");
        return false;
      };
      uint32_t num_frame_rate;
      if (!device->EnumFrameInterval(format.fourcc, format.width,
                                     format.height, &num_frame_rate, false)) {
        printf("[Error] Enumerate frame interval error\n");
        return false;
      };

      format.frame_rates.clear();
      float frame_rate;
      for (uint32_t k = 0; k < num_frame_rate; ++k) {
        if (!device->GetFrameInterval(k, format.fourcc, format.width,
                                      format.height, &frame_rate)) {
          printf("[Error] Get frame interval error\n");
          return false;
        };
        // All supported resolution should have at least 1 fps.
        if (frame_rate < 1.0) {
          printf("[Error] Frame rate should be at least 1.\n");
          return false;
        }
        format.frame_rates.push_back(frame_rate);
      }
      supported_formats->push_back(format);
    }
  }
  return true;
}

SupportedFormat GetMaximumResolution(const SupportedFormats& formats) {
  SupportedFormat max_format;
  memset(&max_format, 0, sizeof(max_format));
  for (const auto& format : formats) {
    if (format.width >= max_format.width &&
        format.height >= max_format.height) {
      max_format = format;
    }
  }
  return max_format;
}

// Find format according to width and height. If multiple formats support the
// same resolution, choose V4L2_PIX_FMT_MJPEG first.
const SupportedFormat* FindFormatByResolution(const SupportedFormats& formats,
                                              uint32_t width,
                                              uint32_t height) {
  const SupportedFormat* result_format = nullptr;
  for (const auto& format : formats) {
    if (format.width == width && format.height == height) {
      if (!result_format || format.fourcc == V4L2_PIX_FMT_MJPEG) {
        result_format = &format;
      }
    }
  }
  return result_format;
}

// Find format according to V4L2 fourcc. If multiple resolution support the
// same fourcc, choose the first one.
const SupportedFormat* FindFormatByFourcc(const SupportedFormats& formats,
                                          uint32_t fourcc) {
  for (const auto& format : formats) {
    if (format.fourcc == V4L2_PIX_FMT_MJPEG) {
      return &format;
    }
  }
  return nullptr;
}

// This is for Android testCameraToSurfaceTextureMetadata CTS test case.
bool CheckConstantFramerate(const std::vector<int64_t>& timestamps,
                            uint32_t capture_time_in_sec,
                            float require_fps) {
  // Timestamps are from driver. We only allow 1.5% error buffer for the frame
  // duration. The margin is aligned to CTS tests.
  float slop_margin = 0.015;
  float slop_max_frame_duration_ms = (1000.f / require_fps) * (1 + slop_margin);
  float slop_min_frame_duration_ms = (1000.f / require_fps) * (1 - slop_margin);

  for (size_t i = 1; i < timestamps.size(); i++) {
    float frame_duration_ms =
        (timestamps[i] - timestamps[i-1]) / 1000000.f;
    if (frame_duration_ms > slop_max_frame_duration_ms ||
        frame_duration_ms < slop_min_frame_duration_ms) {
      printf("[Error] Frame duration %f out of frame rate bounds [%f,%f]\n",
          frame_duration_ms, slop_min_frame_duration_ms,
          slop_max_frame_duration_ms);
      return false;
    }
  }
  return true;
}

bool TestIO(const std::string& dev_name) {
  printf("[Info] TestIO\n");
  uint32_t buffers = 4;
  uint32_t width = 640;
  uint32_t height = 480;
  uint32_t pixfmt = V4L2_PIX_FMT_YUYV;
  float fps = 30.0;
  uint32_t time_to_capture = 3;  // The unit is second.
  uint32_t skip_frames = 0;
  bool check_1280x960 = false;
  V4L2Device::ConstantFramerate constant_framerate =
      V4L2Device::DEFAULT_FRAMERATE_SETTING;

  std::unique_ptr<V4L2Device> device(
      new V4L2Device(dev_name.c_str(), buffers));

  if (!device->OpenDevice())
    return false;

  v4l2_capability cap;
  if (!device->ProbeCaps(&cap))
    return false;

  if (cap.capabilities & V4L2_CAP_STREAMING) {
    int mmap_ret = RunTest(device.get(), V4L2Device::IO_METHOD_MMAP, buffers,
        time_to_capture, width, height, pixfmt, fps, constant_framerate,
        skip_frames);
    int userp_ret = RunTest(device.get(), V4L2Device::IO_METHOD_USERPTR,
        buffers, time_to_capture, width, height, pixfmt, fps,
        constant_framerate, skip_frames);
    if (mmap_ret && userp_ret) {
      printf("[Error] Stream I/O failed.\n");
      return false;
    }
  } else {
    printf("[Error] Streaming capability is mandatory.\n");
    return false;
  }

  device->CloseDevice();
  printf("[Info] TestIO pass\n");
  return true;
}

// Test all required resolutions with 30 fps.
// If device supports constant framerate, the test will toggle the setting
// and check actual fps. Otherwise, use the default setting of
// V4L2_CID_EXPOSURE_AUTO_PRIORITY.
bool TestResolutions(const std::string& dev_name,
                     bool check_1280x960,
                     bool check_1600x1200,
                     bool test_constant_framerate) {
  printf("[Info] TestResolutions\n");
  uint32_t buffers = 4;
  uint32_t time_to_capture = 3;
  uint32_t skip_frames = 0;
  V4L2Device::IOMethod io = V4L2Device::IO_METHOD_MMAP;
  std::unique_ptr<V4L2Device> device(
      new V4L2Device(dev_name.c_str(), buffers));

  if (!device->OpenDevice())
    return false;

  std::vector<V4L2Device::ConstantFramerate> constant_framerate_setting;
  if (test_constant_framerate) {
    constant_framerate_setting.push_back(V4L2Device::ENABLE_CONSTANT_FRAMERATE);
    constant_framerate_setting.push_back(
        V4L2Device::DISABLE_CONSTANT_FRAMERATE);
  } else {
    constant_framerate_setting.push_back(
        V4L2Device::DEFAULT_FRAMERATE_SETTING);
  }

  SupportedFormats supported_formats;
  if (!GetSupportedFormats(device.get(), &supported_formats)) {
    printf("[Error] Get supported formats failed in %s.\n", dev_name.c_str());
    return false;
  }
  SupportedFormat max_resolution = GetMaximumResolution(supported_formats);

  const float kFrameRate = 30.0;
  SupportedFormats required_resolutions;
  required_resolutions.push_back(SupportedFormat(320, 240, 0, kFrameRate));
  required_resolutions.push_back(SupportedFormat(640, 480, 0, kFrameRate));
  required_resolutions.push_back(SupportedFormat(1280, 720, 0, kFrameRate));
  required_resolutions.push_back(SupportedFormat(1920, 1080, 0, kFrameRate));
  if (check_1600x1200) {
    required_resolutions.push_back(SupportedFormat(1600, 1200, 0, kFrameRate));
  }
  if (check_1280x960) {
    required_resolutions.push_back(SupportedFormat(1280, 960, 0, kFrameRate));
  }

  v4l2_streamparm param;
  if (!device->GetParam(&param)) {
    printf("[Error] Can not get stream param on device '%s'\n",
        dev_name.c_str());
    return false;
  }

  for (const auto& test_resolution : required_resolutions) {
    // Skip the resolution that is larger than the maximum.
    if (max_resolution.width < test_resolution.width ||
        max_resolution.height < test_resolution.height) {
      continue;
    }

    const SupportedFormat* test_format = FindFormatByResolution(
        supported_formats, test_resolution.width, test_resolution.height);
    if (test_format == nullptr) {
      printf("[Error] %dx%d not found in %s\n", test_resolution.width,
          test_resolution.height, dev_name.c_str());
      return false;
    }

    bool frame_rate_30_supported = false;
    for (const auto& frame_rate : test_format->frame_rates) {
      if (std::fabs(frame_rate - kFrameRate) <=
          std::numeric_limits<float>::epsilon()) {
        frame_rate_30_supported = true;
        break;
      }
    }
    if (!frame_rate_30_supported) {
      printf("[Error] Cannot test 30 fps for %dx%d (%08X) failed in %s\n",
          test_format->width, test_format->height, test_format->fourcc,
          dev_name.c_str());
      return false;
    }

    for (const auto& constant_framerate : constant_framerate_setting) {
      if (RunTest(device.get(), io, buffers, time_to_capture,
            test_format->width, test_format->height, test_format->fourcc,
            kFrameRate, constant_framerate, skip_frames)) {
        printf("[Error] Could not capture frames for %dx%d (%08X) in %s\n",
            test_format->width, test_format->height, test_format->fourcc,
            dev_name.c_str());
        return false;
      }

      // Make sure the driver didn't adjust the format.
      v4l2_format fmt;
      if (!device->GetV4L2Format(&fmt)) {
        return false;
      }
      if (test_format->width != fmt.fmt.pix.width ||
          test_format->height != fmt.fmt.pix.height ||
          test_format->fourcc != fmt.fmt.pix.pixelformat ||
          std::fabs(kFrameRate - device->GetFrameRate()) >
              std::numeric_limits<float>::epsilon()) {
        printf("[Error] Capture test %dx%d (%08X) %.2f fps failed in %s\n",
            test_format->width, test_format->height, test_format->fourcc,
            kFrameRate, dev_name.c_str());
        return false;
      }

      if (constant_framerate != V4L2Device::ENABLE_CONSTANT_FRAMERATE) {
        continue;
      }

      float actual_fps = device->GetNumFrames() /
          static_cast<float>(time_to_capture);
      // 1 fps buffer is because |time_to_capture| may be too short.
      // EX: 30 fps and capture 3 secs. We may get 89 frames or 91 frames.
      // The actual fps will be 29.66 or 30.33.
      if (fabsf(actual_fps - kFrameRate) > 1) {
        printf("[Error] Capture test %dx%d (%08X) failed with fps %.2f in "
               "%s\n", test_format->width, test_format->height,
               test_format->fourcc, actual_fps, dev_name.c_str());
        return false;
      }

      if (!CheckConstantFramerate(device->GetFrameTimestamps(),
                                  time_to_capture, kFrameRate)) {
        printf("[Error] Capture test %dx%d (%08X) failed and didn't meet "
               "constant framerate in %s\n", test_format->width,
               test_format->height, test_format->fourcc, dev_name.c_str());
        return false;
      }
    }
  }
  device->CloseDevice();
  printf("[Info] TestResolutions pass\n");
  return true;
}

bool TestFirstFrameAfterStreamOn(const std::string& dev_name,
                                 uint32_t skip_frames) {
  printf("[Info] TestFirstFrameAfterStreamOn\n");
  uint32_t buffers = 4;
  uint32_t pixfmt = V4L2_PIX_FMT_MJPEG;
  uint32_t fps = 30;
  V4L2Device::ConstantFramerate constant_framerate =
      V4L2Device::DEFAULT_FRAMERATE_SETTING;
  V4L2Device::IOMethod io = V4L2Device::IO_METHOD_MMAP;

  std::unique_ptr<V4L2Device> device(
      new V4L2Device(dev_name.c_str(), buffers));
  if (!device->OpenDevice())
    return false;

  SupportedFormats supported_formats;
  if (!GetSupportedFormats(device.get(), &supported_formats)) {
    printf("[Error] Get supported formats failed in %s.\n", dev_name.c_str());
    return false;
  }
  const SupportedFormat* test_format = FindFormatByFourcc(
      supported_formats, V4L2_PIX_FMT_MJPEG);
  if (test_format == nullptr) {
    printf("[Info] The camera doesn't support MJPEG format.\n");
    return true;
  }

  uint32_t width = test_format->width;
  uint32_t height = test_format->height;

  const int kTestLoop = 20;
  for (size_t i = 0; i < kTestLoop; i++) {
    if (!device->InitDevice(io, width, height, pixfmt, fps, constant_framerate,
                            skip_frames))
      return false;

    if (!device->StartCapture())
      return false;

    uint32_t buf_index, data_size;
    if (!device->ReadOneFrame(&buf_index, &data_size))
      return false;

    const V4L2Device::Buffer& buffer = device->GetBufferInfo(buf_index);
    std::unique_ptr<uint8_t[]> yuv_buffer(new uint8_t[width * height * 2]);

    int res = libyuv::MJPGToI420(
        reinterpret_cast<uint8_t*>(buffer.start), data_size,
        yuv_buffer.get(), width,
        yuv_buffer.get() + width * height, width / 2,
        yuv_buffer.get() + width * height * 5 / 4, width / 2,
        width, height, width, height);
    if (res) {
      printf("[Error] First frame is not a valid mjpeg image.\n");
      return false;
    }

    if (!device->EnqueueBuffer(buf_index))
      return false;

    if (!device->StopCapture())
      return false;

    if (!device->UninitDevice())
      return false;

  }

  device->CloseDevice();
  printf("[Info] TestFirstFrameAfterStreamOn pass\n");
  return true;
}

// ChromeOS spec requires word-facing camera should be at least 1920x1080 and
// user-facing camera should be at least 1280x720.
bool TestMinimumResolution(const std::string& dev_name, uint32_t facing) {
  printf("[Info] TestMinimumResolution\n");
  uint32_t buffers = 4;
  std::unique_ptr<V4L2Device> device(
      new V4L2Device(dev_name.c_str(), buffers));

  if (!device->OpenDevice())
    return false;

  SupportedFormats supported_formats;
  if (!GetSupportedFormats(device.get(), &supported_formats)) {
    printf("[Error] Get supported formats failed in %s.\n", dev_name.c_str());
    return false;
  }
  device->CloseDevice();
  SupportedFormat max_resolution = GetMaximumResolution(supported_formats);

  uint32_t required_minimum_width = 0, required_minimum_height = 0;
  std::string facing_str = "";
  if (facing == FACING_FRONT) {
    required_minimum_width = 1080;
    required_minimum_height = 720;
    facing_str = kFrontCamera;
  } else if (facing == FACING_BACK) {
    required_minimum_width = 1920;
    required_minimum_height = 1080;
    facing_str = kBackCamera;
  } else {
    printf("[Error] Undefined facing: %d\n", facing);
    return false;
  }

  if (max_resolution.width < required_minimum_width ||
      max_resolution.height < required_minimum_height) {
    printf("[Error] The maximum resolution %dx%d does not meet minimum "
           "requirement %dx%d for %s-facing\n", max_resolution.width,
           max_resolution.height, required_minimum_width,
           required_minimum_height, facing_str.c_str());
    return false;
  }
  printf("[Info] TestMinimumResolution pass\n");
  return true;
}

const TestProfile GetTestProfile(const std::string& dev_name,
                                 const std::string& usb_info,
                                 const std::string& test_list) {
  std::unordered_map<std::string, std::string> mapping = {{usb_info, dev_name}};
  CameraCharacteristics characteristics;
  DeviceInfos device_infos =
      characteristics.GetCharacteristicsFromFile(mapping);
  if (device_infos.size() > 1) {
    printf("[Error] One device should not have multiple configs.\n");
    exit(EXIT_FAILURE);
  }

  TestProfile profile;
  profile.dev_name = dev_name;
  // Get parameter from config file.
  if (device_infos.size() == 1) {
    profile.check_1280x960 = !device_infos[0].resolution_1280x960_unsupported;
    profile.check_1600x1200 = !device_infos[0].resolution_1600x1200_unsupported;
    profile.support_constant_framerate =
        !device_infos[0].constant_framerate_unsupported;
    profile.skip_frames = device_infos[0].frames_to_skip_after_streamon;
    profile.check_minimum_resolution = true;
    profile.lens_facing = device_infos[0].lens_facing;
  }

  bool check_constant_framerate = false;
  if (test_list == kHalv3TestList) {
    profile.skip_frames = 0;
    check_constant_framerate = true;
  }

  if (test_list == kExternalCameraTestList) {
    profile.check_1280x960 = true;
    profile.check_1600x1200 = true;
    profile.support_constant_framerate = true;
    profile.skip_frames = 0;
    profile.check_minimum_resolution = false;
    check_constant_framerate = true;
  }

  printf("[Info] check 1280x960: %d\n", profile.check_1280x960);
  printf("[Info] check 1600x1200: %d\n", profile.check_1600x1200);
  printf("[Info] check constant framerate: %d\n", check_constant_framerate);
  printf("[Info] num of skip frames after stream on: %d\n",
      profile.skip_frames);

  return profile;
}

bool RunDefaultTestList(const TestProfile& profile) {
  bool pass = true;
  if (!TestIO(profile.dev_name)) {
    pass = false;
  }
  if (!TestResolutions(profile.dev_name, profile.check_1280x960,
                       profile.check_1600x1200, false)) {
    pass = false;
  }
  if (profile.check_minimum_resolution &&
      !TestMinimumResolution(profile.dev_name, profile.lens_facing)) {
    pass = false;
  }
  return pass;
}

bool RunHalv3TestList(const TestProfile& profile) {
  bool pass = true;
  if (!TestIO(profile.dev_name)) {
    pass = false;
  }
  if (profile.support_constant_framerate) {
    if (!TestResolutions(profile.dev_name, profile.check_1280x960,
                         profile.check_1600x1200, true)) {
      pass = false;
    }
  } else {
    printf("[Error] Hal v3 should support constant framerate.\n");
    pass = false;
  }
  if (!TestFirstFrameAfterStreamOn(profile.dev_name, profile.skip_frames)) {
    pass = false;
  }
  if (profile.check_minimum_resolution &&
      !TestMinimumResolution(profile.dev_name, profile.lens_facing)) {
    pass = false;
  }
  return pass;
}

bool RunExternalCameraTestList(const TestProfile& profile) {
  bool pass = true;
  if (!TestIO(profile.dev_name)) {
    pass = false;
  }
  if (!TestFirstFrameAfterStreamOn(profile.dev_name, profile.skip_frames)) {
    pass = false;
  }
  if (!TestResolutions(profile.dev_name, profile.check_1280x960,
                       profile.check_1600x1200, true)) {
    pass = false;
  }
  return pass;
}

int main(int argc, char** argv) {
  std::string dev_name = "/dev/video";
  std::string usb_info = "";
  std::string test_list = kDefaultTestList;

  for (;;) {
    int32_t index;
    int32_t c = getopt_long(argc, argv, short_options, long_options, &index);
    if (-1 == c)
      break;
    switch (c) {
      case 0:  // getopt_long() flag.
        break;
      case '?':
        PrintUsage(argc, argv);
        return EXIT_SUCCESS;
      case 'd':
        // Initialize default v4l2 device name.
        dev_name = optarg;
        break;
      case 'u':
        usb_info = optarg;
        break;
      case 't':
        test_list = optarg;
        break;
      default:
        PrintUsage(argc, argv);
        return EXIT_FAILURE;
    }
  }

  bool ret = true;
  TestProfile profile = GetTestProfile(dev_name, usb_info, test_list);
  if (test_list == kDefaultTestList) {
    ret = RunDefaultTestList(profile);
  } else if (test_list == kHalv3TestList) {
    ret = RunHalv3TestList(profile);
  } else if (test_list == kExternalCameraTestList) {
    ret = RunExternalCameraTestList(profile);
  } else {
    printf("[Error] Unknown test list %s\n", test_list.c_str());
    return EXIT_FAILURE;
  }

  return ret == true ? EXIT_SUCCESS : EXIT_FAILURE;
}
