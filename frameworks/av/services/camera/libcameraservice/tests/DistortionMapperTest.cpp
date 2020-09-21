/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "DistortionMapperTest"

#include <random>

#include <gtest/gtest.h>
#include <android-base/stringprintf.h>
#include <android-base/chrono_utils.h>

#include "../device3/DistortionMapper.h"

using namespace android;
using namespace android::camera3;


int32_t testActiveArray[] = {100, 100, 1000, 750};

float testICal[] = { 1000.f, 1000.f, 500.f, 500.f, 0.f };

float identityDistortion[] = { 0.f, 0.f, 0.f, 0.f, 0.f};

std::array<int32_t, 12> basicCoords = {
    0, 0,
    testActiveArray[2] - 1, 0,
    testActiveArray[2] - 1,  testActiveArray[3] - 1,
    0, testActiveArray[3] - 1,
    testActiveArray[2] / 2, testActiveArray[3] / 2,
    251, 403  // A particularly bad coordinate for current grid count/array size
};


void setupTestMapper(DistortionMapper *m, float distortion[5]) {
    CameraMetadata deviceInfo;

    deviceInfo.update(ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE,
            testActiveArray, 4);

    deviceInfo.update(ANDROID_LENS_INTRINSIC_CALIBRATION,
            testICal, 5);

    deviceInfo.update(ANDROID_LENS_DISTORTION,
            distortion, 5);

    m->setupStaticInfo(deviceInfo);
}

TEST(DistortionMapperTest, Initialization) {
    CameraMetadata deviceInfo;

    ASSERT_FALSE(DistortionMapper::isDistortionSupported(deviceInfo));

    uint8_t distortionModes[] =
            {ANDROID_DISTORTION_CORRECTION_MODE_OFF,
             ANDROID_DISTORTION_CORRECTION_MODE_FAST,
             ANDROID_DISTORTION_CORRECTION_MODE_HIGH_QUALITY};

    deviceInfo.update(ANDROID_DISTORTION_CORRECTION_AVAILABLE_MODES,
            distortionModes, 1);

    ASSERT_FALSE(DistortionMapper::isDistortionSupported(deviceInfo));

    deviceInfo.update(ANDROID_DISTORTION_CORRECTION_AVAILABLE_MODES,
            distortionModes, 3);

    ASSERT_TRUE(DistortionMapper::isDistortionSupported(deviceInfo));

    DistortionMapper m;

    ASSERT_FALSE(m.calibrationValid());

    ASSERT_NE(m.setupStaticInfo(deviceInfo), OK);

    ASSERT_FALSE(m.calibrationValid());

    deviceInfo.update(ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE,
            testActiveArray, 4);

    deviceInfo.update(ANDROID_LENS_INTRINSIC_CALIBRATION,
            testICal, 5);

    deviceInfo.update(ANDROID_LENS_DISTORTION,
            identityDistortion, 5);

    ASSERT_EQ(m.setupStaticInfo(deviceInfo), OK);

    ASSERT_TRUE(m.calibrationValid());

    CameraMetadata captureResult;

    ASSERT_NE(m.updateCalibration(captureResult), OK);

    captureResult.update(ANDROID_LENS_INTRINSIC_CALIBRATION,
            testICal, 5);
    captureResult.update(ANDROID_LENS_DISTORTION,
            identityDistortion, 5);

    ASSERT_EQ(m.updateCalibration(captureResult), OK);

}

TEST(DistortionMapperTest, IdentityTransform) {
    status_t res;

    DistortionMapper m;
    setupTestMapper(&m, identityDistortion);

    auto coords = basicCoords;
    res = m.mapCorrectedToRaw(coords.data(), 5);
    ASSERT_EQ(res, OK);

    for (size_t i = 0; i < coords.size(); i++) {
        EXPECT_EQ(coords[i], basicCoords[i]);
    }

    res = m.mapRawToCorrected(coords.data(), 5);
    ASSERT_EQ(res, OK);

    for (size_t i = 0; i < coords.size(); i++) {
        EXPECT_EQ(coords[i], basicCoords[i]);
    }

    std::array<int32_t, 8> rects = {
        0, 0, 100, 100,
        testActiveArray[2] - 100, testActiveArray[3]-100, 100, 100
    };

    auto rectsOrig = rects;
    res = m.mapCorrectedRectToRaw(rects.data(), 2);
    ASSERT_EQ(res, OK);

    for (size_t i = 0; i < rects.size(); i++) {
        EXPECT_EQ(rects[i], rectsOrig[i]);
    }

    res = m.mapRawRectToCorrected(rects.data(), 2);
    ASSERT_EQ(res, OK);

    for (size_t i = 0; i < rects.size(); i++) {
        EXPECT_EQ(rects[i], rectsOrig[i]);
    }
}

