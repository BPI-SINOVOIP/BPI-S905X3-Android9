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
#include <cstring>

#include "VulkanPreTransformTestHelpers.h"

#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ASSERT(a)                                              \
    if (!(a)) {                                                \
        ALOGE("Failure: " #a " at " __FILE__ ":%d", __LINE__); \
        return -1;                                             \
    }
#define VK_CALL(a) ASSERT(VK_SUCCESS == (a))

static const float vertexData[] = {
        // L:left, T:top, R:right, B:bottom, C:center
        -1.0f, -1.0f, 0.0f, // LT
        -1.0f,  0.0f, 0.0f, // LC
         0.0f, -1.0f, 0.0f, // CT
         0.0f,  0.0f, 0.0f, // CC
         1.0f, -1.0f, 0.0f, // RT
         1.0f,  0.0f, 0.0f, // RC
        -1.0f,  0.0f, 0.0f, // LC
        -1.0f,  1.0f, 0.0f, // LB
         0.0f,  0.0f, 0.0f, // CC
         0.0f,  1.0f, 0.0f, // CB
         1.0f,  0.0f, 0.0f, // RC
         1.0f,  1.0f, 0.0f, // RB
};

static const float fragData[] = {
        1.0f, 0.0f, 0.0f, // Red
        0.0f, 1.0f, 0.0f, // Green
        0.0f, 0.0f, 1.0f, // Blue
        1.0f, 1.0f, 0.0f, // Yellow
};

static const char* requiredInstanceExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_android_surface",
};

static const char* requiredDeviceExtensions[] = {
        "VK_KHR_swapchain",
};

static bool enumerateInstanceExtensions(std::vector<VkExtensionProperties>* extensions) {
    VkResult result;

    uint32_t count = 0;
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (result != VK_SUCCESS) return false;

    extensions->resize(count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions->data());
    if (result != VK_SUCCESS) return false;

    return true;
}

static bool enumerateDeviceExtensions(VkPhysicalDevice device,
                                      std::vector<VkExtensionProperties>* extensions) {
    VkResult result;

    uint32_t count = 0;
    result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    if (result != VK_SUCCESS) return false;

    extensions->resize(count);
    result = vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions->data());
    if (result != VK_SUCCESS) return false;

    return true;
}

static bool hasExtension(const char* extension_name,
                         const std::vector<VkExtensionProperties>& extensions) {
    return std::find_if(extensions.cbegin(), extensions.cend(),
                        [extension_name](const VkExtensionProperties& extension) {
                            return strcmp(extension.extensionName, extension_name) == 0;
                        }) != extensions.cend();
}

DeviceInfo::DeviceInfo()
      : mInstance(VK_NULL_HANDLE),
        mGpu(VK_NULL_HANDLE),
        mWindow(nullptr),
        mSurface(VK_NULL_HANDLE),
        mQueueFamilyIndex(0),
        mDevice(VK_NULL_HANDLE),
        mQueue(VK_NULL_HANDLE) {}

DeviceInfo::~DeviceInfo() {
    if (mDevice) {
        vkDeviceWaitIdle(mDevice);
        vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    }
    if (mInstance) {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }
    if (mWindow) {
        ANativeWindow_release(mWindow);
        mWindow = nullptr;
    }
}

