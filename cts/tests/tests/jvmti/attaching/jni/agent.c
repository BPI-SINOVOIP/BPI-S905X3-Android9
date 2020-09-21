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

#include <jni.h>

#include <string.h>
#include <stdbool.h>

static bool sIsAttached = false;

jint
Agent_OnAttach(JavaVM* vm, char* options, void* reserved) {
    if (options != NULL && options[0] == 'a') {
        sIsAttached = true;
        return JNI_OK;
    } else {
        return JNI_ERR;
    }
}

#ifndef AGENT_NR
#error "Missing AGENT_NR"
#endif
#define CONCAT(A,B) A ## B
#define EVAL(A,B) CONCAT(A,B)
#define NAME(BASE) EVAL(BASE,AGENT_NR)

JNIEXPORT jboolean JNICALL NAME(Java_android_jvmti_attaching_cts_AttachingTest_isAttached) (
        JNIEnv* env, jclass klass) {
    if (sIsAttached) {
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}
