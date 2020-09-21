/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2015/04/07
 *  @par function description:
 *  - 1 transparent the video player
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "SurfaceOverlay-jni"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <jni.h>
#include <JNIHelp.h>

#include <utils/Log.h>
#include <utils/KeyedVector.h>
//#include <android_runtime/AndroidRuntime.h>
//#include <android_runtime/android_view_Surface.h>
#include <android/native_window.h>
//#include <gui/Surface.h>
//#include <gui/IGraphicBufferProducer.h>
#include <ui/GraphicBuffer.h>
#include <gralloc_usage_ext.h>

namespace android
{
    static int updateNative(sp<ANativeWindow> nativeWin) {
        char* vaddr;
        int ret = 0;
        ANativeWindowBuffer* buf;

        if (nativeWin.get() == NULL) {
            return 0;
        }
        int err = nativeWin->dequeueBuffer_DEPRECATED(nativeWin.get(), &buf);
        if (err != 0) {
            ALOGE("dequeueBuffer failed: %s (%d)", strerror(-err), -err);
            return -1;
        }
        /*
        nativeWin->lockBuffer_DEPRECATED(nativeWin.get(), buf);
        sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(buf, false));
        graphicBuffer->lock(1, (void **)&vaddr);
        if (vaddr != NULL) {
            memset(vaddr, 0x0, graphicBuffer->getWidth() * graphicBuffer->getHeight() * 4); //to show video in osd hole...
        }
        graphicBuffer->unlock();
        graphicBuffer.clear();*/

        return nativeWin->queueBuffer_DEPRECATED(nativeWin.get(), buf);
    }

    static void setSurface(JNIEnv *env, jobject thiz, jobject jsurface) {
#if 0
        sp<IGraphicBufferProducer> new_st = NULL;
        if (jsurface) {
            sp<Surface> surface(android_view_Surface_getSurface(env, jsurface));
            if (surface != NULL) {
                new_st = surface->getIGraphicBufferProducer();
                if (new_st == NULL) {
                    jniThrowException(env, "java/lang/IllegalArgumentException",
                        "The surface does not have a binding SurfaceTexture!");
                    return;
                }
            } else {
                jniThrowException(env, "java/lang/IllegalArgumentException",
                        "The surface has been released");
                return;
            }
        }

        sp<ANativeWindow> tmpWindow = NULL;
        if (new_st != NULL) {
            tmpWindow = new Surface(new_st);
            status_t err = native_window_api_connect(tmpWindow.get(),
                NATIVE_WINDOW_API_MEDIA);
            ALOGI("set native window overlay");
            native_window_set_usage(tmpWindow.get(), GRALLOC_USAGE_HW_TEXTURE |
                GRALLOC_USAGE_EXTERNAL_DISP  | GRALLOC_USAGE_AML_VIDEO_OVERLAY);
            native_window_set_buffers_format(tmpWindow.get(), WINDOW_FORMAT_RGBA_8888);

            updateNative(tmpWindow);
        }
#endif
    }

    static JNINativeMethod gMethods[] = {
        {"nativeSetSurface", "(Landroid/view/Surface;)V",
            (void *)setSurface},
    };

    int register_droidlogic_surfaceoverlay(JNIEnv* env) {
        jclass clazz;
        const char *kClassPathName = "com/droidlogic/app/SurfaceOverlay";

        clazz = env->FindClass(kClassPathName);
        if (clazz == NULL) {
            ALOGE("Native registration unable to find class '%s'",
                    kClassPathName);
            return JNI_FALSE;
        }

        if (env->RegisterNatives(clazz, gMethods, NELEM(gMethods)) < 0) {
            env->DeleteLocalRef(clazz);
            ALOGE("RegisterNatives failed for '%s'", kClassPathName);
            return JNI_FALSE;
        }

        return JNI_TRUE;
    }
} // end namespace android

using namespace android;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_droidlogic_surfaceoverlay(env);

    return JNI_VERSION_1_4;
}


