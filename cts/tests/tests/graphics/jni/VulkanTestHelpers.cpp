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

#define LOG_TAG "VulkanTestHelpers"

#include "VulkanTestHelpers.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/hardware_buffer.h>
#include <android/log.h>

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define ASSERT(a)                                                              \
  if (!(a)) {                                                                  \
    ALOGE("Failure: " #a " at " __FILE__ ":%d", __LINE__);                     \
    return false;                                                              \
  }

#define VK_CALL(a) ASSERT(VK_SUCCESS == (a))

namespace {

void addImageTransitionBarrier(VkCommandBuffer commandBuffer, VkImage image,
                               VkPipelineStageFlags srcStageMask,
                               VkPipelineStageFlags dstStageMask,
                               VkAccessFlags srcAccessMask,
                               VkAccessFlags dstAccessMask,
                               VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t srcQueue = VK_QUEUE_FAMILY_IGNORED,
                               uint32_t dstQueue = VK_QUEUE_FAMILY_IGNORED) {
  const VkImageSubresourceRange subResourcerange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  const VkImageMemoryBarrier imageBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = srcAccessMask,
      .dstAccessMask = dstAccessMask,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = srcQueue,
      .dstQueueFamilyIndex = dstQueue,
      .image = image,
      .subresourceRange = subResourcerange,
  };
  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr,
                       0, nullptr, 1, &imageBarrier);
}

} // namespace

bool VkInit::init() {
  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .apiVersion = VK_MAKE_VERSION(1, 1, 0),
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .pApplicationName = "VulkanGpuTest",
      .pEngineName = "VulkanGpuTestEngine",
  };
  std::vector<const char *> instanceExt, deviceExt;
  instanceExt.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  instanceExt.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
  instanceExt.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
  instanceExt.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  deviceExt.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
  deviceExt.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
  deviceExt.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
  deviceExt.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
  deviceExt.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
  deviceExt.push_back(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME);
  deviceExt.push_back(VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME);
  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = static_cast<uint32_t>(instanceExt.size()),
      .ppEnabledExtensionNames = instanceExt.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
  };
  VK_CALL(vkCreateInstance(&createInfo, nullptr, &mInstance));

  // Find a GPU to use.
  uint32_t gpuCount = 1;
  int status = vkEnumeratePhysicalDevices(mInstance, &gpuCount, &mGpu);
  ASSERT(status == VK_SUCCESS || status == VK_INCOMPLETE);
  ASSERT(gpuCount > 0);

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(mGpu, &queueFamilyCount, nullptr);
  ASSERT(queueFamilyCount != 0);
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(mGpu, &queueFamilyCount,
                                           queueFamilyProperties.data());

  uint32_t queueFamilyIndex;
  for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount;
       ++queueFamilyIndex) {
    if (queueFamilyProperties[queueFamilyIndex].queueFlags &
        VK_QUEUE_GRAPHICS_BIT)
      break;
  }
  ASSERT(queueFamilyIndex < queueFamilyCount);
  mQueueFamilyIndex = queueFamilyIndex;

  float priorities[] = {1.0f};
  VkDeviceQueueCreateInfo queueCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueCount = 1,
      .queueFamilyIndex = queueFamilyIndex,
      .pQueuePriorities = priorities,
  };

  VkDeviceCreateInfo deviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = nullptr,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCreateInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExt.size()),
      .ppEnabledExtensionNames = deviceExt.data(),
      .pEnabledFeatures = nullptr,
  };

  VK_CALL(vkCreateDevice(mGpu, &deviceCreateInfo, nullptr, &mDevice));

  mPfnGetAndroidHardwareBufferPropertiesANDROID =
      (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)vkGetDeviceProcAddr(
          mDevice, "vkGetAndroidHardwareBufferPropertiesANDROID");
  ASSERT(mPfnGetAndroidHardwareBufferPropertiesANDROID);

  VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR ycbcrFeatures{
      .sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES_KHR,
      .pNext = nullptr,
  };
  VkPhysicalDeviceFeatures2KHR physicalDeviceFeatures{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
      .pNext = &ycbcrFeatures,
  };
  PFN_vkGetPhysicalDeviceFeatures2KHR getFeatures =
      (PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(
          mInstance, "vkGetPhysicalDeviceFeatures2KHR");
  ASSERT(getFeatures);
  getFeatures(mGpu, &physicalDeviceFeatures);
  ASSERT(ycbcrFeatures.samplerYcbcrConversion == VK_TRUE);

  vkGetDeviceQueue(mDevice, 0, 0, &mQueue);
  vkGetPhysicalDeviceMemoryProperties(mGpu, &mMemoryProperties);

  return true;
}

