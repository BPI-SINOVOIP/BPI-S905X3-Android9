/*
 * Copyright 2017 The Android Open Source Project
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

#define LOG_TAG "RootlessGpuDebug"

#include <android/log.h>
#include <jni.h>
#include <string>
#include <vulkan/vulkan.h>

#define ALOGI(msg, ...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, (msg), __VA_ARGS__)
#define ALOGE(msg, ...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, (msg), __VA_ARGS__)

namespace {

std::string initVulkan()
{
    std::string result = "";

    const VkApplicationInfo app_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,            // pNext
        "RootlessGpuDebug", // app name
        0,                  // app version
        nullptr,            // engine name
        0,                  // engine version
        VK_API_VERSION_1_0,
    };
    const VkInstanceCreateInfo instance_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,   // pNext
        0,         // flags
        &app_info,
        0,         // layer count
        nullptr,   // layers
        0,         // extension count
        nullptr,   // extensions
    };
    VkInstance instance;
    VkResult vkResult = vkCreateInstance(&instance_info, nullptr, &instance);
    if (vkResult == VK_ERROR_INITIALIZATION_FAILED) {
        result = "vkCreateInstance failed, meaning layers could not be chained.";
    } else {
        result = "vkCreateInstance succeeded.";
    }

    return result;
}

jstring android_gputools_cts_RootlessGpuDebug_nativeInitVulkan(JNIEnv* env,
    jclass /*clazz*/)
{
    std::string result;

    result = initVulkan();

    return env->NewStringUTF(result.c_str());
}

static JNINativeMethod gMethods[] = {
    {    "nativeInitVulkan", "()Ljava/lang/String;",
         (void*) android_gputools_cts_RootlessGpuDebug_nativeInitVulkan },
};

} // anonymous namespace

int register_android_gputools_cts_RootlessGpuDebug(JNIEnv* env) {
    jclass clazz = env->FindClass("android/rootlessgpudebug/app/RootlessGpuDebugDeviceActivity");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
