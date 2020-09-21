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

/* Four simple example tests that exerise the Celcius->Farenheit and
 * Farenheit->Celcius native-code conversion functions using GTest. */
namespace TFNativeTestSampleLibTests {

TEST(CelciusToFarenheitTest, testNegative) {
    ASSERT_EQ(41, TFNativeTestSampleLib::c_to_f(5));
}

TEST(CelciusToFarenheitTest, testPositive) {
    ASSERT_EQ(5, TFNativeTestSampleLib::c_to_f(-15));
}

TEST(FarenheitToCelciusTest, testExactFail) {
    ASSERT_EQ(11.66, TFNativeTestSampleLib::f_to_c(53));
}

TEST(FarenheitToCelciusTest, testApproximatePass) {
    ASSERT_NEAR(11.66, TFNativeTestSampleLib::f_to_c(53), (double).01);
}


}
