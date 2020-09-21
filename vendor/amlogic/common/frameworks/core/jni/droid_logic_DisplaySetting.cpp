/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC droid_logic_DisplaySetting
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "DisplaySetting-jni"

#include <jni.h>
#include <JNIHelp.h>

#include <utils/Log.h>
#include "utils/misc.h"
//#include <gui/SurfaceComposerClient.h>

namespace android
{
    static void nativeSetDisplaySize(JNIEnv *env, jobject obj,
        jint displayid, jint format) {
        ALOGD("nativeSetDisplaySize format is %d", format);
        //SurfaceComposerClient::openGlobalTransaction();
        //SurfaceComposerClient::setVDisplaySize(displayid,format);
        //SurfaceComposerClient::closeGlobalTransaction();
    }

    static void nativeSetDisplay2Stereoscopic(JNIEnv *env, jobject obj,
        jint displayid, jint format) {
        ALOGD("nativeSetDisplay2Stereoscopic format is %d", format);
        //SurfaceComposerClient::openGlobalTransaction();
        //SurfaceComposerClient::setDisplay2Stereoscopic(displayid,format);
        //SurfaceComposerClient::closeGlobalTransaction();
    }

    static JNINativeMethod sMethods[] = {
        {"nativeSetDisplaySize", "(II)V", (void*)nativeSetDisplaySize},
        {"nativeSetDisplay2Stereoscopic", "(II)V", (void*)nativeSetDisplay2Stereoscopic},
    };

    int register_android_DisplaySetting(JNIEnv* env) {
        jclass clazz;
        const char *kClassPathName = "com/droidlogic/app/DisplaySettingManager";

        clazz = env->FindClass(kClassPathName);
        if (clazz == NULL) {
            ALOGE("Native registration unable to find class '%s'",
                    kClassPathName);
            return JNI_FALSE;
        }

        if (env->RegisterNatives(clazz, sMethods, NELEM(sMethods)) < 0) {
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

    register_android_DisplaySetting(env);

    return JNI_VERSION_1_4;
}

