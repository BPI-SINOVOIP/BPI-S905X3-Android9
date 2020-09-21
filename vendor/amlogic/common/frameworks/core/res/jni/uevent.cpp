/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define LOG_TAG "UEventObserver"
//#define LOG_NDEBUG 0

#include "jni.h"

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <utils/String8.h>
#include <cutils/uevent.h>

namespace android {

static Mutex gMatchesMutex;
static Vector<String8> gMatches;
static int gFd;

static void nativeSetup(JNIEnv *env, jclass clazz) {
    gFd = uevent_open_socket(64*1024, true);
    if (gFd < 0) {
        ALOGE("nativeSetup error");
    }
}

static bool isMatch(const char* buffer, size_t length) {
    AutoMutex _l(gMatchesMutex);

    for (size_t i = 0; i < gMatches.size(); i++) {
        const String8& match = gMatches.itemAt(i);

        // Consider all zero-delimited fields of the buffer.
        const char* field = buffer;
        const char* end = buffer + length + 1;
        do {
            if (strstr(field, match.string())) {
                //ALOGD("Matched uevent message with pattern: %s", match.string());
                return true;
            }
            field += strlen(field) + 1;
        } while (field != end);
    }
    return false;
}

static jstring nativeWaitForNextEvent(JNIEnv *env, jclass clazz) {
    char buffer[1024];

    for (;;) {
        int length = uevent_kernel_multicast_recv(gFd, buffer, 1022);
        if (length <= 0) {
            return NULL;
        }
        buffer[length] = '\0';

        if (isMatch(buffer, length)) {
            // Assume the message is ASCII.
            jchar message[length];
            for (int i = 0; i < length; i++) {
                message[i] = buffer[i];
            }
            return env->NewString(message, length);
        }
    }
}

static void nativeAddMatch(JNIEnv* env, jclass clazz, jstring matchStr) {
    const char* match = env->GetStringUTFChars(matchStr, NULL);

    AutoMutex _l(gMatchesMutex);
    gMatches.add(String8(match));
}

static void nativeRemoveMatch(JNIEnv* env, jclass clazz, jstring matchStr) {
    const char* match = env->GetStringUTFChars(matchStr, NULL);

    AutoMutex _l(gMatchesMutex);
    for (size_t i = 0; i < gMatches.size(); i++) {
        if (gMatches.itemAt(i) == match) {
            gMatches.removeAt(i);
            break; // only remove first occurrence
        }
    }
}

static const JNINativeMethod gMethods[] = {
    { "nativeSetup", "()V",
            (void *)nativeSetup },
    { "nativeWaitForNextEvent", "()Ljava/lang/String;",
            (void *)nativeWaitForNextEvent },
    { "nativeAddMatch", "(Ljava/lang/String;)V",
            (void *)nativeAddMatch },
    { "nativeRemoveMatch", "(Ljava/lang/String;)V",
            (void *)nativeRemoveMatch },
};

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

int register_droidlogic_ueventobserver(JNIEnv* env) {
    jclass clazz;
    const char *kClassPathName = "com/droidlogic/UEventObserver";

    clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        ALOGE("unable to find class '%s'", kClassPathName);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, gMethods, NELEM(gMethods)) < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_droidlogic_ueventobserver(env);

    return JNI_VERSION_1_4;
}

}   // namespace android