int32_t DeviceInfo::init(JNIEnv* env, jobject jSurface) {
    ASSERT(jSurface);

    mWindow = ANativeWindow_fromSurface(env, jSurface);
    ASSERT(mWindow);

    std::vector<VkExtensionProperties> supportedInstanceExtensions;
    ASSERT(enumerateInstanceExtensions(&supportedInstanceExtensions));

    std::vector<const char*> enabledInstanceExtensions;
    for (const auto extension : requiredInstanceExtensions) {
        ASSERT(hasExtension(extension, supportedInstanceExtensions));
        enabledInstanceExtensions.push_back(extension);
    }

    const VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "VulkanPreTransformTest",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_MAKE_VERSION(1, 0, 0),
    };
    const VkInstanceCreateInfo instanceInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size()),
            .ppEnabledExtensionNames = enabledInstanceExtensions.data(),
    };
    VK_CALL(vkCreateInstance(&instanceInfo, nullptr, &mInstance));

    uint32_t gpuCount = 0;
    VK_CALL(vkEnumeratePhysicalDevices(mInstance, &gpuCount, nullptr));
    if (gpuCount == 0) {
        ALOGD("No physical device available");
        return 1;
    }

    std::vector<VkPhysicalDevice> gpus(gpuCount, VK_NULL_HANDLE);
    VK_CALL(vkEnumeratePhysicalDevices(mInstance, &gpuCount, gpus.data()));

    mGpu = gpus[0];

    const VkAndroidSurfaceCreateInfoKHR surfaceInfo = {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = mWindow,
    };
    VK_CALL(vkCreateAndroidSurfaceKHR(mInstance, &surfaceInfo, nullptr, &mSurface));

    std::vector<VkExtensionProperties> supportedDeviceExtensions;
    ASSERT(enumerateDeviceExtensions(mGpu, &supportedDeviceExtensions));

    std::vector<const char*> enabledDeviceExtensions;
    for (const auto extension : requiredDeviceExtensions) {
        ASSERT(hasExtension(extension, supportedDeviceExtensions));
        enabledDeviceExtensions.push_back(extension);
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mGpu, &queueFamilyCount, nullptr);
    ASSERT(queueFamilyCount);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mGpu, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t queueFamilyIndex;
    for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex) {
        if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }
    ASSERT(queueFamilyIndex < queueFamilyCount);
    mQueueFamilyIndex = queueFamilyIndex;

    const float priority = 1.0f;
    const VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = mQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &priority,
    };
    const VkDeviceCreateInfo deviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(enabledDeviceExtensions.size()),
            .ppEnabledExtensionNames = enabledDeviceExtensions.data(),
            .pEnabledFeatures = nullptr,
    };
    VK_CALL(vkCreateDevice(mGpu, &deviceCreateInfo, nullptr, &mDevice));

    vkGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mQueue);

    return 0;
}

SwapchainInfo::SwapchainInfo(const DeviceInfo* const deviceInfo)
      : mDeviceInfo(deviceInfo),
        mFormat(VK_FORMAT_UNDEFINED),
        mDisplaySize({0, 0}),
        mSwapchain(VK_NULL_HANDLE),
        mSwapchainLength(0) {}

SwapchainInfo::~SwapchainInfo() {
    if (mDeviceInfo->device()) {
        vkDeviceWaitIdle(mDeviceInfo->device());
        vkDestroySwapchainKHR(mDeviceInfo->device(), mSwapchain, nullptr);
    }
}

