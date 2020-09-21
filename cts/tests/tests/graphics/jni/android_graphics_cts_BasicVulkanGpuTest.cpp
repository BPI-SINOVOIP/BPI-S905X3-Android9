/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define LOG_TAG "BasicVulkanGpuTest"

#include <map>
#include <string>

#include <android/hardware_buffer.h>
#include <android/log.h>
#include <jni.h>
#include <unistd.h>

#include "NativeTestHelpers.h"
#include "VulkanTestHelpers.h"

namespace {

static constexpr uint32_t kTestImageWidth = 64;
static constexpr uint32_t kTestImageHeight = 64;

} // namespace

// A Vulkan AHardwareBuffer import test which does the following:
// 1) Allocates an AHardwareBuffer in one of 5 formats.
// 2) Populates the buffer with well-defined data.
// 3) Creates a VkImage from this AHardwareBuffer.
// 4) Renders the AHardwareBuffer to a Vulkan RGBA intermediate.
// 5) Reads back the intermediate into a CPU accessible VkBuffer.
// 6) Validates that the values are as expected.
static void verifyBasicBufferImport(JNIEnv *env, jclass, jobject assetMgr,
                                    jint format, jboolean useExternalFormat) {
  // Define and chose parameters.
  struct FormatDescription {
    std::string name;
    size_t pixelWidth;
    VkFormat vkFormat;
  };
  std::map<uint32_t, FormatDescription> bufferFormats{
      {AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
       {"AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM", 4, VK_FORMAT_R8G8B8A8_UNORM}},
      {AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM,
       {"AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM", 2,
        VK_FORMAT_R5G6B5_UNORM_PACK16}},
      {AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM,
       {"AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM", 4, VK_FORMAT_R8G8B8A8_UNORM}},
      {AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM,
       {"AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM", 3, VK_FORMAT_R8G8B8_UNORM}},
      {AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM,
       {"AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM", 4,
        VK_FORMAT_A2B10G10R10_UNORM_PACK32}},
      // TODO(ericrk): Test float and non-renderable formats.
  };
  ASSERT(bufferFormats.find(format) != bufferFormats.end(),
         "Called verifyBasicBufferImport with unexpected format %d.", format);
  const FormatDescription &formatDesc = bufferFormats[format];

  // Set up Vulkan.
  VkInit init;
  if (!init.init()) {
    // Could not initialize Vulkan due to lack of device support, skip test.
    return;
  }
  VkImageRenderer renderer(&init, kTestImageWidth, kTestImageHeight,
                           formatDesc.vkFormat, formatDesc.pixelWidth);
  ASSERT(renderer.init(env, assetMgr), "Unable to initialize VkRenderer.");

  // Create and initialize buffer based on parameters.
  AHardwareBuffer_Desc hwbDesc{
      .width = kTestImageWidth,
      .height = kTestImageHeight,
      .layers = 1,
      .usage = AHARDWAREBUFFER_USAGE_CPU_READ_NEVER |
               AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
               AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
      .format = static_cast<uint32_t>(format),
  };
  AHardwareBuffer *buffer;
  if (0 != AHardwareBuffer_allocate(&hwbDesc, &buffer)) {
    // We don't require support for all formats, we only require that if a
    // format is supported it must be importable into Vulkan.
    return;
  }

  // Populate the buffer with well-defined data.
  AHardwareBuffer_describe(buffer, &hwbDesc);
  uint8_t *bufferAddr;
  ASSERT(0 == AHardwareBuffer_lock(
                  buffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr,
                  reinterpret_cast<void **>(&bufferAddr)),
         "Unable to lock hardware buffer.");

  uint8_t *dst = bufferAddr;
  for (size_t y = 0; y < kTestImageHeight; ++y) {
    for (size_t x = 0; x < kTestImageWidth; ++x) {
      uint8_t *target = dst + ((y * hwbDesc.stride * formatDesc.pixelWidth) +
                               x * formatDesc.pixelWidth);
      *target = x + y;
    }
  }

  int syncFd = -1;
  AHardwareBuffer_unlock(buffer, &syncFd);

  // Import the AHardwareBuffer into Vulkan.
  VkAHardwareBufferImage vkImage(&init);
  ASSERT(vkImage.init(buffer, useExternalFormat, syncFd),
         "Could not initialize VkAHardwareBufferImage.");

  // Render the AHardwareBuffer and read back the result.
  std::vector<uint8_t> framePixels;
  ASSERT(renderer.renderImageAndReadback(vkImage.image(), vkImage.sampler(),
                                         vkImage.view(), vkImage.semaphore(),
                                         vkImage.isSamplerImmutable(), &framePixels),
         "Could not render/read-back image bits.");
  ASSERT(framePixels.size() ==
             kTestImageWidth * kTestImageHeight * formatDesc.pixelWidth,
         "Got unexpected pixel size.");

  // Check that the result is as expected.
  for (uint32_t y = 0; y < kTestImageHeight; ++y) {
    for (uint32_t x = 0; x < kTestImageWidth; ++x) {
      size_t offset = y * kTestImageWidth * formatDesc.pixelWidth +
                      x * formatDesc.pixelWidth;
      ASSERT(framePixels[offset] == x + y,
             "Format %s Expected %d at (%d,%d), got %d",
             formatDesc.name.c_str(), x + y, x, y, framePixels[offset]);
    }
  }
}

static JNINativeMethod gMethods[] = {
    {"verifyBasicBufferImport", "(Landroid/content/res/AssetManager;IZ)V",
     (void *)verifyBasicBufferImport},
};

int register_android_graphics_cts_BasicVulkanGpuTest(JNIEnv *env) {
  jclass clazz = env->FindClass("android/graphics/cts/BasicVulkanGpuTest");
  return env->RegisterNatives(clazz, gMethods,
                              sizeof(gMethods) / sizeof(JNINativeMethod));
}
