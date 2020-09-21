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

#include <iostream>

#include <gtest/gtest.h>
#include <stdlib.h>

#include "fifo/FifoBuffer.h"
#include "fifo/FifoController.h"

using android::fifo_frames_t;
using android::FifoController;
using android::FifoBuffer;
using android::WrappingBuffer;

//void foo() {
TEST(test_fifi_controller, fifo_indices) {
    // Values are arbitrary primes designed to trigger edge cases.
    constexpr int capacity = 83;
    constexpr int threshold = 47;
    FifoController   fifoController(capacity, threshold);
    ASSERT_EQ(capacity, fifoController.getCapacity());
    ASSERT_EQ(threshold, fifoController.getThreshold());

    ASSERT_EQ(0, fifoController.getReadCounter());
    ASSERT_EQ(0, fifoController.getWriteCounter());
    ASSERT_EQ(0, fifoController.getFullFramesAvailable());
    ASSERT_EQ(threshold, fifoController.getEmptyFramesAvailable());

    // Pretend to write some data.
    constexpr int advance1 = 23;
    fifoController.advanceWriteIndex(advance1);
    int advanced = advance1;
    ASSERT_EQ(0, fifoController.getReadCounter());
    ASSERT_EQ(0, fifoController.getReadIndex());
    ASSERT_EQ(advanced, fifoController.getWriteCounter());
    ASSERT_EQ(advanced, fifoController.getWriteIndex());
    ASSERT_EQ(advanced, fifoController.getFullFramesAvailable());
    ASSERT_EQ(threshold - advanced, fifoController.getEmptyFramesAvailable());

    // Pretend to read the data.
    fifoController.advanceReadIndex(advance1);
    ASSERT_EQ(advanced, fifoController.getReadCounter());
    ASSERT_EQ(advanced, fifoController.getReadIndex());
    ASSERT_EQ(advanced, fifoController.getWriteCounter());
    ASSERT_EQ(advanced, fifoController.getWriteIndex());
    ASSERT_EQ(0, fifoController.getFullFramesAvailable());
    ASSERT_EQ(threshold, fifoController.getEmptyFramesAvailable());

    // Write past end of buffer.
    constexpr int advance2 = 13 + capacity - advance1;
    fifoController.advanceWriteIndex(advance2);
    advanced += advance2;
    ASSERT_EQ(advance1, fifoController.getReadCounter());
    ASSERT_EQ(advance1, fifoController.getReadIndex());
    ASSERT_EQ(advanced, fifoController.getWriteCounter());
    ASSERT_EQ(advanced - capacity, fifoController.getWriteIndex());
    ASSERT_EQ(advance2, fifoController.getFullFramesAvailable());
    ASSERT_EQ(threshold - advance2, fifoController.getEmptyFramesAvailable());
}

// TODO consider using a template for other data types.
class TestFifoBuffer {
public:
    explicit TestFifoBuffer(fifo_frames_t capacity, fifo_frames_t threshold = 0)
        : mFifoBuffer(sizeof(int16_t), capacity) {
        // For reading and writing.
        mData = new int16_t[capacity];
        if (threshold <= 0) {
            threshold = capacity;
        }
        mFifoBuffer.setThreshold(threshold);
        mThreshold = threshold;
    }

    void checkMisc() {
        ASSERT_EQ((int32_t)(2 * sizeof(int16_t)), mFifoBuffer.convertFramesToBytes(2));
        ASSERT_EQ(mThreshold, mFifoBuffer.getThreshold());
    }

    // Verify that the available frames in each part add up correctly.
    void checkWrappingBuffer() {
        WrappingBuffer wrappingBuffer;
        fifo_frames_t framesAvailable =
                mFifoBuffer.getFifoControllerBase()->getEmptyFramesAvailable();
        fifo_frames_t wrapAvailable = mFifoBuffer.getEmptyRoomAvailable(&wrappingBuffer);
        EXPECT_EQ(framesAvailable, wrapAvailable);
        fifo_frames_t bothAvailable = wrappingBuffer.numFrames[0] + wrappingBuffer.numFrames[1];
        EXPECT_EQ(framesAvailable, bothAvailable);

        framesAvailable =
                mFifoBuffer.getFifoControllerBase()->getFullFramesAvailable();
        wrapAvailable = mFifoBuffer.getFullDataAvailable(&wrappingBuffer);
        EXPECT_EQ(framesAvailable, wrapAvailable);
        bothAvailable = wrappingBuffer.numFrames[0] + wrappingBuffer.numFrames[1];
        EXPECT_EQ(framesAvailable, bothAvailable);
    }