VkInit::~VkInit() {
  if (mQueue != VK_NULL_HANDLE) {
    // Queues are implicitly destroyed with the device.
    mQueue = VK_NULL_HANDLE;
  }
  if (mDevice != VK_NULL_HANDLE) {
    vkDestroyDevice(mDevice, nullptr);
    mDevice = VK_NULL_HANDLE;
  }
  if (mInstance != VK_NULL_HANDLE) {
    vkDestroyInstance(mInstance, nullptr);
    mInstance = VK_NULL_HANDLE;
  }
}

uint32_t VkInit::findMemoryType(uint32_t memoryTypeBitsRequirement,
                                VkFlags requirementsMask) {
  for (uint32_t memoryIndex = 0; memoryIndex < VK_MAX_MEMORY_TYPES;
       ++memoryIndex) {
    const uint32_t memoryTypeBits = (1 << memoryIndex);
    const bool isRequiredMemoryType =
        memoryTypeBitsRequirement & memoryTypeBits;
    const bool satisfiesFlags =
        (mMemoryProperties.memoryTypes[memoryIndex].propertyFlags &
         requirementsMask) == requirementsMask;
    if (isRequiredMemoryType && satisfiesFlags)
      return memoryIndex;
  }

  // failed to find memory type.
  ALOGE("Couldn't find required memory type.");
  return 0;
}

VkAHardwareBufferImage::VkAHardwareBufferImage(VkInit *init) : mInit(init) {}

