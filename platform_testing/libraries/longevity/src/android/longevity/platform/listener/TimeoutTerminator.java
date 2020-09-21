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
package android.longevity.platform.listener;

import android.os.SystemClock;
import android.util.Log;

import java.util.Map;

import org.junit.runner.notification.RunNotifier;

/**
 * A {@link RunTerminator} for terminating early on test end due to long duration for platform
 * suites.
 */
public final class TimeoutTerminator extends android.longevity.core.listener.TimeoutTerminator {
    public TimeoutTerminator(RunNotifier notifier, Map<String, String> args) {
        super(notifier, args);
    }

    /**
     * Returns the current timestamp for an Android device.
     */
    @Override
    protected long getCurrentTimestamp() {
        return SystemClock.uptimeMillis();
    }

    /**
     * Prints the message to logcat.
     */
    @Override
    protected void print(String reason) {
        Log.e(getClass().getSimpleName(), reason);
    }
}
