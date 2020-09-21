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

#define LOG_TAG "ASurfaceTextureTest"

#include <dlfcn.h>
#include <array>
#include <jni.h>
#include <android/native_window.h>
#include <android/surface_texture.h>
#include <android/surface_texture_jni.h>

#include "NativeTestHelpers.h"

// ------------------------------------------------------------------------------------------------

static void basicTests(JNIEnv* env, jclass, jobject surfaceTexture) {

  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_fromSurfaceTexture"),
      "couldn't dlsym ASurfaceTexture_fromSurfaceTexture");
  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_release"),
      "couldn't dlsym ASurfaceTexture_release");
  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_acquireANativeWindow"),
      "couldn't dlsym ASurfaceTexture_acquireANativeWindow");
  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_attachToGLContext"),
      "couldn't dlsym ASurfaceTexture_attachToGLContext");
  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_detachFromGLContext"),
      "couldn't dlsym ASurfaceTexture_detachFromGLContext");
  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_updateTexImage"),
      "couldn't dlsym ASurfaceTexture_updateTexImage");
  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_getTransformMatrix"),
      "couldn't dlsym ASurfaceTexture_getTransformMatrix");
  ASSERT(dlsym(RTLD_DEFAULT, "ASurfaceTexture_getTimestamp"),
      "couldn't dlsym ASurfaceTexture_getTimestamp");

  ASurfaceTexture* ast = ASurfaceTexture_fromSurfaceTexture(env, surfaceTexture);
  ASSERT(ast, "ASurfaceTexture_fromSurfaceTexture failed")

  ANativeWindow* win = ASurfaceTexture_acquireANativeWindow(ast);
  ASSERT(win, "ASurfaceTexture_acquireANativeWindow returned nullptr")

  ASSERT(!ASurfaceTexture_attachToGLContext(ast, 1234), "ASurfaceTexture_attachToGLContext failed");
  ASSERT( ASurfaceTexture_attachToGLContext(ast, 2000), "ASurfaceTexture_attachToGLContext (2nd) should have failed");

  ARect bounds = {0, 0, 640, 480};
  ANativeWindow_Buffer outBuffer;
  int err = ANativeWindow_lock(win, &outBuffer, &bounds);
  ASSERT(!err, "ANativeWindow_lock failed");
  ASSERT(outBuffer.width == 640, "locked buffer width is wrong")
  ASSERT(outBuffer.height == 480, "locked buffer height is wrong")

  // this pushes a buffer
  ANativeWindow_unlockAndPost(win);

  ASSERT(!ASurfaceTexture_updateTexImage(ast), "ASurfaceTexture_updateTexImage (1st) failed");
  ASSERT(!ASurfaceTexture_updateTexImage(ast), "ASurfaceTexture_updateTexImage (2st) failed");

  ASurfaceTexture_detachFromGLContext(ast);
  ANativeWindow_release(win);
  ASurfaceTexture_release(ast);
}

// ------------------------------------------------------------------------------------------------

static const std::array<JNINativeMethod, 1> JNI_METHODS = {{
    { "nBasicTests", "(Landroid/graphics/SurfaceTexture;)V", (void*)basicTests },
}};

int register_android_graphics_cts_ASurfaceTextureTest(JNIEnv* env) {
    jclass clazz = env->FindClass("android/graphics/cts/ASurfaceTextureTest");
    return env->RegisterNatives(clazz, JNI_METHODS.data(), JNI_METHODS.size());
}
