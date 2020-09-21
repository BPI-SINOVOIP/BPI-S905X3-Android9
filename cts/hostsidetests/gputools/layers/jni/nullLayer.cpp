/*
 * Copyright (C) 2017 The Android Open Source Project
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
 */

#include <android/log.h>
#include <cstring>
#include <vulkan/vulkan.h>
#include "vk_layer_interface.h"

#define xstr(a) str(a)
#define str(a) #a

#define LOG_TAG "nullLayer" xstr(LAYERNAME)

#define ALOGI(msg, ...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, (msg), __VA_ARGS__)


// Announce if anything loads this layer.  LAYERNAME is defined in Android.mk
class StaticLogMessage {
    public:
        StaticLogMessage(const char* msg) {
            ALOGI("%s", msg);
    }
};
StaticLogMessage g_initMessage("nullLayer" xstr(LAYERNAME) " loaded");


namespace {


// Minimal dispatch table for this simple layer
struct {
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
} g_VulkanDispatchTable;


template<class T>
VkResult getProperties(const uint32_t count, const T *properties, uint32_t *pCount,
                            T *pProperties) {
    uint32_t copySize;

    if (pProperties == NULL || properties == NULL) {
        *pCount = count;
        return VK_SUCCESS;
    }

    copySize = *pCount < count ? *pCount : count;
    memcpy(pProperties, properties, copySize * sizeof(T));
    *pCount = copySize;
    if (copySize < count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

static const VkLayerProperties LAYER_PROPERTIES = {
    "VK_LAYER_ANDROID_nullLayer" xstr(LAYERNAME), VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION), 1, "Layer: nullLayer" xstr(LAYERNAME),
};

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return getProperties<VkLayerProperties>(1, &LAYER_PROPERTIES, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice /* physicalDevice */, uint32_t *pCount,
                                                              VkLayerProperties *pProperties) {
    return getProperties<VkLayerProperties>(0, NULL, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char* /* pLayerName */, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    return getProperties<VkExtensionProperties>(0, NULL, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice /* physicalDevice */, const char* /* pLayerName */,
                                                                  uint32_t *pCount, VkExtensionProperties *pProperties) {
    return getProperties<VkExtensionProperties>(0, NULL, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL nullCreateInstance(const VkInstanceCreateInfo* pCreateInfo,
                                                  const VkAllocationCallbacks* pAllocator,
                                                  VkInstance* pInstance) {

    VkLayerInstanceCreateInfo *layerCreateInfo = (VkLayerInstanceCreateInfo *)pCreateInfo->pNext;

    const char* msg = "nullCreateInstance called in nullLayer" xstr(LAYERNAME);
    ALOGI("%s", msg);

    // Step through the pNext chain until we get to the link function
    while(layerCreateInfo && (layerCreateInfo->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO ||
                              layerCreateInfo->function != VK_LAYER_FUNCTION_LINK)) {

      layerCreateInfo = (VkLayerInstanceCreateInfo *)layerCreateInfo->pNext;
    }

    if(layerCreateInfo == NULL)
      return VK_ERROR_INITIALIZATION_FAILED;

    // Grab GIPA for the next layer
    PFN_vkGetInstanceProcAddr gpa = layerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;

    // Track is in our dispatch table
    g_VulkanDispatchTable.GetInstanceProcAddr = gpa;

    // Advance the chain for next layer
    layerCreateInfo->u.pLayerInfo = layerCreateInfo->u.pLayerInfo->pNext;

    // Call the next layer
    PFN_vkCreateInstance createFunc = (PFN_vkCreateInstance)gpa(VK_NULL_HANDLE, "vkCreateInstance");
    VkResult ret = createFunc(pCreateInfo, pAllocator, pInstance);

    return ret;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice /* dev */, const char* /* funcName */) {
    return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char* funcName) {

    // Our simple layer only intercepts vkCreateInstance
    const char* targetFunc = "vkCreateInstance";
    if (!strncmp(targetFunc, funcName, sizeof(&targetFunc)))
        return (PFN_vkVoidFunction)nullCreateInstance;

    return (PFN_vkVoidFunction)g_VulkanDispatchTable.GetInstanceProcAddr(instance, funcName);
}

}  // namespace

// loader-layer interface v0, just wrappers since there is only a layer

__attribute((visibility("default"))) VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                  VkLayerProperties *pProperties) {
    return EnumerateInstanceLayerProperties(pCount, pProperties);
}

__attribute((visibility("default"))) VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                VkLayerProperties *pProperties) {
    return EnumerateDeviceLayerProperties(physicalDevice, pCount, pProperties);
}

__attribute((visibility("default"))) VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                      VkExtensionProperties *pProperties) {
    return EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

__attribute((visibility("default"))) VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                    const char *pLayerName, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    return EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pCount, pProperties);
}

__attribute((visibility("default"))) VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return GetDeviceProcAddr(dev, funcName);
}

__attribute((visibility("default"))) VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return GetInstanceProcAddr(instance, funcName);
}
