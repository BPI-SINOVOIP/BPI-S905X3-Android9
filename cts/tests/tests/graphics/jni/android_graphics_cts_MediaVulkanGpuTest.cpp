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

#include <android/log.h>
#include <jni.h>
#include <unistd.h>

#include "ImageReaderTestHelpers.h"
#include "MediaTestHelpers.h"
#include "NativeTestHelpers.h"
#include "VulkanTestHelpers.h"

namespace {

static constexpr uint32_t kTestImageWidth = 1920;
static constexpr uint32_t kTestImageHeight = 1080;
static constexpr uint32_t kTestImageFormat = AIMAGE_FORMAT_YUV_420_888;
static constexpr uint64_t kTestImageUsage =
    AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
static constexpr uint32_t kTestImageCount = 3;

// Confirms that the two values match, allowing for an error of tolerance per
// channel.
bool fuzzyMatch(uint32_t value1, uint32_t value2, int32_t tolerance) {
  for (size_t i = 0; i < 4; ++i) {
    size_t shift = 8 * i;
    uint32_t mask = 0x000000FF << shift;

    int32_t value1Masked = static_cast<int32_t>((value1 & mask) >> shift);
    int32_t value2Masked = static_cast<int32_t>((value2 & mask) >> shift);

    if (std::abs(value1Masked - value2Masked) > tolerance)
      return false;
  }

  return true;
}

uint32_t swizzleBgraToRgba(uint32_t bgra) {
  uint32_t result = 0;
  result |= (bgra & 0xFF000000) >> 0;  // Alpha
  result |= (bgra & 0x00FF0000) >> 16; // Red
  result |= (bgra & 0x0000FF00) >> 0;  // Green
  result |= (bgra & 0x000000FF) << 16; // Blue
  return result;
}

} // namespace

// A Vulkan media import test which does the following:
// 1) Reads the first frame from a video as an AHardwareBuffer.
// 2) Creates a VkImage from this AHardwareBuffer.
// 3) Renders the AHardwareBuffer to a Vulkan RGBA intermediate.
// 4) Reads back the intermediate into a CPU accessible VkBuffer.
// 5) Validates that the values are as expected.
static void loadMediaAndVerifyFrameImport(JNIEnv *env, jclass, jobject assetMgr,
                                          jstring jfilename,
                                          jintArray referencePixels) {
  // Set up Vulkan.
  VkInit init;
  if (!init.init()) {
    // Could not initialize Vulkan due to lack of device support, skip test.
    return;
  }
  VkImageRenderer renderer(&init, kTestImageWidth, kTestImageHeight,
                           VK_FORMAT_R8G8B8A8_UNORM, 4);
  ASSERT(renderer.init(env, assetMgr), "Could not init VkImageRenderer.");

  // Set up the image reader and media helpers used to get a frames from video.
  ImageReaderHelper imageReader(kTestImageWidth, kTestImageHeight,
                                kTestImageFormat, kTestImageUsage,
                                kTestImageCount);
  ASSERT(imageReader.initImageReader() >= 0,
         "Failed to initialize image reader.");
  MediaHelper media;
  ASSERT(media.init(env, assetMgr, jfilename, imageReader.getNativeWindow()),
         "Failed to initialize media codec.");

  // Get an AHardwareBuffer for the first frame of the video.
  ASSERT(media.processOneFrame(),
         "Could not get a media frame to import into Vulkan.");
  AHardwareBuffer *buffer;
  int ret = imageReader.getBufferFromCurrentImage(&buffer);
  while (ret != 0) {
    usleep(1000);
    ret = imageReader.getBufferFromCurrentImage(&buffer);
  }

  // Import the AHardwareBuffer into Vulkan.
  VkAHardwareBufferImage vkImage(&init);
  ASSERT(vkImage.init(buffer, true /* useExternalFormat */), "Could not init VkAHardwareBufferImage.");

  // Render the AHardwareBuffer using Vulkan and read back the result.
  std::vector<uint32_t> framePixels;
  ASSERT(renderer.renderImageAndReadback(
             vkImage.image(), vkImage.sampler(), vkImage.view(),
             vkImage.semaphore(), vkImage.isSamplerImmutable(), &framePixels),
         "Could not get frame pixels from Vulkan.");
  ASSERT(framePixels.size() == kTestImageWidth * kTestImageHeight,
         "Unexpected number of pixels in frame");

  // Ensure that the data we read back matches our reference image.
  size_t referenceSize =
      static_cast<size_t>(env->GetArrayLength(referencePixels));
  ASSERT(framePixels.size() == referenceSize,
         "Unexpected number of pixels in frame.");
  uint32_t *referenceData = reinterpret_cast<uint32_t *>(
      env->GetIntArrayElements(referencePixels, 0));
  for (uint32_t x = 0; x < kTestImageWidth; ++x) {
    for (uint32_t y = 0; y < kTestImageHeight; ++y) {
      size_t offset = y * kTestImageWidth + x;
      static const int32_t kTolerance = 0x30;
      uint32_t value1 = framePixels[offset];
      // Reference data is BGRA, Vk data is BGRA.
      uint32_t value2 = swizzleBgraToRgba(referenceData[offset]);
      ASSERT(fuzzyMatch(value1, value2, kTolerance),
             "Expected ~0x%08X at (%i,%i), got 0x%08X", value2, x, y, value1);
    }
  }
}

static JNINativeMethod gMethods[] = {
    {"loadMediaAndVerifyFrameImport",
     "(Landroid/content/res/AssetManager;Ljava/lang/String;[I)V",
     (void *)loadMediaAndVerifyFrameImport},
};

int register_android_graphics_cts_MediaVulkanGpuTest(JNIEnv *env) {
  jclass clazz = env->FindClass("android/graphics/cts/MediaVulkanGpuTest");
  return env->RegisterNatives(clazz, gMethods,
                              sizeof(gMethods) / sizeof(JNINativeMethod));
}
