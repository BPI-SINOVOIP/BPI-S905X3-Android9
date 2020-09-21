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

#ifndef ANDROID_VULKANTESTHELPERS_H
#define ANDROID_VULKANTESTHELPERS_H

#include <vector>

#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

// Provides basic set-up of a Vulkan device and queue.
class VkInit {
public:
  ~VkInit();

  bool init();
  VkDevice device() { return mDevice; }
  VkQueue queue() { return mQueue; }
  VkPhysicalDevice gpu() { return mGpu; }
  uint32_t queueFamilyIndex() { return mQueueFamilyIndex; }
  PFN_vkGetAndroidHardwareBufferPropertiesANDROID
    getHardwareBufferPropertiesFn() {
      return mPfnGetAndroidHardwareBufferPropertiesANDROID;
    }

  uint32_t findMemoryType(uint32_t memoryTypeBitsRequirement,
                          VkFlags requirementsMask);

private:
  VkInstance mInstance = VK_NULL_HANDLE;
  VkPhysicalDevice mGpu = VK_NULL_HANDLE;
  VkDevice mDevice = VK_NULL_HANDLE;
  VkQueue mQueue = VK_NULL_HANDLE;
  uint32_t mQueueFamilyIndex = 0;
  VkPhysicalDeviceMemoryProperties mMemoryProperties = {};
  PFN_vkGetAndroidHardwareBufferPropertiesANDROID
      mPfnGetAndroidHardwareBufferPropertiesANDROID = nullptr;
};

// Provides import of AHardwareBuffer.
class VkAHardwareBufferImage {
public:
  VkAHardwareBufferImage(VkInit *init);
  ~VkAHardwareBufferImage();

  bool init(AHardwareBuffer *buffer, bool useExternalFormat, int syncFd = -1);
  VkImage image() { return mImage; }
  VkSampler sampler() { return mSampler; }
  VkImageView view() { return mView; }
  VkSemaphore semaphore() { return mSemaphore; }
  bool isSamplerImmutable() { return mConversion != VK_NULL_HANDLE; }

private:
  VkInit *const mInit;
  VkImage mImage = VK_NULL_HANDLE;
  VkDeviceMemory mMemory = VK_NULL_HANDLE;
  VkSampler mSampler = VK_NULL_HANDLE;
  VkImageView mView = VK_NULL_HANDLE;
  VkSamplerYcbcrConversion mConversion = VK_NULL_HANDLE;
  VkSemaphore mSemaphore = VK_NULL_HANDLE;
};

// Renders a provided image to a new texture, then reads back that texture to
// disk.
class VkImageRenderer {
public:
  VkImageRenderer(VkInit *init, uint32_t width, uint32_t height,
                  VkFormat format, uint32_t bytesPerPixel);
  ~VkImageRenderer();

  bool init(JNIEnv *env, jobject assetMgr);
  bool renderImageAndReadback(VkImage image, VkSampler sampler,
                              VkImageView view, VkSemaphore semaphore,
                              bool useImmutableSampler,
                              std::vector<uint8_t> *data);
  bool renderImageAndReadback(VkImage image, VkSampler sampler,
                              VkImageView view, VkSemaphore semaphore,
                              bool useImmutableSampler,
                              std::vector<uint32_t> *data);

private:
  // Cleans up the temporary renderImageAndReadback variables.
  void cleanUpTemporaries();

  VkInit *const mInit;
  const uint32_t mWidth;
  const uint32_t mHeight;
  const VkFormat mFormat;
  const VkDeviceSize mResultBufferSize;
  VkImage mDestImage = VK_NULL_HANDLE;
  VkDeviceMemory mDestImageMemory = VK_NULL_HANDLE;
  VkImageView mDestImageView = VK_NULL_HANDLE;
  VkBuffer mResultBuffer = VK_NULL_HANDLE;
  VkDeviceMemory mResultBufferMemory = VK_NULL_HANDLE;
  VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
  VkCommandPool mCmdPool = VK_NULL_HANDLE;
  VkRenderPass mRenderPass = VK_NULL_HANDLE;
  VkBuffer mVertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;
  VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
  VkShaderModule mVertModule = VK_NULL_HANDLE;
  VkShaderModule mPixelModule = VK_NULL_HANDLE;
  VkFence mFence = VK_NULL_HANDLE;

  // Temporary values used during renderImageAndReadback.
  VkCommandBuffer mCmdBuffer = VK_NULL_HANDLE;
  VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
  VkPipelineCache mCache = VK_NULL_HANDLE;
  VkPipeline mPipeline = VK_NULL_HANDLE;
  VkDescriptorSetLayout mDescriptorLayout = VK_NULL_HANDLE;
  VkPipelineLayout mLayout = VK_NULL_HANDLE;
};

#endif // ANDROID_VULKANTESTHELPERS_H