bool VkAHardwareBufferImage::init(AHardwareBuffer *buffer, bool useExternalFormat, int syncFd) {
  AHardwareBuffer_Desc bufferDesc;
  AHardwareBuffer_describe(buffer, &bufferDesc);
  ASSERT(bufferDesc.layers == 1);

  VkAndroidHardwareBufferFormatPropertiesANDROID formatInfo = {
      .sType =
          VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID,
      .pNext = nullptr,
  };
  VkAndroidHardwareBufferPropertiesANDROID properties = {
      .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,
      .pNext = &formatInfo,
  };
  VK_CALL(mInit->getHardwareBufferPropertiesFn()(mInit->device(), buffer,
                                                 &properties));
  ASSERT(useExternalFormat || formatInfo.format != VK_FORMAT_UNDEFINED);
  // Create an image to bind to our AHardwareBuffer.
  VkExternalFormatANDROID externalFormat{
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID,
      .pNext = nullptr,
      .externalFormat = formatInfo.externalFormat,
  };
  VkExternalMemoryImageCreateInfo externalCreateInfo{
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
      .pNext = useExternalFormat ? &externalFormat : nullptr,
      .handleTypes =
          VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
  };
  VkImageCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = &externalCreateInfo,
      .flags = 0u,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = useExternalFormat ? VK_FORMAT_UNDEFINED : formatInfo.format,
      .extent =
          {
              bufferDesc.width, bufferDesc.height, 1u,
          },
      .mipLevels = 1u,
      .arrayLayers = 1u,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VK_CALL(vkCreateImage(mInit->device(), &createInfo, nullptr, &mImage));

  VkImageMemoryRequirementsInfo2 memReqsInfo;
  memReqsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
  memReqsInfo.pNext = nullptr;
  memReqsInfo.image = mImage;

  VkMemoryDedicatedRequirements dedicatedMemReqs;
  dedicatedMemReqs.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
  dedicatedMemReqs.pNext = nullptr;

  VkMemoryRequirements2 memReqs;
  memReqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
  memReqs.pNext = &dedicatedMemReqs;

  PFN_vkGetImageMemoryRequirements2KHR getImageMemoryRequirements =
      (PFN_vkGetImageMemoryRequirements2KHR)vkGetDeviceProcAddr(
          mInit->device(), "vkGetImageMemoryRequirements2KHR");
  ASSERT(getImageMemoryRequirements);
  getImageMemoryRequirements(mInit->device(), &memReqsInfo, &memReqs);
  ASSERT(VK_TRUE == dedicatedMemReqs.prefersDedicatedAllocation);
  ASSERT(VK_TRUE == dedicatedMemReqs.requiresDedicatedAllocation);

  VkImportAndroidHardwareBufferInfoANDROID androidHardwareBufferInfo{
      .sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
      .pNext = nullptr,
      .buffer = buffer,
  };
  VkMemoryDedicatedAllocateInfo memoryAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
      .pNext = &androidHardwareBufferInfo,
      .image = mImage,
      .buffer = VK_NULL_HANDLE,
  };
  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateInfo,
      .allocationSize = properties.allocationSize,
      .memoryTypeIndex = mInit->findMemoryType(
          properties.memoryTypeBits, 0u /* requirementsMask */),
  };

  VK_CALL(vkAllocateMemory(mInit->device(), &allocateInfo, nullptr, &mMemory));
  VkBindImageMemoryInfo bindImageInfo;
  bindImageInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
  bindImageInfo.pNext = nullptr;
  bindImageInfo.image = mImage;
  bindImageInfo.memory = mMemory;
  bindImageInfo.memoryOffset = 0;

  PFN_vkBindImageMemory2KHR bindImageMemory =
      (PFN_vkBindImageMemory2KHR)vkGetDeviceProcAddr(mInit->device(),
                                                     "vkBindImageMemory2KHR");
  ASSERT(bindImageMemory);
  VK_CALL(bindImageMemory(mInit->device(), 1, &bindImageInfo));

  if (useExternalFormat /* TODO: || explicit format requires conversion */) {
    VkSamplerYcbcrConversionCreateInfo conversionCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
        .pNext = &externalFormat,
        .format = useExternalFormat ? VK_FORMAT_UNDEFINED : formatInfo.format,
        .ycbcrModel = formatInfo.suggestedYcbcrModel,
        .ycbcrRange = formatInfo.suggestedYcbcrRange,
        .components = formatInfo.samplerYcbcrConversionComponents,
        .xChromaOffset = formatInfo.suggestedXChromaOffset,
        .yChromaOffset = formatInfo.suggestedYChromaOffset,
        .chromaFilter = VK_FILTER_NEAREST,
        .forceExplicitReconstruction = VK_FALSE,
    };
    PFN_vkCreateSamplerYcbcrConversionKHR createSamplerYcbcrConversion =
        (PFN_vkCreateSamplerYcbcrConversionKHR)vkGetDeviceProcAddr(
            mInit->device(), "vkCreateSamplerYcbcrConversionKHR");
    ASSERT(createSamplerYcbcrConversion);
    VK_CALL(createSamplerYcbcrConversion(mInit->device(), &conversionCreateInfo,
                                         nullptr, &mConversion));
  }
  VkSamplerYcbcrConversionInfo samplerConversionInfo{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
      .pNext = &externalFormat,
      .conversion = mConversion,
  };

  VkSamplerCreateInfo samplerCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext =
          (mConversion == VK_NULL_HANDLE) ? nullptr : &samplerConversionInfo,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_NEVER,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      .unnormalizedCoordinates = VK_FALSE,
  };
  VK_CALL(
      vkCreateSampler(mInit->device(), &samplerCreateInfo, nullptr, &mSampler));

  VkImageViewCreateInfo viewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext =
          (mConversion == VK_NULL_HANDLE) ? nullptr : &samplerConversionInfo,
      .image = mImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = useExternalFormat ? VK_FORMAT_UNDEFINED : formatInfo.format,
      .components =
          {
              VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
          },
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
      .flags = 0,
  };
  VK_CALL(vkCreateImageView(mInit->device(), &viewCreateInfo, nullptr, &mView));

  // Create semaphore if necessary.
  if (syncFd != -1) {
    VkSemaphoreCreateInfo semaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    VK_CALL(vkCreateSemaphore(mInit->device(), &semaphoreCreateInfo, nullptr,
                              &mSemaphore));

    // Import the fd into a semaphore.
    VkImportSemaphoreFdInfoKHR importSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR,
        .pNext = nullptr,
        .semaphore = mSemaphore,
        .flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT,
        .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
        .fd = syncFd,
    };

    PFN_vkImportSemaphoreFdKHR importSemaphoreFd =
        (PFN_vkImportSemaphoreFdKHR)vkGetDeviceProcAddr(
            mInit->device(), "vkImportSemaphoreFdKHR");
    ASSERT(importSemaphoreFd);
    VK_CALL(importSemaphoreFd(mInit->device(), &importSemaphoreInfo));
  }

  return true;
}

