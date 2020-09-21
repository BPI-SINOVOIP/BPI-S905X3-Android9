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

package android.server.am;

import android.util.Log;

/**
 * Util class to perform simple state logging.
 */
public class StateLogger {

    private static final boolean DEBUG = false;
    private static String TAG = "AMWM";

    /**
     * Simple info-level logging gated by {@link #DEBUG} flag
     */
    public static void log(String logText) {
        if (DEBUG) {
            Log.i(TAG, logText);
        }
    }

    public static void logAlways(String logText) {
        Log.i(TAG, logText);
    }

    public static void logE(String logText) {
        Log.e(TAG, logText);
    }

    public static void logE(String logText, Throwable e) {
        Log.e(TAG, logText, e);
    }
}
