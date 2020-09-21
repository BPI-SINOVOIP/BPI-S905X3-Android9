/*
 * Copyright 2018 The Android Open Source Project
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

#pragma once

#include <utils/Errors.h>

#include <mutex>

using namespace android::surfaceflinger;

namespace android {

/*
 * Modulates the vsync-offsets depending on current SurfaceFlinger state.
 */
class VSyncModulator {
private:

    // Number of frames we'll keep the early phase offsets once they are activated. This acts as a
    // low-pass filter in case the client isn't quick enough in sending new transactions.
    const int MIN_EARLY_FRAME_COUNT = 2;

public:

    enum TransactionStart {
        EARLY,
        NORMAL
    };

    // Sets the phase offsets
    //
    // early: the phase offset when waking up early. May be the same as late, in which case we don't
    //        shift offsets.
    // late: the regular sf phase offset.
    void setPhaseOffsets(nsecs_t early, nsecs_t late) {
        mEarlyPhaseOffset = early;
        mLatePhaseOffset = late;
        mPhaseOffset = late;
    }

    nsecs_t getEarlyPhaseOffset() const {
        return mEarlyPhaseOffset;
    }

    void setEventThread(EventThread* eventThread) {
        mEventThread = eventThread;
    }

    void setTransactionStart(TransactionStart transactionStart) {

        if (transactionStart == TransactionStart::EARLY) {
            mRemainingEarlyFrameCount = MIN_EARLY_FRAME_COUNT;
        }

        // An early transaction stays an early transaction.
        if (transactionStart == mTransactionStart || mTransactionStart == TransactionStart::EARLY) {
            return;
        }
        mTransactionStart = transactionStart;
        updatePhaseOffsets();
    }

    void onTransactionHandled() {
        if (mTransactionStart == TransactionStart::NORMAL) return;
        mTransactionStart = TransactionStart::NORMAL;
        updatePhaseOffsets();
    }

    void onRefreshed(bool usedRenderEngine) {
        bool updatePhaseOffsetsNeeded = false;
        if (mRemainingEarlyFrameCount > 0) {
            mRemainingEarlyFrameCount--;
            updatePhaseOffsetsNeeded = true;
        }
        if (usedRenderEngine != mLastFrameUsedRenderEngine) {
            mLastFrameUsedRenderEngine = usedRenderEngine;
            updatePhaseOffsetsNeeded = true;
        }
        if (updatePhaseOffsetsNeeded) {
            updatePhaseOffsets();
        }
    }

private:

    void updatePhaseOffsets() {

        // Do not change phase offsets if disabled.
        if (mEarlyPhaseOffset == mLatePhaseOffset) return;

        if (shouldUseEarlyOffset()) {
            if (mPhaseOffset != mEarlyPhaseOffset) {
                if (mEventThread) {
                    mEventThread->setPhaseOffset(mEarlyPhaseOffset);
                }
                mPhaseOffset = mEarlyPhaseOffset;
            }
        } else {
            if (mPhaseOffset != mLatePhaseOffset) {
                if (mEventThread) {
                    mEventThread->setPhaseOffset(mLatePhaseOffset);
                }
                mPhaseOffset = mLatePhaseOffset;
            }
        }
    }

    bool shouldUseEarlyOffset() {
        return mTransactionStart == TransactionStart::EARLY || mLastFrameUsedRenderEngine
                || mRemainingEarlyFrameCount > 0;
    }

    nsecs_t mLatePhaseOffset = 0;
    nsecs_t mEarlyPhaseOffset = 0;
    EventThread* mEventThread = nullptr;
    std::atomic<nsecs_t> mPhaseOffset = 0;
    std::atomic<TransactionStart> mTransactionStart = TransactionStart::NORMAL;
    std::atomic<bool> mLastFrameUsedRenderEngine = false;
    std::atomic<int> mRemainingEarlyFrameCount = 0;
};

} // namespace android