VkAHardwareBufferImage::~VkAHardwareBufferImage() {
  if (mView != VK_NULL_HANDLE) {
    vkDestroyImageView(mInit->device(), mView, nullptr);
    mView = VK_NULL_HANDLE;
  }
  if (mSampler != VK_NULL_HANDLE) {
    vkDestroySampler(mInit->device(), mSampler, nullptr);
    mSampler = VK_NULL_HANDLE;
  }
  if (mConversion != VK_NULL_HANDLE) {
    PFN_vkDestroySamplerYcbcrConversionKHR destroySamplerYcbcrConversion =
        (PFN_vkDestroySamplerYcbcrConversionKHR)vkGetDeviceProcAddr(
            mInit->device(), "vkDestroySamplerYcbcrConversionKHR");
    destroySamplerYcbcrConversion(mInit->device(), mConversion, nullptr);
  }
  if (mMemory != VK_NULL_HANDLE) {
    vkFreeMemory(mInit->device(), mMemory, nullptr);
    mMemory = VK_NULL_HANDLE;
  }
  if (mImage != VK_NULL_HANDLE) {
    vkDestroyImage(mInit->device(), mImage, nullptr);
    mImage = VK_NULL_HANDLE;
  }
  if (mSemaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(mInit->device(), mSemaphore, nullptr);
    mSemaphore = VK_NULL_HANDLE;
  }
}

VkImageRenderer::VkImageRenderer(VkInit *init, uint32_t width, uint32_t height,
                                 VkFormat format, uint32_t bytesPerPixel)
    : mInit(init), mWidth(width), mHeight(height), mFormat(format),
      mResultBufferSize(width * height * bytesPerPixel) {}

