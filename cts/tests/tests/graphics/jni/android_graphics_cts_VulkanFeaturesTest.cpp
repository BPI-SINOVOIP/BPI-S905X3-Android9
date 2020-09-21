/*
 * Copyright 2016 The Android Open Source Project
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
#include <vkjson.h>

namespace {

jstring android_graphics_cts_VulkanFeaturesTest_nativeGetVkJSON(JNIEnv* env,
    jclass /*clazz*/)
{
    std::string vkjson(VkJsonInstanceToJson(VkJsonGetInstance()));
    return env->NewStringUTF(vkjson.c_str());
}

static JNINativeMethod gMethods[] = {
    {   "nativeGetVkJSON", "()Ljava/lang/String;",
        (void*) android_graphics_cts_VulkanFeaturesTest_nativeGetVkJSON },
};

} // anonymous namespace

int register_android_graphics_cts_VulkanFeaturesTest(JNIEnv* env) {
    jclass clazz = env->FindClass("android/graphics/cts/VulkanFeaturesTest");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