int32_t SwapchainInfo::init(bool setPreTransform) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDeviceInfo->gpu(), mDeviceInfo->surface(),
                                                      &surfaceCapabilities));
    ALOGD("Vulkan Surface Capabilities:\n");
    ALOGD("\timage count: %u - %u\n", surfaceCapabilities.minImageCount,
          surfaceCapabilities.maxImageCount);
    ALOGD("\tarray layers: %u\n", surfaceCapabilities.maxImageArrayLayers);
    ALOGD("\timage size (now): %dx%d\n", surfaceCapabilities.currentExtent.width,
          surfaceCapabilities.currentExtent.height);
    ALOGD("\timage size (extent): %dx%d - %dx%d\n", surfaceCapabilities.minImageExtent.width,
          surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.width,
          surfaceCapabilities.maxImageExtent.height);
    ALOGD("\tusage: %x\n", surfaceCapabilities.supportedUsageFlags);
    ALOGD("\tcurrent transform: %u\n", surfaceCapabilities.currentTransform);
    ALOGD("\tallowed transforms: %x\n", surfaceCapabilities.supportedTransforms);
    ALOGD("\tcomposite alpha flags: %u\n", surfaceCapabilities.supportedCompositeAlpha);

    uint32_t formatCount = 0;
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(mDeviceInfo->gpu(), mDeviceInfo->surface(),
                                                 &formatCount, nullptr));

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(mDeviceInfo->gpu(), mDeviceInfo->surface(),
                                                 &formatCount, formats.data()));

    uint32_t formatIndex;
    for (formatIndex = 0; formatIndex < formatCount; ++formatIndex) {
        if (formats[formatIndex].format == VK_FORMAT_R8G8B8A8_UNORM) {
            break;
        }
    }
    ASSERT(formatIndex < formatCount);

    mFormat = formats[formatIndex].format;
    mDisplaySize = surfaceCapabilities.currentExtent;

    VkSurfaceTransformFlagBitsKHR preTransform =
            (setPreTransform ? surfaceCapabilities.currentTransform
                             : VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);
    ALOGD("currentTransform = %u, preTransform = %u",
          static_cast<uint32_t>(surfaceCapabilities.currentTransform),
          static_cast<uint32_t>(preTransform));

    const uint32_t queueFamilyIndex = mDeviceInfo->queueFamilyIndex();
    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = mDeviceInfo->surface(),
            .minImageCount = surfaceCapabilities.minImageCount,
            .imageFormat = mFormat,
            .imageColorSpace = formats[formatIndex].colorSpace,
            .imageExtent = mDisplaySize,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = preTransform,
            .imageArrayLayers = 1,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &queueFamilyIndex,
            .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_FALSE,
    };
    VK_CALL(vkCreateSwapchainKHR(mDeviceInfo->device(), &swapchainCreateInfo, nullptr,
                                 &mSwapchain));

    VK_CALL(vkGetSwapchainImagesKHR(mDeviceInfo->device(), mSwapchain, &mSwapchainLength, nullptr));
    ALOGD("Swapchain length = %u", mSwapchainLength);

    return 0;
}

Renderer::Renderer(const DeviceInfo* const deviceInfo, const SwapchainInfo* const swapchainInfo)
      : mDeviceInfo(deviceInfo),
        mSwapchainInfo(swapchainInfo),
        mDeviceMemory(VK_NULL_HANDLE),
        mVertexBuffer(VK_NULL_HANDLE),
        mRenderPass(VK_NULL_HANDLE),
        mVertexShader(VK_NULL_HANDLE),
        mFragmentShader(VK_NULL_HANDLE),
        mPipelineLayout(VK_NULL_HANDLE),
        mPipeline(VK_NULL_HANDLE),
        mCommandPool(VK_NULL_HANDLE),
        mSemaphore(VK_NULL_HANDLE),
        mFence(VK_NULL_HANDLE) {}

Renderer::~Renderer() {
    if (mDeviceInfo->device()) {
        vkDeviceWaitIdle(mDeviceInfo->device());
        vkDestroyShaderModule(mDeviceInfo->device(), mVertexShader, nullptr);
        vkDestroyShaderModule(mDeviceInfo->device(), mFragmentShader, nullptr);
        vkDestroyFence(mDeviceInfo->device(), mFence, nullptr);
        vkDestroySemaphore(mDeviceInfo->device(), mSemaphore, nullptr);
        if (!mCommandBuffers.empty()) {
            vkFreeCommandBuffers(mDeviceInfo->device(), mCommandPool, mCommandBuffers.size(),
                                 mCommandBuffers.data());
        }
        vkDestroyCommandPool(mDeviceInfo->device(), mCommandPool, nullptr);
        vkDestroyPipeline(mDeviceInfo->device(), mPipeline, nullptr);
        vkDestroyPipelineLayout(mDeviceInfo->device(), mPipelineLayout, nullptr);
        vkDestroyBuffer(mDeviceInfo->device(), mVertexBuffer, nullptr);
        vkFreeMemory(mDeviceInfo->device(), mDeviceMemory, nullptr);
        vkDestroyRenderPass(mDeviceInfo->device(), mRenderPass, nullptr);
        for (auto& framebuffer : mFramebuffers) {
            vkDestroyFramebuffer(mDeviceInfo->device(), framebuffer, nullptr);
        }
        for (auto& imageView : mImageViews) {
            vkDestroyImageView(mDeviceInfo->device(), imageView, nullptr);
        }
    }
}

