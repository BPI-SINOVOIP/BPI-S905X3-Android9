/*
 * Copyright (C) 2015 The Android Open Source Project
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
#include "sles.h"
#include "jni_sles.h"
#include <stdio.h>
#include <stddef.h>

/////
JNIEXPORT jlong JNICALL Java_com_android_cts_verifier_audio_NativeAudioThread_slesInit
  (JNIEnv *env __unused, jobject obj __unused, jint samplingRate, jint frameCount,
   jint micSource, jint numFramesToIgnore) {

    sles_data * pSles = NULL;

    if (slesInit(&pSles, samplingRate, frameCount, micSource, numFramesToIgnore) != SLES_FAIL) {

        return (long)pSles;
    }
    // FIXME This should be stored as a (long) field in the object,
    //       so that incorrect Java code could not synthesize a bad sles pointer.
    return 0;
}

JNIEXPORT jint JNICALL Java_com_android_cts_verifier_audio_NativeAudioThread_slesProcessNext
  (JNIEnv *env __unused, jobject obj __unused, jlong sles, jdoubleArray samplesArray,
          jlong offset) {
    sles_data * pSles= (sles_data*) ((long)sles);

    long maxSamples = (*env)->GetArrayLength(env, samplesArray);
    double *pSamples = (*env)->GetDoubleArrayElements(env, samplesArray,0);

    long availableSamples = maxSamples-offset;
    double *pCurrentSample = pSamples+offset;

    SLES_PRINTF("jni slesProcessNext pSles:%p, currentSample %p, availableSamples %ld ", pSles,
            pCurrentSample, availableSamples);

    int samplesRead = slesProcessNext(pSles, pCurrentSample, availableSamples);

    return samplesRead;
}

JNIEXPORT jint JNICALL Java_com_android_cts_verifier_audio_NativeAudioThread_slesDestroy
  (JNIEnv *env __unused, jobject obj __unused, jlong sles) {
    sles_data * pSles= (sles_data*) ((long) sles);

    int status = slesDestroy(&pSles);

    return status;
}
