/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.tradefed.util;

/**
 * Data structure for holding a fixed size array that operates as a circular buffer,
 * and tracks the total sum of all values in the array.
 */
public class CircularByteArray {

    private byte[] mArray;
    private int mCurPos = 0;
    private boolean mIsWrapped = false;
    private long mSum = 0;

    public CircularByteArray(int size) {
        mArray = new byte[size];
    }

    /**
     * Adds a new value to array, replacing oldest value if necessary
     * @param value
     */
    public void add(byte value) {
        if (mIsWrapped) {
            // pop value and adjust total
            mSum -= mArray[mCurPos];
        }
        mArray[mCurPos] = value;
        mCurPos++;
        mSum += value;
        if (mCurPos >= mArray.length) {
            mIsWrapped = true;
            mCurPos = 0;
        }
    }

    /**
     * Get the number of elements stored
     */
    public int size() {
        if (mIsWrapped) {
            return mArray.length;
        } else {
            return mCurPos;
        }
    }

    /**
     * Gets the total value of all elements currently stored in array
     */
    public long getSum() {
        return mSum;
    }
}
