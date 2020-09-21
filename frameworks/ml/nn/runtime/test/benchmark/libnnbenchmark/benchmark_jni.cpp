/**
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
 */

#include "run_tflite.h"

#include <jni.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <fcntl.h>

#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/sharedmem.h>
#include <sys/mman.h>

extern "C"
JNIEXPORT jlong
JNICALL
Java_com_example_android_nn_benchmark_NNTestBase_initModel(
        JNIEnv *env,
        jobject /* this */,
        jstring _modelFileName) {
    const char *modelFileName = env->GetStringUTFChars(_modelFileName, NULL);
    void* handle = new BenchmarkModel(modelFileName);
    env->ReleaseStringUTFChars(_modelFileName, modelFileName);

    return (jlong)(uintptr_t)handle;
}

extern "C"
JNIEXPORT void
JNICALL
Java_com_example_android_nn_benchmark_NNTestBase_destroyModel(
        JNIEnv *env,
        jobject /* this */,
        jlong _modelHandle) {
    BenchmarkModel* model = (BenchmarkModel *) _modelHandle;
    delete(model);
}

extern "C"
JNIEXPORT jboolean
JNICALL
Java_com_example_android_nn_benchmark_NNTestBase_resizeInputTensors(
        JNIEnv *env,
        jobject /* this */,
        jlong _modelHandle,
        jintArray _inputShape) {
    BenchmarkModel* model = (BenchmarkModel *) _modelHandle;
    jint* shapePtr = env->GetIntArrayElements(_inputShape, nullptr);
    jsize shapeLen = env->GetArrayLength(_inputShape);

    std::vector<int> shape(shapePtr, shapePtr + shapeLen);
    return model->resizeInputTensors(std::move(shape));
}

extern "C"
JNIEXPORT jboolean
JNICALL
Java_com_example_android_nn_benchmark_NNTestBase_runBenchmark(
        JNIEnv *env,
        jobject /* this */,
        jlong _modelHandle,
        jboolean _useNNAPI) {
    BenchmarkModel* model = (BenchmarkModel *) _modelHandle;
    return model->runBenchmark(1, _useNNAPI);
}