int32_t Renderer::createRenderPass() {
    const VkAttachmentDescription attachmentDescription = {
            .flags = 0,
            .format = mSwapchainInfo->format(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    const VkAttachmentReference attachmentReference = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpassDescription = {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentReference,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
    };
    const VkRenderPassCreateInfo renderPassCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .attachmentCount = 1,
            .pAttachments = &attachmentDescription,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 0,
            .pDependencies = nullptr,
    };
    VK_CALL(vkCreateRenderPass(mDeviceInfo->device(), &renderPassCreateInfo, nullptr,
                               &mRenderPass));

    return 0;
}

int32_t Renderer::createFrameBuffers() {
    uint32_t swapchainLength = mSwapchainInfo->swapchainLength();
    std::vector<VkImage> images(swapchainLength, VK_NULL_HANDLE);
    VK_CALL(vkGetSwapchainImagesKHR(mDeviceInfo->device(), mSwapchainInfo->swapchain(),
                                    &swapchainLength, images.data()));

    mImageViews.resize(swapchainLength, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < swapchainLength; ++i) {
        const VkImageViewCreateInfo imageViewCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = mSwapchainInfo->format(),
                .components =
                        {
                                .r = VK_COMPONENT_SWIZZLE_R,
                                .g = VK_COMPONENT_SWIZZLE_G,
                                .b = VK_COMPONENT_SWIZZLE_B,
                                .a = VK_COMPONENT_SWIZZLE_A,
                        },
                .subresourceRange =
                        {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                        },
        };
        VK_CALL(vkCreateImageView(mDeviceInfo->device(), &imageViewCreateInfo, nullptr,
                                  &mImageViews[i]));
    }

    mFramebuffers.resize(swapchainLength, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < swapchainLength; ++i) {
        const VkFramebufferCreateInfo framebufferCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderPass = mRenderPass,
                .attachmentCount = 1,
                .pAttachments = &mImageViews[i],
                .width = mSwapchainInfo->displaySize().width,
                .height = mSwapchainInfo->displaySize().height,
                .layers = 1,
        };
        VK_CALL(vkCreateFramebuffer(mDeviceInfo->device(), &framebufferCreateInfo, nullptr,
                                    &mFramebuffers[i]));
    }

    return 0;
}

int32_t Renderer::createVertexBuffers() {
    const uint32_t queueFamilyIndex = mDeviceInfo->queueFamilyIndex();
    const VkBufferCreateInfo bufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = sizeof(vertexData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &queueFamilyIndex,
    };
    VK_CALL(vkCreateBuffer(mDeviceInfo->device(), &bufferCreateInfo, nullptr, &mVertexBuffer));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(mDeviceInfo->device(), mVertexBuffer, &memoryRequirements);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(mDeviceInfo->gpu(), &memoryProperties);

    int32_t typeIndex = -1;
    for (int32_t i = 0, typeBits = memoryRequirements.memoryTypeBits; i < 32; ++i) {
        if ((typeBits & 1) == 1) {
            if ((memoryProperties.memoryTypes[i].propertyFlags &
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                typeIndex = i;
                break;
            }
        }
        typeBits >>= 1;
    }
    ASSERT(typeIndex != -1);

    VkMemoryAllocateInfo memoryAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = static_cast<uint32_t>(typeIndex),
    };
    VK_CALL(vkAllocateMemory(mDeviceInfo->device(), &memoryAllocateInfo, nullptr, &mDeviceMemory));

    void* data;
    VK_CALL(vkMapMemory(mDeviceInfo->device(), mDeviceMemory, 0, sizeof(vertexData), 0, &data));

    memcpy(data, vertexData, sizeof(vertexData));
    vkUnmapMemory(mDeviceInfo->device(), mDeviceMemory);

    VK_CALL(vkBindBufferMemory(mDeviceInfo->device(), mVertexBuffer, mDeviceMemory, 0));

    return 0;
}

