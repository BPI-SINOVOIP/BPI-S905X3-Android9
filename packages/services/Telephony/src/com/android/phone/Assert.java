/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */


package com.android.phone;

import android.os.Looper;

/**
 * Assertions which will result in program termination.
 */
public class Assert {

    private static Boolean sIsMainThreadForTest;

    public static void isTrue(boolean condition) {
        if (!condition) {
            throw new AssertionError("Expected condition to be true");
        }
    }

    public static void isMainThread() {
        if (sIsMainThreadForTest != null) {
            isTrue(sIsMainThreadForTest);
            return;
        }
        isTrue(Looper.getMainLooper().equals(Looper.myLooper()));
    }

    public static void isNotMainThread() {
        if (sIsMainThreadForTest != null) {
            isTrue(!sIsMainThreadForTest);
            return;
        }
        isTrue(!Looper.getMainLooper().equals(Looper.myLooper()));
    }

    public static void fail() {
        fail("Fail");
    }

    public static void fail(String reason) {
        throw new AssertionError(reason);
    }

    /**
     * Override the main thread status for tests. Set to null to revert to normal behavior
     */
    @NeededForTesting
    public static void setIsMainThreadForTesting(Boolean isMainThread) {
        sIsMainThreadForTest = isMainThread;
    }
}
