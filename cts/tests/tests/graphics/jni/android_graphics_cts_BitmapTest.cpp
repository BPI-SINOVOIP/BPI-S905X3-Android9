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

#define LOG_TAG "BitmapTest"

#include <jni.h>
#include <android/bitmap.h>

#include "NativeTestHelpers.h"

static void validateBitmapInfo(JNIEnv* env, jclass, jobject jbitmap, jint width, jint height,
        jboolean is565) {
    AndroidBitmapInfo info;
    int err = 0;
    err = AndroidBitmap_getInfo(env, jbitmap, &info);
    ASSERT_EQ(ANDROID_BITMAP_RESULT_SUCCESS, err);
    ASSERT_TRUE(width >= 0 && height >= 0);
    ASSERT_EQ((uint32_t) width, info.width);
    ASSERT_EQ((uint32_t) height, info.height);
    int32_t format = is565 ? ANDROID_BITMAP_FORMAT_RGB_565 : ANDROID_BITMAP_FORMAT_RGBA_8888;
    ASSERT_EQ(format, info.format);
}

static void validateNdkAccessAfterRecycle(JNIEnv* env, jclass, jobject jbitmap) {
    void* pixels = nullptr;
    int err = AndroidBitmap_lockPixels(env, jbitmap, &pixels);
    ASSERT_EQ(err, ANDROID_BITMAP_RESULT_JNI_EXCEPTION);
}

static JNINativeMethod gMethods[] = {
    { "nValidateBitmapInfo", "(Landroid/graphics/Bitmap;IIZ)V",
        (void*) validateBitmapInfo },
    { "nValidateNdkAccessAfterRecycle", "(Landroid/graphics/Bitmap;)V",
        (void*) validateNdkAccessAfterRecycle },
};

int register_android_graphics_cts_BitmapTest(JNIEnv* env) {
    jclass clazz = env->FindClass("android/graphics/cts/BitmapTest");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
