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

package android.graphics.drawable.cts;

import static junit.framework.TestCase.assertTrue;

import static org.junit.Assert.assertEquals;

import android.graphics.drawable.Animatable2;
import android.graphics.drawable.Drawable;

// Now this class can not only listen to the events, but also synchronize the key events,
// logging the event timestamp, and centralize some assertions.
public class Animatable2Callback extends Animatable2.AnimationCallback {
    private static final long MAX_START_TIMEOUT_MS = 5000;

    private boolean mStarted = false;
    private boolean mEnded = false;

    private long mStartNs = Long.MAX_VALUE;
    private long mEndNs = Long.MIN_VALUE;

    // Use this lock to make sure the onAnimationEnd() has been called.
    // Each sub test should have its own lock.
    private final Object mEndLock = new Object();

    // Use this lock to make sure the test thread know when the AVD.start() has been called.
    // Each sub test should have its own lock.
    private final Object mStartLock = new Object();

    public boolean waitForEnd(long timeoutMs) throws InterruptedException {
        synchronized (mEndLock) {
            if (!mEnded) {
                // Return immediately if the AVD has already ended.
                mEndLock.wait(timeoutMs);
            }
            return mEnded;
        }
    }

    public boolean waitForStart() throws InterruptedException {
        synchronized(mStartLock) {
            if (!mStarted) {
                // Return immediately if the AVD has already started.
                mStartLock.wait(MAX_START_TIMEOUT_MS);
            }
            return mStarted;
        }
    }

    @Override
    public void onAnimationStart(Drawable drawable) {
        mStartNs = System.nanoTime();
        synchronized(mStartLock) {
            mStarted = true;
            mStartLock.notify();
        }
    }

    @Override
    public void onAnimationEnd(Drawable drawable) {
        mEndNs = System.nanoTime();
        synchronized (mEndLock) {
            mEnded = true;
            mEndLock.notify();
        }
    }

    public boolean endIsCalled() {
        synchronized (mEndLock) {
            return mEnded;
        }
    }

    public void assertStarted(boolean started) {
        assertEquals(started, mStarted);
    }

    public void assertEnded(boolean ended) {
        assertEquals(ended, mEnded);
    }

    public void assertAVDRuntime(long min, long max) {
        assertTrue(mStartNs != Long.MAX_VALUE);
        assertTrue(mEndNs != Long.MIN_VALUE);
        long durationNs = mEndNs - mStartNs;
        assertTrue("current duration " + durationNs + " should be within " +
                   min + "," + max, durationNs <= max && durationNs >= min);
    }
}
