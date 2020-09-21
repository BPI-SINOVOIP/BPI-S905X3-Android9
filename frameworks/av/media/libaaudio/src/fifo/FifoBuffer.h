/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef FIFO_FIFO_BUFFER_H
#define FIFO_FIFO_BUFFER_H

#include <stdint.h>

#include "FifoControllerBase.h"

namespace android {

/**
 * Structure that represents a region in a circular buffer that might be at the
 * end of the array and split in two.
 */
struct WrappingBuffer {
    enum {
        SIZE = 2
    };
    void *data[SIZE];
    int32_t numFrames[SIZE];
};

class FifoBuffer {
public:
    FifoBuffer(int32_t bytesPerFrame, fifo_frames_t capacityInFrames);

    FifoBuffer(int32_t bytesPerFrame,
               fifo_frames_t capacityInFrames,
               fifo_counter_t *readCounterAddress,
               fifo_counter_t *writeCounterAddress,
               void *dataStorageAddress);

    ~FifoBuffer();

    int32_t convertFramesToBytes(fifo_frames_t frames);

    fifo_frames_t read(void *destination, fifo_frames_t framesToRead);

    fifo_frames_t write(const void *source, fifo_frames_t framesToWrite);

    fifo_frames_t getThreshold();

    void setThreshold(fifo_frames_t threshold);

    fifo_frames_t getBufferCapacityInFrames();

    /**
     * Return pointer to available full frames in data1 and set size in numFrames1.
     * if the data is split across the end of the FIFO then set data2 and numFrames2.
     * Other wise set them to null
     * @param wrappingBuffer
     * @return total full frames available
     */
    fifo_frames_t getFullDataAvailable(WrappingBuffer *wrappingBuffer);

    /**
     * Return pointer to available empty frames in data1 and set size in numFrames1.
     * if the room is split across the end of the FIFO then set data2 and numFrames2.
     * Other wise set them to null
     * @param wrappingBuffer
     * @return total empty frames available
     */
    fifo_frames_t getEmptyRoomAvailable(WrappingBuffer *wrappingBuffer);

    /**
     * Copy data from the FIFO into the buffer.
     * @param buffer
     * @param numFrames
     * @return
     */
    fifo_frames_t readNow(void *buffer, fifo_frames_t numFrames);

    int64_t getNextReadTime(int32_t frameRate);

    int32_t getUnderrunCount() const { return mUnderrunCount; }

    FifoControllerBase *getFifoControllerBase() { return mFifo; }

    int32_t getBytesPerFrame() {
        return mBytesPerFrame;
    }

    fifo_counter_t getReadCounter() {
        return mFifo->getReadCounter();
    }

    void setReadCounter(fifo_counter_t n) {
        mFifo->setReadCounter(n);
    }

    fifo_counter_t getWriteCounter() {
        return mFifo->getWriteCounter();
    }

    void setWriteCounter(fifo_counter_t n) {
        mFifo->setWriteCounter(n);
    }

    /*
     * This is generally only called before or after the buffer is used.
     */
    void eraseMemory();

private:

    void fillWrappingBuffer(WrappingBuffer *wrappingBuffer,
                            int32_t framesAvailable, int32_t startIndex);

    const fifo_frames_t mFrameCapacity;
    const int32_t mBytesPerFrame;
    uint8_t *mStorage;
    bool mStorageOwned; // did this object allocate the storage?
    FifoControllerBase *mFifo;
    fifo_counter_t mFramesReadCount;
    fifo_counter_t mFramesUnderrunCount;
    int32_t mUnderrunCount; // need? just use frames
    int32_t mLastReadSize;
};

}  // android

#endif //FIFO_FIFO_BUFFER_H
