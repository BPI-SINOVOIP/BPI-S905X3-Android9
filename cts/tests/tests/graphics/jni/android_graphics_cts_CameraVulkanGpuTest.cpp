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
#include <jni.h>
#include <set>
#include <unistd.h>

#include "CameraTestHelpers.h"
#include "ImageReaderTestHelpers.h"
#include "NativeTestHelpers.h"
#include "VulkanTestHelpers.h"

namespace {

// Constants used to control test execution.
static constexpr uint32_t kTestImageWidth = 640;
static constexpr uint32_t kTestImageHeight = 480;
static constexpr uint32_t kTestImageFormat = AIMAGE_FORMAT_PRIVATE;
static constexpr uint64_t kTestImageUsage =
    AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;
static constexpr uint32_t kTestImageCount = 3;

// Checks if a vector has more than 10 unique values, in which case we consider
// it noisy.
bool isNoisy(const std::vector<uint32_t> &data) {
  std::set<uint32_t> values_seen;
  for (uint32_t value : data) {
    values_seen.insert(value);
  }
  return values_seen.size() > 10;
}

} // namespace

// A basic test which does the following:
// 1) Reads a Camera frame into an AHardwareBuffer.
// 2) Creates a VkImage from this AHardwareBuffer.
// 3) Renders the AHardwareBuffer to a Vulkan RGBA intermediate.
// 4) Reads back the intermediate into a CPU accessible VkBuffer.
// 5) Validates that there is noise in the buffer.
static void loadCameraAndVerifyFrameImport(JNIEnv *env, jclass,
                                           jobject assetMgr) {
  // Initialize Vulkan.
  VkInit init;
  if (!init.init()) {
    // Could not initialize Vulkan due to lack of device support, skip test.
    return;
  }
  VkImageRenderer renderer(&init, kTestImageWidth, kTestImageHeight,
                           VK_FORMAT_R8G8B8A8_UNORM, 4);
  ASSERT(renderer.init(env, assetMgr), "Could not init VkImageRenderer.");

  // Initialize image reader and camera helpers.
  ImageReaderHelper imageReader(kTestImageWidth, kTestImageHeight,
                                kTestImageFormat, kTestImageUsage,
                                kTestImageCount);
  CameraHelper camera;
  ASSERT(imageReader.initImageReader() >= 0,
         "Failed to initialize image reader.");
  ASSERT(camera.initCamera(imageReader.getNativeWindow()) >= 0,
         "Failed to initialize camera.");
  ASSERT(camera.isCameraReady(), "Camera is not ready.");

  // Acquire an AHardwareBuffer from the camera.
  ASSERT(camera.takePicture() >= 0, "Camera failed to take picture.");
  AHardwareBuffer *buffer;
  int ret = imageReader.getBufferFromCurrentImage(&buffer);
  while (ret != 0) {
    usleep(1000);
    ret = imageReader.getBufferFromCurrentImage(&buffer);
  }

  // Impor the AHardwareBuffer into Vulkan.
  VkAHardwareBufferImage vkImage(&init);
  ASSERT(vkImage.init(buffer, true /* useExternalFormat */), "Could not initialize VkAHardwareBufferImage.");

  // Render the AHardwareBuffer using Vulkan and read back the result.
  std::vector<uint32_t> imageData;
  ASSERT(renderer.renderImageAndReadback(
             vkImage.image(), vkImage.sampler(), vkImage.view(),
             vkImage.semaphore(), vkImage.isSamplerImmutable(), &imageData),
         "Could not render/read-back Vulkan pixels.");

  // Ensure that we see noise.
  ASSERT(isNoisy(imageData), "Camera data should be noisy.");
}

static JNINativeMethod gMethods[] = {
    {"loadCameraAndVerifyFrameImport", "(Landroid/content/res/AssetManager;)V",
     (void *)loadCameraAndVerifyFrameImport},
};

int register_android_graphics_cts_CameraVulkanGpuTest(JNIEnv *env) {
  jclass clazz = env->FindClass("android/graphics/cts/CameraVulkanGpuTest");
  return env->RegisterNatives(clazz, gMethods,
                              sizeof(gMethods) / sizeof(JNINativeMethod));
}
