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

#include "NeuralNetworksWrapper.h"

#ifndef NNTEST_ONLY_PUBLIC_API
#include "Manager.h"
#include "Utils.h"
#endif

#include <gtest/gtest.h>

using namespace android::nn::wrapper;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

#ifndef NNTEST_ONLY_PUBLIC_API
    android::nn::initVLogMask();
#endif
    // Test with the installed drivers.
    int n1 = RUN_ALL_TESTS();
#ifdef NNTEST_ONLY_PUBLIC_API
    // Can't use non-public functionality, because we're linking against
    // the shared library version of the runtime.
    return n1;
#else
    // Test with the CPU driver only.
    android::nn::DeviceManager::get()->setUseCpuOnly(true);
    int n2 = RUN_ALL_TESTS();
    return n1 | n2;
#endif // NNTEST_ONLY_PUBLIC_API
}
