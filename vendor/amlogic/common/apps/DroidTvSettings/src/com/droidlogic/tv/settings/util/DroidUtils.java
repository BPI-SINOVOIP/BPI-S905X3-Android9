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

package com.droidlogic.tv.settings.util;

import android.os.SystemProperties;
import android.support.v7.preference.Preference;
import android.text.TextUtils;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.util.Log;

/**
 * Utilities for working with Droid.
 */
public final class DroidUtils {
    private static final String TAG = "DroidUtils";

    public static final String KEY_HIDE_STARTUP = "hide_startup";
    public static final String VALUE_HIDE_STARTUP = "1";
    public static final String VALUE_SHOW_STARTUP = "0";
    /**
    * Non instantiable.
    */
    private DroidUtils() {
    }

    public static boolean hasTvUiMode() {
        return SystemProperties.getBoolean("ro.vendor.platform.has.tvuimode", false);
    }

    public static boolean hasMboxUiMode() {
        return SystemProperties.getBoolean("ro.vendor.platform.has.mboxuimode", false);
    }

    public static boolean hasGtvsUiMode() {
        return !TextUtils.isEmpty(SystemProperties.get("ro.com.google.gmsversion", ""));
    }

    public static void invisiblePreference(Preference preference, boolean tvUiMode) {
        if (preference == null) {
            return;
        }
        if (tvUiMode) {
            preference.setVisible(false);
        } else {
            preference.setVisible(true);
        }
    }

    public static void store(Context context, String keyword, String content) {
        SharedPreferences DealData = context.getSharedPreferences("record_value", 0);
        Editor editor = DealData.edit();
        editor.putString(keyword, content);
        editor.commit();
        Log.d(TAG, "store keyword: " + keyword + ",content: " + content);
    }

    public static String read(Context context, String keyword) {
        SharedPreferences DealData = context.getSharedPreferences("record_value", 0);
        Log.d(TAG, "read keyword: " + keyword + ",value: " + DealData.getString(keyword, null));
        return DealData.getString(keyword, null);
    }
}
