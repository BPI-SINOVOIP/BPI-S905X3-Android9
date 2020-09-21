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

#include "tensorflow/contrib/lite/kernels/register.h"

#include <android/log.h>
#include <cstdio>
#include <sys/time.h>

#define LOG_TAG "NN_BENCHMARK"

BenchmarkModel::BenchmarkModel(const char* modelfile) {
    // Memory map the model. NOTE this needs lifetime greater than or equal
    // to interpreter context.
    mTfliteModel = tflite::FlatBufferModel::BuildFromFile(modelfile);
    if (!mTfliteModel) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                            "Failed to load model %s", modelfile);
        return;
    }

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder(*mTfliteModel, resolver)(&mTfliteInterpreter);
    if (!mTfliteInterpreter) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                            "Failed to create TFlite interpreter");
        return;
    }
}

BenchmarkModel::~BenchmarkModel() {
}

bool BenchmarkModel::setInput(const uint8_t* dataPtr, size_t length) {
    int input = mTfliteInterpreter->inputs()[0];
    auto* input_tensor = mTfliteInterpreter->tensor(input);
    switch (input_tensor->type) {
        case kTfLiteFloat32:
        case kTfLiteUInt8: {
            void* raw = mTfliteInterpreter->typed_tensor<void>(input);
            memcpy(raw, dataPtr, length);
            break;
        }
        default:
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                                "Input tensor type not supported");
            return false;
    }
    return true;
}

bool BenchmarkModel::resizeInputTensors(std::vector<int> shape) {
    // The benchmark only expects single input tensor, hardcoded as 0.
    int input = mTfliteInterpreter->inputs()[0];
    mTfliteInterpreter->ResizeInputTensor(input, shape);
    if (mTfliteInterpreter->AllocateTensors() != kTfLiteOk) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to allocate tensors!");
        return false;
    }
    return true;
}

bool BenchmarkModel::runBenchmark(int num_inferences,
                                  bool use_nnapi) {
    mTfliteInterpreter->UseNNAPI(use_nnapi);

    for(int i = 0; i  < num_inferences; i++){
        auto status = mTfliteInterpreter->Invoke();
        if (status != kTfLiteOk) {
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Failed to invoke: %d!", (int)status);
            return false;
        }
    }
    return true;
}