bool VkImageRenderer::init(JNIEnv *env, jobject assetMgr) {
  // Create an image to back our framebuffer.
  {
    const VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    const VkImageCreateInfo imageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = mFormat,
        .extent = {mWidth, mHeight, 1u},
        .mipLevels = 1u,
        .arrayLayers = 1u,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = imageUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0u,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VK_CALL(
        vkCreateImage(mInit->device(), &imageCreateInfo, nullptr, &mDestImage));

    // Get memory requirements for image and allocate memory backing.
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(mInit->device(), mDestImage,
                                 &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex =
            mInit->findMemoryType(memoryRequirements.memoryTypeBits, 0u),
    };
    VK_CALL(vkAllocateMemory(mInit->device(), &memoryAllocateInfo, nullptr,
                             &mDestImageMemory));
    VK_CALL(
        vkBindImageMemory(mInit->device(), mDestImage, mDestImageMemory, 0));
  }

  // Create image view.
  {
    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .image = mDestImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = mFormat,
        .components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                       VK_COMPONENT_SWIZZLE_IDENTITY,
                       VK_COMPONENT_SWIZZLE_IDENTITY,
                       VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u},
    };
    VK_CALL(vkCreateImageView(mInit->device(), &imageViewCreateInfo, nullptr,
                              &mDestImageView));
  }

  // Create render pass
  {
    VkAttachmentDescription attachmentDesc{
        .flags = 0u,
        .format = mFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference attachmentRef{
        .attachment = 0u, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpassDesc{
        .flags = 0u,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0u,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1u,
        .pColorAttachments = &attachmentRef,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0u,
        .pPreserveAttachments = nullptr,
    };
    VkRenderPassCreateInfo renderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .attachmentCount = 1u,
        .pAttachments = &attachmentDesc,
        .subpassCount = 1u,
        .pSubpasses = &subpassDesc,
        .dependencyCount = 0u,
        .pDependencies = nullptr,
    };
    VK_CALL(vkCreateRenderPass(mInit->device(), &renderPassCreateInfo, nullptr,
                               &mRenderPass));
  }

  // Create vertex buffer.
  {
    const float vertexData[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f,
    };
    VkBufferCreateInfo createBufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = sizeof(vertexData),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .flags = 0,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    VK_CALL(vkCreateBuffer(mInit->device(), &createBufferInfo, nullptr,
                           &mVertexBuffer));

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(mInit->device(), mVertexBuffer, &memReq);
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memReq.size,
        .memoryTypeIndex = mInit->findMemoryType(
            memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    };
    VK_CALL(vkAllocateMemory(mInit->device(), &allocInfo, nullptr,
                             &mVertexBufferMemory));

    void *mappedData;
    VK_CALL(vkMapMemory(mInit->device(), mVertexBufferMemory, 0,
                        sizeof(vertexData), 0, &mappedData));
    memcpy(mappedData, vertexData, sizeof(vertexData));
    vkUnmapMemory(mInit->device(), mVertexBufferMemory);

    VK_CALL(vkBindBufferMemory(mInit->device(), mVertexBuffer,
                               mVertexBufferMemory, 0));
  }

  // Create framebuffer.
  {
    VkFramebufferCreateInfo framebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .renderPass = mRenderPass,
        .attachmentCount = 1u,
        .pAttachments = &mDestImageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1u,
    };
    VK_CALL(vkCreateFramebuffer(mInit->device(), &framebufferCreateInfo,
                                nullptr, &mFramebuffer));
  }

  // Create the host-side buffer which will be used to read back the results.
  {
    VkBufferCreateInfo bufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .size = mResultBufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0u,
        .pQueueFamilyIndices = nullptr,
    };
    VK_CALL(vkCreateBuffer(mInit->device(), &bufferCreateInfo, nullptr,
                           &mResultBuffer));

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(mInit->device(), mResultBuffer, &memReq);
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memReq.size,
        .memoryTypeIndex = mInit->findMemoryType(
            memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    };
    VK_CALL(vkAllocateMemory(mInit->device(), &allocInfo, nullptr,
                             &mResultBufferMemory));
    VK_CALL(vkBindBufferMemory(mInit->device(), mResultBuffer,
                               mResultBufferMemory, 0));
  }

  // Create shaders.
  {
    AAsset *vertFile =
        AAssetManager_open(AAssetManager_fromJava(env, assetMgr),
                           "passthrough_vsh.spv", AASSET_MODE_BUFFER);
    ASSERT(vertFile);
    size_t vertShaderLength = AAsset_getLength(vertFile);
    std::vector<uint8_t> vertShader;
    vertShader.resize(vertShaderLength);
    AAsset_read(vertFile, static_cast<void *>(vertShader.data()),
                vertShaderLength);
    AAsset_close(vertFile);

    AAsset *pixelFile =
        AAssetManager_open(AAssetManager_fromJava(env, assetMgr),
                           "passthrough_fsh.spv", AASSET_MODE_BUFFER);
    ASSERT(pixelFile);
    size_t pixelShaderLength = AAsset_getLength(pixelFile);
    std::vector<uint8_t> pixelShader;
    pixelShader.resize(pixelShaderLength);
    AAsset_read(pixelFile, static_cast<void *>(pixelShader.data()),
                pixelShaderLength);
    AAsset_close(pixelFile);

    VkShaderModuleCreateInfo vertexShaderInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .codeSize = vertShaderLength,
        .pCode = reinterpret_cast<const uint32_t *>(vertShader.data()),
    };
    VK_CALL(vkCreateShaderModule(mInit->device(), &vertexShaderInfo, nullptr,
                                 &mVertModule));

    VkShaderModuleCreateInfo pixelShaderInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .codeSize = pixelShaderLength,
        .pCode = reinterpret_cast<const uint32_t *>(pixelShader.data()),
    };
    VK_CALL(vkCreateShaderModule(mInit->device(), &pixelShaderInfo, nullptr,
                                 &mPixelModule));
  }

  VkPipelineCacheCreateInfo pipelineCacheInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      .pNext = nullptr,
      .initialDataSize = 0,
      .pInitialData = nullptr,
      .flags = 0, // reserved, must be 0
  };
  VK_CALL(vkCreatePipelineCache(mInit->device(), &pipelineCacheInfo, nullptr,
                                &mCache));

  // Create Descriptor Pool
  {
    const VkDescriptorPoolSize typeCount = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1,
    };
    const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &typeCount,
    };

    VK_CALL(vkCreateDescriptorPool(mInit->device(), &descriptorPoolCreateInfo,
                                   nullptr, &mDescriptorPool));
  }

  // Create command pool.
  {
    VkCommandPoolCreateInfo cmdPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = 0,
    };
    VK_CALL(vkCreateCommandPool(mInit->device(), &cmdPoolCreateInfo, nullptr,
                                &mCmdPool));
  }

  // Create a fence
  {
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    VK_CALL(vkCreateFence(mInit->device(), &fenceInfo, nullptr, &mFence));
  }

  return true;
}

