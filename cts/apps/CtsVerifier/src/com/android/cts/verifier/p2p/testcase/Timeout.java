/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.cts.verifier.p2p.testcase;

/**
 * This class holds the remaining timeout value.
 */
public class Timeout {

    private long mExpiredTime;

    /**
     * Initialize timeout value.
     * @param msec
     */
    public Timeout(long msec) {
        mExpiredTime = System.currentTimeMillis() + msec;
    }

    /**
     * Return true if the timeout has already expired.
     * @return
     */
    boolean isTimeout() {
        return getRemainTime() == 0;
    }

    /**
     * Return the remaining timeout value.
     * If already expired, return 0.
     * @return
     */
    long getRemainTime() {
        long t = mExpiredTime - System.currentTimeMillis();
        if (t < 0) {
            return 0;
        }
        return t;
    }
}
