/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include "TradeFedNativeTestSampleLib.h"
#include <gtest/gtest.h>

namespace TFNativeTestSampleLibTests {

TEST(FibonacciTest, testRecursive_One) {
    ASSERT_EQ((unsigned long)1, TFNativeTestSampleLib::fibonacci_recursive(1));
}

TEST(FibonacciTest, testRecursive_Ten) {
    ASSERT_EQ((unsigned long)55, TFNativeTestSampleLib::fibonacci_recursive(10));
}

TEST(FibonacciTest, testIterative_Ten) {
    ASSERT_EQ((unsigned long)55, TFNativeTestSampleLib::fibonacci_iterative(10));
}

}


