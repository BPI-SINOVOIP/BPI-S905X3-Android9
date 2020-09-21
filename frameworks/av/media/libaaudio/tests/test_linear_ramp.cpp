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

#include <iostream>
#include <math.h>

#include <gtest/gtest.h>

#include "utility/AAudioUtilities.h"
#include "utility/LinearRamp.h"

TEST(test_linear_ramp, linear_ramp_segments) {
    LinearRamp ramp;
    const float source[4] = {1.0f, 1.0f, 1.0f, 1.0f };
    float destination[4] = {1.0f, 1.0f, 1.0f, 1.0f };

    float levelFrom = -1.0f;
    float levelTo = -1.0f;
    ramp.setLengthInFrames(8);
    ramp.setTarget(8.0f);

    EXPECT_EQ(8, ramp.getLengthInFrames());

    bool ramping = ramp.nextSegment(4, &levelFrom, &levelTo);
    EXPECT_EQ(1, ramping);
    EXPECT_EQ(0.0f, levelFrom);
    EXPECT_EQ(4.0f, levelTo);

    AAudio_linearRamp(source, destination, 4, 1, levelFrom, levelTo);
    EXPECT_EQ(0.0f, destination[0]);
    EXPECT_EQ(1.0f, destination[1]);
    EXPECT_EQ(2.0f, destination[2]);
    EXPECT_EQ(3.0f, destination[3]);

    ramping = ramp.nextSegment(4, &levelFrom, &levelTo);
    EXPECT_EQ(1, ramping);
    EXPECT_EQ(4.0f, levelFrom);
    EXPECT_EQ(8.0f, levelTo);

    AAudio_linearRamp(source, destination, 4, 1, levelFrom, levelTo);
    EXPECT_EQ(4.0f, destination[0]);
    EXPECT_EQ(5.0f, destination[1]);
    EXPECT_EQ(6.0f, destination[2]);
    EXPECT_EQ(7.0f, destination[3]);

    ramping = ramp.nextSegment(4, &levelFrom, &levelTo);
    EXPECT_EQ(0, ramping);
    EXPECT_EQ(8.0f, levelFrom);
    EXPECT_EQ(8.0f, levelTo);

    AAudio_linearRamp(source, destination, 4, 1, levelFrom, levelTo);
    EXPECT_EQ(8.0f, destination[0]);
    EXPECT_EQ(8.0f, destination[1]);
    EXPECT_EQ(8.0f, destination[2]);
    EXPECT_EQ(8.0f, destination[3]);

};


TEST(test_linear_ramp, linear_ramp_forced) {
    LinearRamp ramp;
    const float source[4] = {1.0f, 1.0f, 1.0f, 1.0f };
    float destination[4] = {1.0f, 1.0f, 1.0f, 1.0f };

    float levelFrom = -1.0f;
    float levelTo = -1.0f;
    ramp.setLengthInFrames(4);
    ramp.setTarget(8.0f);
    ramp.forceCurrent(4.0f);
    EXPECT_EQ(4.0f, ramp.getCurrent());

    bool ramping = ramp.nextSegment(4, &levelFrom, &levelTo);
    EXPECT_EQ(1, ramping);
    EXPECT_EQ(4.0f, levelFrom);
    EXPECT_EQ(8.0f, levelTo);

    AAudio_linearRamp(source, destination, 4, 1, levelFrom, levelTo);
    EXPECT_EQ(4.0f, destination[0]);
    EXPECT_EQ(5.0f, destination[1]);
    EXPECT_EQ(6.0f, destination[2]);
    EXPECT_EQ(7.0f, destination[3]);

    ramping = ramp.nextSegment(4, &levelFrom, &levelTo);
    EXPECT_EQ(0, ramping);
    EXPECT_EQ(8.0f, levelFrom);
    EXPECT_EQ(8.0f, levelTo);

    AAudio_linearRamp(source, destination, 4, 1, levelFrom, levelTo);
    EXPECT_EQ(8.0f, destination[0]);
    EXPECT_EQ(8.0f, destination[1]);
    EXPECT_EQ(8.0f, destination[2]);
    EXPECT_EQ(8.0f, destination[3]);

};

constexpr int16_t kMaxI16 = INT16_MAX;
constexpr int16_t kMinI16 = INT16_MIN;
constexpr int16_t kHalfI16 = 16384;
constexpr int16_t kTenthI16 = 3277;

//void AAudioConvert_floatToPcm16(const float *source,
//                                int16_t *destination,
//                                int32_t numSamples,
//                                float amplitude);
TEST(test_linear_ramp, float_to_i16) {
    const float source[] = {12345.6f, 1.0f, 0.5f, 0.1f, 0.0f, -0.1f, -0.5f, -1.0f, -12345.6f};
    constexpr size_t count = sizeof(source) / sizeof(source[0]);
    int16_t destination[count];
    const int16_t expected[count] = {kMaxI16, kMaxI16, kHalfI16, kTenthI16, 0,
                                     -kTenthI16, -kHalfI16, kMinI16, kMinI16};

    AAudioConvert_floatToPcm16(source, destination, count, 1.0f);
    for (size_t i = 0; i < count; i++) {
        EXPECT_EQ(expected[i], destination[i]);
    }

}

//void AAudioConvert_pcm16ToFloat(const int16_t *source,
//                                float *destination,
//                                int32_t numSamples,
//                                float amplitude);
TEST(test_linear_ramp, i16_to_float) {
    const int16_t source[] = {kMaxI16, kHalfI16, kTenthI16, 0,
                              -kTenthI16, -kHalfI16, kMinI16};
    constexpr size_t count = sizeof(source) / sizeof(source[0]);
    float destination[count];
    const float expected[count] = {(32767.0f / 32768.0f), 0.5f, 0.1f, 0.0f, -0.1f, -0.5f, -1.0f};

    AAudioConvert_pcm16ToFloat(source, destination, count, 1.0f);
    for (size_t i = 0; i < count; i++) {
        EXPECT_NEAR(expected[i], destination[i], 0.0001f);
    }

}

//void AAudio_linearRamp(const int16_t *source,
//                       int16_t *destination,
//                       int32_t numFrames,
//                       int32_t samplesPerFrame,
//                       float amplitude1,
//                       float amplitude2);
TEST(test_linear_ramp, ramp_i16_to_i16) {
    const int16_t source[] = {1, 1, 1, 1, 1, 1, 1, 1};
    constexpr size_t count = sizeof(source) / sizeof(source[0]);
    int16_t destination[count];
    // Ramp will sweep from -1 to almost +1
    const int16_t expected[count] = {
            -1, // from -1.00
            -1, // from -0.75
            -1, // from -0.55, round away from zero
            0,  // from -0.25, round up to zero
            0,  // from  0.00
            0,  // from  0.25, round down to zero
            1,  // from  0.50, round away from zero
            1   // from  0.75
    };

    // sweep across zero to test symmetry
    constexpr float amplitude1 = -1.0;
    constexpr float amplitude2 = 1.0;
    AAudio_linearRamp(source, destination, count, 1, amplitude1, amplitude2);
    for (size_t i = 0; i < count; i++) {
        EXPECT_EQ(expected[i], destination[i]);
    }

}