VkImageRenderer::~VkImageRenderer() {
  cleanUpTemporaries();

  if (mCmdPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(mInit->device(), mCmdPool, nullptr);
    mCmdPool = VK_NULL_HANDLE;
  }
  if (mDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(mInit->device(), mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }
  if (mDestImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(mInit->device(), mDestImageView, nullptr);
    mDestImageView = VK_NULL_HANDLE;
  }
  if (mDestImage != VK_NULL_HANDLE) {
    vkDestroyImage(mInit->device(), mDestImage, nullptr);
    mDestImage = VK_NULL_HANDLE;
  }
  if (mDestImageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(mInit->device(), mDestImageMemory, nullptr);
    mDestImageMemory = VK_NULL_HANDLE;
  }
  if (mResultBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(mInit->device(), mResultBuffer, nullptr);
    mResultBuffer = VK_NULL_HANDLE;
  }
  if (mResultBufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(mInit->device(), mResultBufferMemory, nullptr);
    mResultBufferMemory = VK_NULL_HANDLE;
  }
  if (mRenderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(mInit->device(), mRenderPass, nullptr);
    mRenderPass = VK_NULL_HANDLE;
  }
  if (mVertexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(mInit->device(), mVertexBuffer, nullptr);
    mVertexBuffer = VK_NULL_HANDLE;
  }
  if (mVertexBufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(mInit->device(), mVertexBufferMemory, nullptr);
    mVertexBufferMemory = VK_NULL_HANDLE;
  }
  if (mFramebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(mInit->device(), mFramebuffer, nullptr);
    mFramebuffer = VK_NULL_HANDLE;
  }
  if (mCache != VK_NULL_HANDLE) {
    vkDestroyPipelineCache(mInit->device(), mCache, nullptr);
    mCache = VK_NULL_HANDLE;
  }
  if (mVertModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(mInit->device(), mVertModule, nullptr);
    mVertModule = VK_NULL_HANDLE;
  }
  if (mPixelModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(mInit->device(), mPixelModule, nullptr);
    mPixelModule = VK_NULL_HANDLE;
  }
  if (mFence != VK_NULL_HANDLE) {
    vkDestroyFence(mInit->device(), mFence, nullptr);
    mFence = VK_NULL_HANDLE;
  }
}

bool VkImageRenderer::renderImageAndReadback(VkImage image, VkSampler sampler,
                                             VkImageView view,
                                             VkSemaphore semaphore,
                                             bool useExternalFormat,
                                             std::vector<uint32_t> *data) {
  std::vector<uint8_t> unconvertedData;
  ASSERT(renderImageAndReadback(image, sampler, view, semaphore,
                                useExternalFormat, &unconvertedData));
  if ((unconvertedData.size() % sizeof(uint32_t)) != 0)
    return false;

  const uint32_t *dataPtr =
      reinterpret_cast<const uint32_t *>(unconvertedData.data());
  *data = std::vector<uint32_t>(dataPtr, dataPtr + unconvertedData.size() /
                                                       sizeof(uint32_t));
  return true;
}

bool VkImageRenderer::renderImageAndReadback(VkImage image, VkSampler sampler,
                                             VkImageView view,
                                             VkSemaphore semaphore,
                                             bool useImmutableSampler,
                                             std::vector<uint8_t> *data) {
  cleanUpTemporaries();

  // Create graphics pipeline.
  {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = useImmutableSampler ? &sampler : nullptr,
    };
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .bindingCount = 1,
        .pBindings = &descriptorSetLayoutBinding,
    };
    VK_CALL(vkCreateDescriptorSetLayout(mInit->device(),
                                        &descriptorSetLayoutCreateInfo, nullptr,
                                        &mDescriptorLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .setLayoutCount = 1,
        .pSetLayouts = &mDescriptorLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    VK_CALL(vkCreatePipelineLayout(mInit->device(), &pipelineLayoutCreateInfo,
                                   nullptr, &mLayout));

    VkPipelineShaderStageCreateInfo shaderStageParams[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = mVertModule,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = mPixelModule,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        }};

    VkViewport viewports{
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
        .x = 0,
        .y = 0,
        .width = static_cast<float>(mWidth),
        .height = static_cast<float>(mHeight),
    };

    VkRect2D scissor = {.extent = {mWidth, mHeight}, .offset = {0, 0}};
    VkPipelineViewportStateCreateInfo viewportInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &viewports,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
        .pSampleMask = &sampleMask,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    VkPipelineColorBlendAttachmentState attachmentStates{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &attachmentStates,
        .flags = 0,
    };
    VkPipelineRasterizationStateCreateInfo rasterInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1,
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Specify vertex input state
    VkVertexInputBindingDescription vertexInputBindingDescription{
        .binding = 0u,
        .stride = 4 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription vertex_input_attributes[2]{
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0,
        },
        {
            .binding = 0,
            .location = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = sizeof(float) * 2,
        }};
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStageParams,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pTessellationState = nullptr,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlendInfo,
        .pDynamicState = 0u,
        .layout = mLayout,
        .renderPass = mRenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    VK_CALL(vkCreateGraphicsPipelines(
        mInit->device(), mCache, 1, &pipelineCreateInfo, nullptr, &mPipeline));
  }

  // Create a command buffer.
  {
    VkCommandBufferAllocateInfo cmdBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = mCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VK_CALL(vkAllocateCommandBuffers(mInit->device(), &cmdBufferCreateInfo,
                                     &mCmdBuffer));
  }

  // Create the descriptor sets.
  {
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mDescriptorLayout};
    VK_CALL(
        vkAllocateDescriptorSets(mInit->device(), &allocInfo, &mDescriptorSet));

    VkDescriptorImageInfo texDesc{
        .sampler = sampler,
        .imageView = view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writeDst{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = mDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &texDesc,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr};
    vkUpdateDescriptorSets(mInit->device(), 1, &writeDst, 0, nullptr);
  }

  // Begin Command Buffer
  {
    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .pInheritanceInfo = nullptr,
    };

    VK_CALL(vkBeginCommandBuffer(mCmdBuffer, &cmdBufferBeginInfo));
  }

  // Transition the provided resource so we can sample from it.
  addImageTransitionBarrier(
      mCmdBuffer, image, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, VK_ACCESS_SHADER_READ_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_QUEUE_FAMILY_EXTERNAL_KHR, mInit->queueFamilyIndex());

  // Transition the destination texture for use as a framebuffer.
  addImageTransitionBarrier(
      mCmdBuffer, mDestImage, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  // Begin Render Pass to draw the source resource to the framebuffer.
  {
    const VkClearValue clearValue{
        .color =
            {
                .float32 = {0.0f, 0.0f, 0.0f, 0.0f},
            },
    };
    VkRenderPassBeginInfo renderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = mRenderPass,
        .framebuffer = mFramebuffer,
        .renderArea = {{0, 0}, {mWidth, mHeight}},
        .clearValueCount = 1u,
        .pClearValues = &clearValue,
    };
    vkCmdBeginRenderPass(mCmdBuffer, &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
  }

  /// Draw texture to renderpass.
  vkCmdBindPipeline(mCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
  vkCmdBindDescriptorSets(mCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout,
                          0u, 1, &mDescriptorSet, 0u, nullptr);
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(mCmdBuffer, 0, 1, &mVertexBuffer, &offset);
  vkCmdDraw(mCmdBuffer, 6u, 1u, 0u, 0u);
  vkCmdEndRenderPass(mCmdBuffer);

  // Copy to our staging buffer.
  {
    VkBufferMemoryBarrier bufferBarrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = mResultBuffer,
        .offset = 0u,
        .size = mResultBufferSize,
    };

    VkBufferImageCopy copyRegion{
        .bufferOffset = 0u,
        .bufferRowLength = mWidth,
        .bufferImageHeight = mHeight,
        .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u},
        .imageOffset = {0, 0, 0},
        .imageExtent = {mWidth, mHeight, 1u}};

    addImageTransitionBarrier(
        mCmdBuffer, mDestImage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    vkCmdCopyImageToBuffer(mCmdBuffer, mDestImage,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mResultBuffer,
                           1, &copyRegion);

    vkCmdPipelineBarrier(mCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT, (VkDependencyFlags)0, 0,
                         (const VkMemoryBarrier *)nullptr, 1, &bufferBarrier, 0,
                         (const VkImageMemoryBarrier *)nullptr);
  }

  VK_CALL(vkEndCommandBuffer(mCmdBuffer));

  VkPipelineStageFlags semaphoreWaitFlags =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = semaphore != VK_NULL_HANDLE ? 1u : 0u,
      .pWaitSemaphores = semaphore != VK_NULL_HANDLE ? &semaphore : nullptr,
      .pWaitDstStageMask =
          semaphore != VK_NULL_HANDLE ? &semaphoreWaitFlags : nullptr,
      .commandBufferCount = 1u,
      .pCommandBuffers = &mCmdBuffer,
      .signalSemaphoreCount = 0u,
      .pSignalSemaphores = nullptr,
  };
  VK_CALL(vkResetFences(mInit->device(), 1, &mFence))
  VK_CALL(vkQueueSubmit(mInit->queue(), 1, &submitInfo, mFence));
  VK_CALL(vkWaitForFences(mInit->device(), 1, &mFence, VK_TRUE,
                          ~(0ull) /* infinity */));

  void *outData;
  VK_CALL(vkMapMemory(mInit->device(), mResultBufferMemory, 0u,
                      mResultBufferSize, 0u, &outData));
  uint8_t *uData = reinterpret_cast<uint8_t *>(outData);
  *data = std::vector<uint8_t>(uData, uData + mResultBufferSize);
  vkUnmapMemory(mInit->device(), mResultBufferMemory);

  return true;
}

void VkImageRenderer::cleanUpTemporaries() {
  if (mCmdBuffer != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(mInit->device(), mCmdPool, 1, &mCmdBuffer);
    mCmdBuffer = VK_NULL_HANDLE;
  }
  if (mDescriptorSet != VK_NULL_HANDLE) {
    vkResetDescriptorPool(mInit->device(), mDescriptorPool, 0 /*flags*/);
    mDescriptorSet = VK_NULL_HANDLE;
  }
  if (mPipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(mInit->device(), mPipeline, nullptr);
    mPipeline = VK_NULL_HANDLE;
  }
  if (mDescriptorLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(mInit->device(), mDescriptorLayout, nullptr);
    mDescriptorLayout = VK_NULL_HANDLE;
  }
  if (mLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(mInit->device(), mLayout, nullptr);
    mLayout = VK_NULL_HANDLE;
  }
}