int32_t Renderer::loadShaderFromFile(const char* filePath, VkShaderModule* const outShader) {
    ASSERT(filePath);

    AAsset* file = AAssetManager_open(mAssetManager, filePath, AASSET_MODE_BUFFER);
    if (!file) {
        ALOGE("Failed to open shader file");
        return -1;
    }
    size_t fileLength = AAsset_getLength(file);
    std::vector<char> fileContent(fileLength);
    AAsset_read(file, fileContent.data(), fileLength);
    AAsset_close(file);

    const VkShaderModuleCreateInfo shaderModuleCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = fileLength,
            .pCode = (const uint32_t*)(fileContent.data()),
    };
    VK_CALL(vkCreateShaderModule(mDeviceInfo->device(), &shaderModuleCreateInfo, nullptr,
                                 outShader));

    return 0;
}

int32_t Renderer::createGraphicsPipeline() {
    const VkPushConstantRange pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = 3 * sizeof(float),
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
    };
    VK_CALL(vkCreatePipelineLayout(mDeviceInfo->device(), &pipelineLayoutCreateInfo, nullptr,
                                   &mPipelineLayout));

    ASSERT(!loadShaderFromFile("shaders/tri.vert.spv", &mVertexShader));
    ASSERT(!loadShaderFromFile("shaders/tri.frag.spv", &mFragmentShader));

    const VkPipelineShaderStageCreateInfo shaderStages[2] =
            {{
                     .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                     .pNext = nullptr,
                     .flags = 0,
                     .stage = VK_SHADER_STAGE_VERTEX_BIT,
                     .module = mVertexShader,
                     .pName = "main",
                     .pSpecializationInfo = nullptr,
             },
             {
                     .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                     .pNext = nullptr,
                     .flags = 0,
                     .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                     .module = mFragmentShader,
                     .pName = "main",
                     .pSpecializationInfo = nullptr,
             }};
    const VkViewport viewports = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)mSwapchainInfo->displaySize().width,
            .height = (float)mSwapchainInfo->displaySize().height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };
    const VkRect2D scissor = {
            .offset =
                    {
                            .x = 0,
                            .y = 0,
                    },
            .extent = mSwapchainInfo->displaySize(),
    };
    const VkPipelineViewportStateCreateInfo viewportInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };
    VkSampleMask sampleMask = ~0u;
    const VkPipelineMultisampleStateCreateInfo multisampleInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };
    const VkPipelineColorBlendAttachmentState attachmentStates = {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = (VkBlendFactor)0,
            .dstColorBlendFactor = (VkBlendFactor)0,
            .colorBlendOp = (VkBlendOp)0,
            .srcAlphaBlendFactor = (VkBlendFactor)0,
            .dstAlphaBlendFactor = (VkBlendFactor)0,
            .alphaBlendOp = (VkBlendOp)0,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };
    const VkPipelineRasterizationStateCreateInfo rasterInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0,
            .depthBiasClamp = 0,
            .depthBiasSlopeFactor = 0,
            .lineWidth = 1,
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE,
    };
    const VkVertexInputBindingDescription vertexInputBindingDescription = {
            .binding = 0,
            .stride = 3 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    const VkVertexInputAttributeDescription vertexInputAttributeDescription = {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
    };
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexInputBindingDescription,
            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions = &vertexInputAttributeDescription,
    };
    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = nullptr,
            .layout = mPipelineLayout,
            .renderPass = mRenderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };
    VK_CALL(vkCreateGraphicsPipelines(mDeviceInfo->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                                      nullptr, &mPipeline));

    vkDestroyShaderModule(mDeviceInfo->device(), mVertexShader, nullptr);
    vkDestroyShaderModule(mDeviceInfo->device(), mFragmentShader, nullptr);
    mVertexShader = VK_NULL_HANDLE;
    mFragmentShader = VK_NULL_HANDLE;

    return 0;
}