    // Write data but do not overflow.
    void writeData(fifo_frames_t numFrames) {
        fifo_frames_t framesAvailable =
                mFifoBuffer.getFifoControllerBase()->getEmptyFramesAvailable();
        fifo_frames_t framesToWrite = std::min(framesAvailable, numFrames);
        for (int i = 0; i < framesToWrite; i++) {
            mData[i] = mNextWriteIndex++;
        }
        fifo_frames_t actual = mFifoBuffer.write(mData, framesToWrite);
        ASSERT_EQ(framesToWrite, actual);
    }

    // Read data but do not underflow.
    void verifyData(fifo_frames_t numFrames) {
        fifo_frames_t framesAvailable =
                mFifoBuffer.getFifoControllerBase()->getFullFramesAvailable();
        fifo_frames_t framesToRead = std::min(framesAvailable, numFrames);
        fifo_frames_t actual = mFifoBuffer.read(mData, framesToRead);
        ASSERT_EQ(framesToRead, actual);
        for (int i = 0; i < framesToRead; i++) {
            ASSERT_EQ(mNextVerifyIndex++, mData[i]);
        }
    }

    // Wrap around the end of the buffer.
    void checkWrappingWriteRead() {
        constexpr int frames1 = 43;
        constexpr int frames2 = 15;

        writeData(frames1);
        checkWrappingBuffer();
        verifyData(frames1);
        checkWrappingBuffer();

        writeData(frames2);
        checkWrappingBuffer();
        verifyData(frames2);
        checkWrappingBuffer();
    }

    // Write and Read a specific amount of data.
    void checkWriteRead() {
        const fifo_frames_t capacity = mFifoBuffer.getBufferCapacityInFrames();
        // Wrap around with the smaller region in the second half.
        const int frames1 = capacity - 4;
        const int frames2 = 7; // arbitrary, small
        writeData(frames1);
        verifyData(frames1);
        writeData(frames2);
        verifyData(frames2);
    }

    // Write and Read a specific amount of data.
    void checkWriteReadSmallLarge() {
        const fifo_frames_t capacity = mFifoBuffer.getBufferCapacityInFrames();
        // Wrap around with the larger region in the second half.
        const int frames1 = capacity - 4;
        const int frames2 = capacity - 9; // arbitrary, large
        writeData(frames1);
        verifyData(frames1);
        writeData(frames2);
        verifyData(frames2);
    }

    // Randomly read or write up to the maximum amount of data.
    void checkRandomWriteRead() {
        for (int i = 0; i < 20; i++) {
            fifo_frames_t framesEmpty =
                    mFifoBuffer.getFifoControllerBase()->getEmptyFramesAvailable();
            fifo_frames_t numFrames = (fifo_frames_t)(drand48() * framesEmpty);
            writeData(numFrames);

            fifo_frames_t framesFull =
                    mFifoBuffer.getFifoControllerBase()->getFullFramesAvailable();
            numFrames = (fifo_frames_t)(drand48() * framesFull);
            verifyData(numFrames);
        }
    }

    FifoBuffer     mFifoBuffer;
    int16_t       *mData;
    fifo_frames_t  mNextWriteIndex = 0;
    fifo_frames_t  mNextVerifyIndex = 0;
    fifo_frames_t  mThreshold;
};

TEST(test_fifo_buffer, fifo_read_write) {
    constexpr int capacity = 51; // arbitrary
    TestFifoBuffer tester(capacity);
    tester.checkMisc();
    tester.checkWriteRead();
}

TEST(test_fifo_buffer, fifo_wrapping_read_write) {
    constexpr int capacity = 59; // arbitrary, a little bigger this time
    TestFifoBuffer tester(capacity);
    tester.checkWrappingWriteRead();
}

TEST(test_fifo_buffer, fifo_read_write_small_large) {
    constexpr int capacity = 51; // arbitrary
    TestFifoBuffer tester(capacity);
    tester.checkWriteReadSmallLarge();
}

TEST(test_fifo_buffer, fifo_random_read_write) {
    constexpr int capacity = 51; // arbitrary
    TestFifoBuffer tester(capacity);
    tester.checkRandomWriteRead();
}

TEST(test_fifo_buffer, fifo_random_threshold) {
    constexpr int capacity = 67; // arbitrary
    constexpr int threshold = 37; // arbitrary
    TestFifoBuffer tester(capacity, threshold);
    tester.checkRandomWriteRead();
}
