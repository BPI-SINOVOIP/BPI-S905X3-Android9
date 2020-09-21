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
 * limitations under the License.
 */

package com.android.car.systeminterface;

import static java.util.concurrent.Executors.newSingleThreadScheduledExecutor;

import android.os.SystemClock;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * Interface that abstracts time operations
 */
public interface TimeInterface {
    public static final boolean INCLUDE_DEEP_SLEEP_TIME = true;
    public static final boolean EXCLUDE_DEEP_SLEEP_TIME = false;

    default long getUptime() {
        return getUptime(EXCLUDE_DEEP_SLEEP_TIME);
    }
    default long getUptime(boolean includeDeepSleepTime) {
        return includeDeepSleepTime ?
            SystemClock.elapsedRealtime() :
            SystemClock.uptimeMillis();
    }

    void scheduleAction(Runnable r, long delayMs);
    void cancelAllActions();

    class DefaultImpl implements TimeInterface {
        private final ScheduledExecutorService mExecutor = newSingleThreadScheduledExecutor();

        @Override
        public void scheduleAction(Runnable r, long delayMs) {
            mExecutor.scheduleAtFixedRate(r, delayMs, delayMs, TimeUnit.MILLISECONDS);
        }

        @Override
        public void cancelAllActions() {
            mExecutor.shutdownNow();
        }
    }
}
