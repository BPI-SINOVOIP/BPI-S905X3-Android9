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
package android.autofillservice.cts;

import android.os.SystemClock;
import androidx.annotation.NonNull;
import android.text.TextUtils;
import android.util.Log;

import java.util.concurrent.Callable;

/**
 * A "smart" timeout that supports exponential backoff.
 */
//TODO: move to common CTS Code
public final class Timeout {

    private static final String TAG = "Timeout";
    private static final boolean VERBOSE = true;

    private final String mName;
    private long mCurrentValue;
    private final float mMultiplier;
    private final long mMaxValue;

    /**
     * Default constructor.
     *
     * @param name name to be used for logging purposes.
     * @param initialValue initial timeout value, in ms.
     * @param multiplier multiplier for {@link #increase()}.
     * @param maxValue max timeout value (in ms) set by {@link #increase()}.
     *
     * @throws IllegalArgumentException if {@code name} is {@code null} or empty,
     * {@code initialValue}, {@code multiplir} or {@code maxValue} are less than {@code 1},
     * or if {@code initialValue} is higher than {@code maxValue}
     */
    public Timeout(String name, long initialValue, float multiplier, long maxValue) {
        if (initialValue < 1 || maxValue < 1 || initialValue > maxValue) {
            throw new IllegalArgumentException(
                    "invalid initial and/or max values: " + initialValue + " and " + maxValue);
        }
        if (multiplier <= 1) {
            throw new IllegalArgumentException("multiplier must be higher than 1: " + multiplier);
        }
        if (TextUtils.isEmpty(name)) {
            throw new IllegalArgumentException("no name");
        }
        mName = name;
        mCurrentValue = initialValue;
        mMultiplier = multiplier;
        mMaxValue = maxValue;
        Log.d(TAG, "Constructor: " + this + " at " + JUnitHelper.getCurrentTestName());
    }

    /**
     * Gets the current timeout, in ms.
     */
    public long ms() {
        return mCurrentValue;
    }

    /**
     * Gets the max timeout, in ms.
     */
    public long getMaxValue() {
        return mMaxValue;
    }

    /**
     * @return the mMultiplier
     */
    public float getMultiplier() {
        return mMultiplier;
    }

    /**
     * Gets the user-friendly name of this timeout.
     */
    @NonNull
    public String getName() {
        return mName;
    }

    /**
     * Increases the current value by the {@link #getMultiplier()}, up to {@link #getMaxValue()}.
     *
     * @return previous current value.
     */
    public long increase() {
        final long oldValue = mCurrentValue;
        mCurrentValue = Math.min(mMaxValue, (long) (mCurrentValue * mMultiplier));
        if (oldValue != mCurrentValue) {
            Log.w(TAG, mName + " increased from " + oldValue + "ms to " + mCurrentValue + "ms at "
                    + JUnitHelper.getCurrentTestName());
        }
        return oldValue;
    }

    /**
     * Runs a {@code job} many times before giving up, sleeping between failed attempts up to
     * {@link #ms()}.
     *
     * @param description description of the job for logging purposes.
     * @param job job to be run, must return {@code null} if it failed and should be retried.
     * @throws RetryableException if all attempts failed.
     * @throws IllegalArgumentException if {@code description} is {@code null} or empty, if
     * {@code job} is {@code  null}, or if {@code maxAttempts} is less than 1.
     * @throws Exception any other exception thrown by helper methods.
     *
     * @return job's result.
     */
    public <T> T run(String description, Callable<T> job) throws Exception {
        return run(description, 100, job);
    }

    /**
     * Runs a {@code job} many times before giving up, sleeping between failed attempts up to
     * {@link #ms()}.
     *
     * @param description description of the job for logging purposes.
     * @param job job to be run, must return {@code null} if it failed and should be retried.
     * @param retryMs how long to sleep between failures.
     * @throws RetryableException if all attempts failed.
     * @throws IllegalArgumentException if {@code description} is {@code null} or empty, if
     * {@code job} is {@code  null}, or if {@code maxAttempts} is less than 1.
     * @throws Exception any other exception thrown by helper methods.
     *
     * @return job's result.
     */
    public <T> T run(String description, long retryMs, Callable<T> job) throws Exception {
        if (TextUtils.isEmpty(description)) {
            throw new IllegalArgumentException("no description");
        }
        if (job == null) {
            throw new IllegalArgumentException("no job");
        }
        if (retryMs < 1) {
            throw new IllegalArgumentException("need to sleep at least 1ms, right?");
        }
        long startTime = System.currentTimeMillis();
        int attempt = 0;
        while (System.currentTimeMillis() - startTime <= mCurrentValue) {
            final T result = job.call();
            if (result != null) {
                // Good news, everyone: job succeeded on first attempt!
                return result;
            }
            attempt++;
            if (VERBOSE) {
                Log.v(TAG, description + " failed at attempt #" + attempt + "; sleeping for "
                        + retryMs + "ms before trying again");
            }
            SystemClock.sleep(retryMs);
            retryMs *= mMultiplier;
        }
        Log.w(TAG, description + " failed after " + attempt + " attempts and "
                + (System.currentTimeMillis() - startTime) + "ms: " + this);
        throw new RetryableException(this, description);
    }

    @Override
    public String toString() {
        return mName + ": [current=" + mCurrentValue + "ms; multiplier=" + mMultiplier + "x; max="
                + mMaxValue + "ms]";
    }

}
