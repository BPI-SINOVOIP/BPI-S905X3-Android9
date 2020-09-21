/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if GOOGLE_CUDA

#include "tensorflow/core/common_runtime/gpu/gpu_device.h"

#include "tensorflow/core/common_runtime/gpu/gpu_init.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/platform/test.h"

namespace tensorflow {
namespace {
const char* kDeviceNamePrefix = "/job:localhost/replica:0/task:0";

static SessionOptions MakeSessionOptions(
    const string& visible_device_list = "",
    double per_process_gpu_memory_fraction = 0, int gpu_device_count = 1,
    const std::vector<std::vector<float>>& memory_limit_mb = {}) {
  SessionOptions options;
  ConfigProto* config = &options.config;
  (*config->mutable_device_count())["GPU"] = gpu_device_count;
  GPUOptions* gpu_options = config->mutable_gpu_options();
  gpu_options->set_visible_device_list(visible_device_list);
  gpu_options->set_per_process_gpu_memory_fraction(
      per_process_gpu_memory_fraction);
  for (const auto& v : memory_limit_mb) {
    auto virtual_devices =
        gpu_options->mutable_experimental()->add_virtual_devices();
    for (float mb : v) {
      virtual_devices->add_memory_limit_mb(mb);
    }
  }
  return options;
}

static bool StartsWith(const string& lhs, const string& rhs) {
  if (rhs.length() > lhs.length()) return false;
  return lhs.substr(0, rhs.length()) == rhs;
}

TEST(GPUDeviceTest, FailedToParseVisibleDeviceList) {
  SessionOptions opts = MakeSessionOptions("0,abc");
  std::vector<tensorflow::Device*> devices;
  Status status = DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices);
  EXPECT_EQ(status.code(), error::INVALID_ARGUMENT);
  EXPECT_TRUE(StartsWith(status.error_message(), "Could not parse entry"))
      << status;
}

TEST(GPUDeviceTest, InvalidGpuId) {
  SessionOptions opts = MakeSessionOptions("100");
  std::vector<tensorflow::Device*> devices;
  Status status = DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices);
  EXPECT_EQ(status.code(), error::INVALID_ARGUMENT);
  EXPECT_TRUE(StartsWith(status.error_message(),
                         "'visible_device_list' listed an invalid GPU id"))
      << status;
}

TEST(GPUDeviceTest, DuplicateEntryInVisibleDeviceList) {
  SessionOptions opts = MakeSessionOptions("0,0");
  std::vector<tensorflow::Device*> devices;
  Status status = DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices);
  EXPECT_EQ(status.code(), error::INVALID_ARGUMENT);
  EXPECT_TRUE(StartsWith(status.error_message(),
                         "visible_device_list contained a duplicate entry"))
      << status;
}

TEST(GPUDeviceTest, VirtualDeviceConfigConflictsWithMemoryFractionSettings) {
  SessionOptions opts = MakeSessionOptions("0", 0.1, 1, {{}});
  std::vector<tensorflow::Device*> devices;
  Status status = DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices);
  EXPECT_EQ(status.code(), error::INVALID_ARGUMENT);
  EXPECT_TRUE(StartsWith(status.error_message(),
                         "It's invalid to set per_process_gpu_memory_fraction"))
      << status;
}

TEST(GPUDeviceTest, GpuDeviceCountTooSmall) {
  // device_count is 0, but with one entry in visible_device_list and one
  // (empty) VirtualDevices messages.
  SessionOptions opts = MakeSessionOptions("0", 0, 0, {{}});
  std::vector<tensorflow::Device*> devices;
  Status status = DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices);
  EXPECT_EQ(status.code(), error::UNKNOWN);
  EXPECT_TRUE(StartsWith(status.error_message(),
                         "Not enough GPUs to create virtual devices."))
      << status;
}

TEST(GPUDeviceTest, NotEnoughGpuInVisibleDeviceList) {
  // Single entry in visible_device_list with two (empty) VirtualDevices
  // messages.
  SessionOptions opts = MakeSessionOptions("0", 0, 8, {{}, {}});
  std::vector<tensorflow::Device*> devices;
  Status status = DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices);
  EXPECT_EQ(status.code(), error::UNKNOWN);
  EXPECT_TRUE(StartsWith(status.error_message(),
                         "Not enough GPUs to create virtual devices."))
      << status;
}

