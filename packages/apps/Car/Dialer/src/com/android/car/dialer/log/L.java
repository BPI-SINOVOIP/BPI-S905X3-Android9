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
package com.android.car.dialer.log;

import android.util.Log;

/**
 * Util class for logging.
 */
public class L {

    private String mTag;

    public L(String tag) {
        mTag = tag;
    }

    public void v(String msg) {
        if (Log.isLoggable(mTag, Log.VERBOSE)) {
            Log.v(mTag, msg);
        }
    }

    public void d(String msg) {
        if (Log.isLoggable(mTag, Log.DEBUG)) {
            Log.d(mTag, msg);
        }
    }

    public void w(String msg) {
        Log.w(mTag, msg);
    }

    public static L logger(String tag) {
        return new L(tag);
    }

    public static void v(String tag, String msg) {
        if (Log.isLoggable(tag, Log.VERBOSE)) {
            Log.v(tag, msg);
        }
    }

    public static void d(String tag, String msg) {
        if (Log.isLoggable(tag, Log.DEBUG)) {
            Log.d(tag, msg);
        }
    }

    public static void w(String tag, String msg) {
        Log.w(tag, msg);
    }

    public static void i(String tag, String msg) {
        Log.i(tag, msg);
    }
}
