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
package com.android.compatibility.common.util;

import static junit.framework.Assert.fail;

import android.os.SystemClock;
import android.util.Log;

public class TestUtils {
    private static final String TAG = "CtsTestUtils";

    private TestUtils() {
    }

    public static final int DEFAULT_TIMEOUT_SECONDS = 30;

    /** Print an error log and fail. */
    public static void failWithLog(String message) {
        Log.e(TAG, message);
        fail(message);
    }

    @FunctionalInterface
    public interface BooleanSupplierWithThrow {
        boolean getAsBoolean() throws Exception;
    }

    @FunctionalInterface
    public interface RunnableWithThrow {
        void run() throws Exception;
    }

    /**
     * Wait until {@code predicate} is satisfied, or fail, with {@link #DEFAULT_TIMEOUT_SECONDS}.
     */
    public static void waitUntil(String message, BooleanSupplierWithThrow predicate)
            throws Exception {
        waitUntil(message, 0, predicate);
    }

    /**
     * Wait until {@code predicate} is satisfied, or fail, with a given timeout.
     */
    public static void waitUntil(
            String message, int timeoutSecond, BooleanSupplierWithThrow predicate)
            throws Exception {
        if (timeoutSecond <= 0) {
            timeoutSecond = DEFAULT_TIMEOUT_SECONDS;
        }
        int sleep = 125;
        final long timeout = SystemClock.uptimeMillis() + timeoutSecond * 1000;
        while (SystemClock.uptimeMillis() < timeout) {
            if (predicate.getAsBoolean()) {
                return; // okay
            }
            Thread.sleep(sleep);
            sleep *= 5;
            sleep = Math.min(2000, sleep);
        }
        failWithLog("Timeout: " + message);
    }

    /**
     * Run a Runnable {@code r}, and if it throws, also run {@code onFailure}.
     */
    public static void runWithFailureHook(RunnableWithThrow r, RunnableWithThrow onFailure)
            throws Exception {
        if (r == null) {
            throw new NullPointerException("r");
        }
        if (onFailure == null) {
            throw new NullPointerException("onFailure");
        }
        try {
            r.run();
        } catch (Throwable th) {
            Log.e(TAG, "Caught exception: " + th, th);
            onFailure.run();
            throw th;
        }
    }
}