TEST(GPUDeviceTest, VirtualDeviceConfigConflictsWithVisibleDeviceList) {
  // This test requires at least two visible GPU hardware.
  if (GPUMachineManager()->VisibleDeviceCount() < 2) return;
  // Three entries in visible_device_list with two (empty) VirtualDevices
  // messages.
  SessionOptions opts = MakeSessionOptions("0,1", 0, 8, {{}});
  std::vector<tensorflow::Device*> devices;
  Status status = DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices);
  EXPECT_EQ(status.code(), error::INVALID_ARGUMENT);
  EXPECT_TRUE(StartsWith(status.error_message(),
                         "The number of GPUs in visible_device_list doesn't "
                         "match the number of elements in the virtual_devices "
                         "list."))
      << status;
}

TEST(GPUDeviceTest, EmptyVirtualDeviceConfig) {
  // It'll create single virtual device when the virtual device config is empty.
  SessionOptions opts = MakeSessionOptions("0");
  std::vector<tensorflow::Device*> devices;
  TF_CHECK_OK(DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices));
  EXPECT_EQ(1, devices.size());
  EXPECT_GE(devices[0]->attributes().memory_limit(), 0);
  for (auto d : devices) delete d;
}

TEST(GPUDeviceTest, SingleVirtualDeviceWithNoMemoryLimit) {
  // It'll create single virtual device for the gpu in question when
  // memory_limit_mb is unset.
  SessionOptions opts = MakeSessionOptions("0", 0, 1, {{}});
  std::vector<tensorflow::Device*> devices;
  TF_CHECK_OK(DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices));
  EXPECT_EQ(1, devices.size());
  EXPECT_GE(devices[0]->attributes().memory_limit(), 0);
  for (auto d : devices) delete d;
}

TEST(GPUDeviceTest, SingleVirtualDeviceWithMemoryLimit) {
  SessionOptions opts = MakeSessionOptions("0", 0, 1, {{123}});
  std::vector<tensorflow::Device*> devices;
  TF_CHECK_OK(DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices));
  EXPECT_EQ(1, devices.size());
  EXPECT_EQ(123 << 20, devices[0]->attributes().memory_limit());
  for (auto d : devices) delete d;
}

TEST(GPUDeviceTest, MultipleVirtualDevices) {
  SessionOptions opts = MakeSessionOptions("0", 0, 1, {{123, 456}});
  std::vector<tensorflow::Device*> devices;
  TF_CHECK_OK(DeviceFactory::GetFactory("GPU")->CreateDevices(
      opts, kDeviceNamePrefix, &devices));
  EXPECT_EQ(2, devices.size());
  EXPECT_EQ(123 << 20, devices[0]->attributes().memory_limit());
  EXPECT_EQ(456 << 20, devices[1]->attributes().memory_limit());
  ASSERT_EQ(1, devices[0]->attributes().locality().links().link_size());
  ASSERT_EQ(1, devices[1]->attributes().locality().links().link_size());
  EXPECT_EQ(1, devices[0]->attributes().locality().links().link(0).device_id());
  EXPECT_EQ("SAME_DEVICE",
            devices[0]->attributes().locality().links().link(0).type());
  EXPECT_EQ(BaseGPUDeviceFactory::InterconnectMap::kSameDeviceStrength,
            devices[0]->attributes().locality().links().link(0).strength());
  EXPECT_EQ(0, devices[1]->attributes().locality().links().link(0).device_id());
  EXPECT_EQ("SAME_DEVICE",
            devices[1]->attributes().locality().links().link(0).type());
  EXPECT_EQ(BaseGPUDeviceFactory::InterconnectMap::kSameDeviceStrength,
            devices[1]->attributes().locality().links().link(0).strength());
  for (auto d : devices) delete d;
}

}  // namespace
}  // namespace tensorflow

#endif