int32_t Renderer::init(JNIEnv* env, jobject jAssetManager) {
    mAssetManager = AAssetManager_fromJava(env, jAssetManager);
    ASSERT(mAssetManager);

    ASSERT(!createRenderPass());

    ASSERT(!createFrameBuffers());

    ASSERT(!createVertexBuffers());

    ASSERT(!createGraphicsPipeline());

    const VkCommandPoolCreateInfo commandPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = mDeviceInfo->queueFamilyIndex(),
    };
    VK_CALL(vkCreateCommandPool(mDeviceInfo->device(), &commandPoolCreateInfo, nullptr,
                                &mCommandPool));

    uint32_t swapchainLength = mSwapchainInfo->swapchainLength();
    mCommandBuffers.resize(swapchainLength, VK_NULL_HANDLE);
    const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = mCommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = swapchainLength,
    };
    VK_CALL(vkAllocateCommandBuffers(mDeviceInfo->device(), &commandBufferAllocateInfo,
                                     mCommandBuffers.data()));

    for (uint32_t i = 0; i < swapchainLength; ++i) {
        const VkCommandBufferBeginInfo commandBufferBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = 0,
                .pInheritanceInfo = nullptr,
        };
        VK_CALL(vkBeginCommandBuffer(mCommandBuffers[i], &commandBufferBeginInfo));

        const VkClearValue clearVals = {
                .color.float32[0] = 0.0f,
                .color.float32[1] = 0.0f,
                .color.float32[2] = 0.0f,
                .color.float32[3] = 1.0f,
        };
        const VkRenderPassBeginInfo renderPassBeginInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = nullptr,
                .renderPass = mRenderPass,
                .framebuffer = mFramebuffers[i],
                .renderArea =
                        {
                                .offset =
                                        {
                                                .x = 0,
                                                .y = 0,
                                        },
                                .extent = mSwapchainInfo->displaySize(),
                        },
                .clearValueCount = 1,
                .pClearValues = &clearVals,
        };
        vkCmdBeginRenderPass(mCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(mCommandBuffers[i], 0, 1, &mVertexBuffer, &offset);

        vkCmdPushConstants(mCommandBuffers[i], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           3 * sizeof(float), &fragData[0]);
        vkCmdDraw(mCommandBuffers[i], 4, 1, 0, 0);

        vkCmdPushConstants(mCommandBuffers[i], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           3 * sizeof(float), &fragData[3]);
        vkCmdDraw(mCommandBuffers[i], 4, 1, 2, 0);

        vkCmdPushConstants(mCommandBuffers[i], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           3 * sizeof(float), &fragData[6]);
        vkCmdDraw(mCommandBuffers[i], 4, 1, 6, 0);

        vkCmdPushConstants(mCommandBuffers[i], mPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           3 * sizeof(float), &fragData[9]);
        vkCmdDraw(mCommandBuffers[i], 4, 1, 8, 0);

        vkCmdEndRenderPass(mCommandBuffers[i]);

        VK_CALL(vkEndCommandBuffer(mCommandBuffers[i]));
    }

    const VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    VK_CALL(vkCreateFence(mDeviceInfo->device(), &fenceCreateInfo, nullptr, &mFence));

    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    VK_CALL(vkCreateSemaphore(mDeviceInfo->device(), &semaphoreCreateInfo, nullptr, &mSemaphore));

    return 0;
}

int32_t Renderer::drawFrame() {
    uint32_t nextIndex;
    VK_CALL(vkAcquireNextImageKHR(mDeviceInfo->device(), mSwapchainInfo->swapchain(), UINT64_MAX,
                                  mSemaphore, VK_NULL_HANDLE, &nextIndex));

    VK_CALL(vkResetFences(mDeviceInfo->device(), 1, &mFence));

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &mSemaphore,
            .pWaitDstStageMask = &waitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &mCommandBuffers[nextIndex],
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
    };
    VK_CALL(vkQueueSubmit(mDeviceInfo->queue(), 1, &submitInfo, mFence))

    VK_CALL(vkWaitForFences(mDeviceInfo->device(), 1, &mFence, VK_TRUE, 100000000));

    const VkSwapchainKHR swapchain = mSwapchainInfo->swapchain();
    const VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &nextIndex,
            .pResults = nullptr,
    };
    VK_CALL(vkQueuePresentKHR(mDeviceInfo->queue(), &presentInfo));

    return 0;
}