TEST(DistortionMapperTest, LargeTransform) {
    status_t res;
    constexpr int maxAllowedPixelError = 2; // Maximum per-pixel error allowed
    constexpr int bucketsPerPixel = 3; // Histogram granularity

    unsigned int seed = 1234; // Ensure repeatability for debugging
    const size_t coordCount = 1e6; // Number of random test points

    float bigDistortion[] = {0.1, -0.003, 0.004, 0.02, 0.01};

    DistortionMapper m;
    setupTestMapper(&m, bigDistortion);

    std::default_random_engine gen(seed);

    std::uniform_int_distribution<int> x_dist(0, testActiveArray[2] - 1);
    std::uniform_int_distribution<int> y_dist(0, testActiveArray[3] - 1);

    std::vector<int32_t> randCoords(coordCount * 2);

    for (size_t i = 0; i < randCoords.size(); i += 2) {
        randCoords[i] = x_dist(gen);
        randCoords[i + 1] = y_dist(gen);
    }

    randCoords.insert(randCoords.end(), basicCoords.begin(), basicCoords.end());

    auto origCoords = randCoords;

    base::Timer correctedToRawTimer;
    res = m.mapCorrectedToRaw(randCoords.data(), randCoords.size() / 2);
    auto correctedToRawDurationMs = correctedToRawTimer.duration();
    EXPECT_EQ(res, OK);

    base::Timer rawToCorrectedTimer;
    res = m.mapRawToCorrected(randCoords.data(), randCoords.size() / 2);
    auto rawToCorrectedDurationMs = rawToCorrectedTimer.duration();
    EXPECT_EQ(res, OK);

    float correctedToRawDurationPerCoordUs =
            (std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
                correctedToRawDurationMs) / (randCoords.size() / 2) ).count();
    float rawToCorrectedDurationPerCoordUs =
            (std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
                rawToCorrectedDurationMs) / (randCoords.size() / 2) ).count();

    RecordProperty("CorrectedToRawDurationPerCoordUs",
            base::StringPrintf("%f", correctedToRawDurationPerCoordUs));
    RecordProperty("RawToCorrectedDurationPerCoordUs",
            base::StringPrintf("%f", rawToCorrectedDurationPerCoordUs));

    // Calculate mapping errors after round trip
    float totalErrorSq = 0;
    // Basic histogram; buckets go from [N to N+1)
    std::array<int, maxAllowedPixelError * bucketsPerPixel> histogram = {0};
    int outOfHistogram = 0;

    for (size_t i = 0; i < randCoords.size(); i += 2) {
        int xOrig = origCoords[i];
        int yOrig = origCoords[i + 1];
        int xMapped = randCoords[i];
        int yMapped = randCoords[i + 1];

        float errorSq = (xMapped - xOrig) * (xMapped - xOrig) +
                (yMapped - yOrig) * (yMapped - yOrig);

        EXPECT_LE(errorSq, maxAllowedPixelError * maxAllowedPixelError) << "( " <<
                xOrig << "," << yOrig << ") -> (" << xMapped << "," << yMapped << ")";

        // Note: Integer coordinates, so histogram will be clumpy; error distances can only be of
        // form sqrt(X^2+Y^2) where X, Y are integers, so:
        //    0, 1, sqrt(2), 2, sqrt(5), sqrt(8), 3, sqrt(10), sqrt(13), 4 ...
        totalErrorSq += errorSq;
        float errorDist = std::sqrt(errorSq);
        if (errorDist < maxAllowedPixelError) {
            int histBucket = static_cast<int>(errorDist * bucketsPerPixel); // rounds down
            histogram[histBucket]++;
        } else {
            outOfHistogram++;
        }
    }

    float rmsError = std::sqrt(totalErrorSq / randCoords.size());
    RecordProperty("RmsError", base::StringPrintf("%f", rmsError));
    for (size_t i = 0; i < histogram.size(); i++) {
        std::string label = base::StringPrintf("HistogramBin[%f,%f)",
                (float)i/bucketsPerPixel, (float)(i + 1)/bucketsPerPixel);
        RecordProperty(label, histogram[i]);
    }
    RecordProperty("HistogramOutOfRange", outOfHistogram);
}

// Compare against values calculated by OpenCV
// undistortPoints() method, which is the same as mapRawToCorrected
// See script DistortionMapperComp.py
#include "DistortionMapperTest_OpenCvData.h"

TEST(DistortionMapperTest, CompareToOpenCV) {
    status_t res;

    float bigDistortion[] = {0.1, -0.003, 0.004, 0.02, 0.01};

    // Expect to match within sqrt(2) radius pixels
    const int32_t maxSqError = 2;

    DistortionMapper m;
    setupTestMapper(&m, bigDistortion);

    using namespace openCvData;

    res = m.mapRawToCorrected(rawCoords.data(), rawCoords.size() / 2);

    for (size_t i = 0; i < rawCoords.size(); i+=2) {
        int32_t dist = (rawCoords[i] - expCoords[i]) * (rawCoords[i] - expCoords[i]) +
               (rawCoords[i + 1] - expCoords[i + 1]) * (rawCoords[i + 1] - expCoords[i + 1]);
        EXPECT_LE(dist, maxSqError)
                << "(" << rawCoords[i] << ", " << rawCoords[i + 1] << ") != ("
                << expCoords[i] << ", " << expCoords[i + 1] << ")";
    }
}
