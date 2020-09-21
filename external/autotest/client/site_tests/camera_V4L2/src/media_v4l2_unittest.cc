// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>

#include <string>

#include "camera_characteristics.h"
#include "common_types.h"
#include "media_v4l2_device.h"

/* Test lists */
static const char kDefaultTestList[] = "default";
static const char kHalv3TestList[] = "halv3";
static const char kExternalCameraTestList[] = "external-camera";

bool ExerciseControl(V4L2Device* v4l2_dev, uint32_t id, const char* control) {
  v4l2_queryctrl query_ctrl;
  if (v4l2_dev->QueryControl(id, &query_ctrl)) {
    if (!v4l2_dev->SetControl(id, query_ctrl.maximum))
      printf("[Warning] Can not set %s to maximum value\n", control);
    if (!v4l2_dev->SetControl(id, query_ctrl.minimum))
      printf("[Warning] Can not set %s to minimum value\n", control);
    if (!v4l2_dev->SetControl(id, query_ctrl.default_value))
      printf("[Warning] Can not set %s to default value\n", control);
  } else {
    printf("[Warning] Can not query control name :%s\n", control);
    return false;
  }
  return true;
}

void TestMultipleOpen(const char* dev_name) {
  V4L2Device v4l2_dev1(dev_name, 4);
  V4L2Device v4l2_dev2(dev_name, 4);
  if (!v4l2_dev1.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the first time\n", dev_name);
    exit(EXIT_FAILURE);
  }
  if (!v4l2_dev2.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the second time\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_dev1.CloseDevice();
  v4l2_dev2.CloseDevice();
  printf("[OK ] V4L2DeviceTest.MultipleOpen\n");
}

void TestMultipleInit(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device::ConstantFramerate constant_framerate =
      V4L2Device::DEFAULT_FRAMERATE_SETTING;
  V4L2Device v4l2_dev1(dev_name, 4);
  V4L2Device v4l2_dev2(dev_name, 4);
  if (!v4l2_dev1.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the first time\n", dev_name);
    exit(EXIT_FAILURE);
  }
  if (!v4l2_dev2.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the second time\n", dev_name);
    exit(EXIT_FAILURE);
  }

  if (!v4l2_dev1.InitDevice(io, 640, 480, V4L2_PIX_FMT_YUYV, 30,
                            constant_framerate, 0)) {
    printf("[Error] Can not init device '%s' for the first time\n", dev_name);
    exit(EXIT_FAILURE);
  }

  // multiple streaming request should fail.
  if (v4l2_dev2.InitDevice(io, 640, 480, V4L2_PIX_FMT_YUYV, 30,
                           constant_framerate, 0)) {
    printf("[Error] Multiple init device '%s' should fail\n", dev_name);
    exit(EXIT_FAILURE);
  }

  v4l2_dev1.UninitDevice();
  v4l2_dev2.UninitDevice();
  v4l2_dev1.CloseDevice();
  v4l2_dev2.CloseDevice();
  printf("[OK ] V4L2DeviceTest.MultipleInit\n");
}

// EnumInput and EnumStandard are optional.
void TestEnumInputAndStandard(const char* dev_name) {
  V4L2Device v4l2_dev1(dev_name, 4);
  if (!v4l2_dev1.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_dev1.EnumInput();
  v4l2_dev1.EnumStandard();
  v4l2_dev1.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumInputAndStandard\n");
}

void TestEnumControl(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_dev.EnumControl();
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumControl\n");
}

void TestSetControl(const char* dev_name, bool constant_framerate_supported) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  // Test mandatory controls.
  if (constant_framerate_supported &&
      !ExerciseControl(&v4l2_dev, V4L2_CID_EXPOSURE_AUTO_PRIORITY,
                       "exposure_auto_priority")) {
    exit(EXIT_FAILURE);
  }

  // Test optional controls.
  ExerciseControl(&v4l2_dev, V4L2_CID_BRIGHTNESS, "brightness");
  ExerciseControl(&v4l2_dev, V4L2_CID_CONTRAST, "contrast");
  ExerciseControl(&v4l2_dev, V4L2_CID_SATURATION, "saturation");
  ExerciseControl(&v4l2_dev, V4L2_CID_GAMMA, "gamma");
  ExerciseControl(&v4l2_dev, V4L2_CID_HUE, "hue");
  ExerciseControl(&v4l2_dev, V4L2_CID_GAIN, "gain");
  ExerciseControl(&v4l2_dev, V4L2_CID_SHARPNESS, "sharpness");
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.SetControl\n");
}

// SetCrop is optional.
void TestSetCrop(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_cropcap cropcap;
  memset(&cropcap, 0, sizeof(cropcap));
  if (v4l2_dev.GetCropCap(&cropcap)) {
    v4l2_crop crop;
    memset(&crop, 0, sizeof(crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;
    v4l2_dev.SetCrop(&crop);
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.SetCrop\n");
}

// GetCrop is optional.
void TestGetCrop(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_crop crop;
  memset(&crop, 0, sizeof(crop));
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2_dev.GetCrop(&crop);
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.GetCrop\n");
}

void TestProbeCaps(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_capability caps;
  if (!v4l2_dev.ProbeCaps(&caps, true)) {
    printf("[Error] Can not probe caps on device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    printf("<<< Info: %s does not support video capture interface.>>>\n",
        dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.ProbeCaps\n");
}

void TestEnumFormats(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  if (!v4l2_dev.EnumFormat(NULL)) {
    printf("[Error] Can not enumerate format on device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumFormats\n");
}

void TestEnumFrameSize(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  uint32_t format_count = 0;
  if (!v4l2_dev.EnumFormat(&format_count)) {
    printf("[Error] Can not enumerate format on device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  for (uint32_t i = 0; i < format_count; ++i) {
    uint32_t pixfmt;
    if (!v4l2_dev.GetPixelFormat(i, &pixfmt)) {
      printf("[Error] Enumerate format error on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    if (!v4l2_dev.EnumFrameSize(pixfmt, NULL)) {
      printf("[Warning] Enumerate frame size error on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    };
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumFrameSize\n");
}

void TestEnumFrameInterval(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  uint32_t format_count = 0;
  if (!v4l2_dev.EnumFormat(&format_count)) {
    printf("[Error] Can not enumerate format on device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  for (uint32_t i = 0; i < format_count; ++i) {
    uint32_t pixfmt;
    if (!v4l2_dev.GetPixelFormat(i, &pixfmt)) {
      printf("[Error] Get format error on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    uint32_t size_count;
    if (!v4l2_dev.EnumFrameSize(pixfmt, &size_count)) {
      printf("[Error] Enumerate frame size error on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    };

    for (uint32_t j = 0; j < size_count; ++j) {
      uint32_t width, height;
      if (!v4l2_dev.GetFrameSize(j, pixfmt, &width, &height)) {
        printf("[Error] Get frame size error on device '%s'\n", dev_name);
        exit(EXIT_FAILURE);
      };
      if (!v4l2_dev.EnumFrameInterval(pixfmt, width, height, NULL)) {
        printf("[Error] Enumerate frame interval error on device '%s'\n",
            dev_name);
        exit(EXIT_FAILURE);
      };
    }
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumFrameInterval\n");
}

void TestFrameRate(const char* dev_name) {
  V4L2Device v4l2_dev(dev_name, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_streamparm param;
  if (!v4l2_dev.GetParam(&param)) {
    printf("[Error] Can not get stream param on device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  // we only try to adjust frame rate when it claims can.
  if (param.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
    if (!v4l2_dev.SetParam(&param)) {
      printf("[Error] Can not set stream param on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
  } else {
    printf("[Info] %s does not support TIMEPERFRAME.\n", dev_name);
  }

  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.FrameRate\n");
}

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
static const struct option long_options[] = {
        { "help",      no_argument,       NULL, '?' },
        { "device",    required_argument, NULL, 'd' },
        { "usb-info",  required_argument, NULL, 'u' },
        { "test-list", required_argument, NULL, 't' },
        { 0, 0, 0, 0 }
};

int main(int argc, char** argv) {
  std::string dev_name = "/dev/video";
  std::string usb_info = "";
  std::string test_list = kDefaultTestList;

  // Parse the command line.
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

  std::unordered_map<std::string, std::string> mapping = {{usb_info, dev_name}};
  CameraCharacteristics characteristics;
  DeviceInfos device_infos =
      characteristics.GetCharacteristicsFromFile(mapping);
  if (device_infos.size() > 1) {
    printf("[Error] One device should not have multiple configs.\n");
    return EXIT_FAILURE;
  }

  bool constant_framerate = false;
  if (test_list == kDefaultTestList) {
    if (device_infos.size() == 1) {
      constant_framerate = !device_infos[0].constant_framerate_unsupported;
    }
  } else {
    constant_framerate = true;
  }
  printf("[Info] constant framerate: %d\n", constant_framerate);

  TestMultipleOpen(dev_name.c_str());
  TestMultipleInit(dev_name.c_str(), V4L2Device::IO_METHOD_MMAP);
  TestMultipleInit(dev_name.c_str(), V4L2Device::IO_METHOD_USERPTR);
  TestEnumInputAndStandard(dev_name.c_str());
  TestEnumControl(dev_name.c_str());
  TestSetControl(dev_name.c_str(), constant_framerate);
  TestSetCrop(dev_name.c_str());
  TestGetCrop(dev_name.c_str());
  TestProbeCaps(dev_name.c_str());
  TestEnumFormats(dev_name.c_str());
  TestEnumFrameSize(dev_name.c_str());
  TestEnumFrameInterval(dev_name.c_str());
  TestFrameRate(dev_name.c_str());
  return EXIT_SUCCESS;
}
