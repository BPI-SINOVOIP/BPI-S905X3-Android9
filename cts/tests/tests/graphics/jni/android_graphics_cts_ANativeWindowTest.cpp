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

#define LOG_TAG "ANativeWindowTest"

#include <array>

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

namespace {

void pushBufferWithTransform(JNIEnv* env, jclass, jobject jSurface, jint transform) {
    auto window = ANativeWindow_fromSurface(env, jSurface);
    ANativeWindow_setBuffersTransform(window, transform);
    ANativeWindow_Buffer mappedBuffer;
    ANativeWindow_lock(window, &mappedBuffer, nullptr);
    ANativeWindow_unlockAndPost(window);
    ANativeWindow_release(window);
}

jint setBuffersDataSpace(JNIEnv* env, jclass, jobject jSurface, jint dataSpace) {
    ANativeWindow* window = nullptr;
    if (jSurface) {
        window = ANativeWindow_fromSurface(env, jSurface);
    }
    int error = ANativeWindow_setBuffersDataSpace(window, dataSpace);
    if (error != 0) {
        return error;
    }
    ANativeWindow_Buffer mappedBuffer;
    ANativeWindow_lock(window, &mappedBuffer, nullptr);
    ANativeWindow_unlockAndPost(window);
    ANativeWindow_release(window);
    return error;
}

jint getBuffersDataSpace(JNIEnv* env, jclass, jobject jSurface) {
    ANativeWindow* window = nullptr;
    if (jSurface) {
        window = ANativeWindow_fromSurface(env, jSurface);
    }
    return ANativeWindow_getBuffersDataSpace(window);
}

const std::array<JNINativeMethod, 3> JNI_METHODS = {{
    { "nPushBufferWithTransform", "(Landroid/view/Surface;I)V", (void*)pushBufferWithTransform },
    { "nSetBuffersDataSpace", "(Landroid/view/Surface;I)I", (void*)setBuffersDataSpace },
    { "nGetBuffersDataSpace", "(Landroid/view/Surface;)I", (void*)getBuffersDataSpace },
}};

}

int register_android_graphics_cts_ANativeWindowTest(JNIEnv* env) {
    jclass clazz = env->FindClass("android/graphics/cts/ANativeWindowTest");
    return env->RegisterNatives(clazz, JNI_METHODS.data(), JNI_METHODS.size());
}
