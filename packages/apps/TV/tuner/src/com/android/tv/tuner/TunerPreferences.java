/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.tuner;

import android.content.Context;
import android.content.SharedPreferences;
import com.android.tv.common.CommonConstants;
import com.android.tv.common.CommonPreferences;
import com.android.tv.common.SoftPreconditions;

/** A helper class for the tuner preferences. */
public class TunerPreferences extends CommonPreferences {
    private static final String TAG = "TunerPreferences";

    private static final String PREFS_KEY_CHANNEL_DATA_VERSION = "channel_data_version";
    private static final String PREFS_KEY_SCANNED_CHANNEL_COUNT = "scanned_channel_count";
    private static final String PREFS_KEY_SCAN_DONE = "scan_done";
    private static final String PREFS_KEY_TRICKPLAY_EXPIRED_MS = "trickplay_expired_ms";

    private static final String SHARED_PREFS_NAME =
            CommonConstants.BASE_PACKAGE + ".tuner.preferences";

    public static final int CHANNEL_DATA_VERSION_NOT_SET = -1;

    protected static SharedPreferences getSharedPreferences(Context context) {
        return context.getSharedPreferences(SHARED_PREFS_NAME, Context.MODE_PRIVATE);
    }

    public static synchronized int getChannelDataVersion(Context context) {
        SoftPreconditions.checkState(sInitialized);
        return getSharedPreferences(context)
                .getInt(
                        TunerPreferences.PREFS_KEY_CHANNEL_DATA_VERSION,
                        CHANNEL_DATA_VERSION_NOT_SET);
    }

    public static synchronized void setChannelDataVersion(Context context, int version) {
        SoftPreconditions.checkState(sInitialized);
        getSharedPreferences(context)
                .edit()
                .putInt(TunerPreferences.PREFS_KEY_CHANNEL_DATA_VERSION, version)
                .apply();
    }

    public static synchronized int getScannedChannelCount(Context context) {
        SoftPreconditions.checkState(sInitialized);
        return getSharedPreferences(context)
                .getInt(TunerPreferences.PREFS_KEY_SCANNED_CHANNEL_COUNT, 0);
    }

    public static synchronized void setScannedChannelCount(Context context, int channelCount) {
        SoftPreconditions.checkState(sInitialized);
        getSharedPreferences(context)
                .edit()
                .putInt(TunerPreferences.PREFS_KEY_SCANNED_CHANNEL_COUNT, channelCount)
                .apply();
    }

    public static synchronized boolean isScanDone(Context context) {
        SoftPreconditions.checkState(sInitialized);
        return getSharedPreferences(context)
                .getBoolean(TunerPreferences.PREFS_KEY_SCAN_DONE, false);
    }

    public static synchronized void setScanDone(Context context) {
        SoftPreconditions.checkState(sInitialized);
        getSharedPreferences(context)
                .edit()
                .putBoolean(TunerPreferences.PREFS_KEY_SCAN_DONE, true)
                .apply();
    }

    public static synchronized long getTrickplayExpiredMs(Context context) {
        SoftPreconditions.checkState(sInitialized);
        return getSharedPreferences(context)
                .getLong(TunerPreferences.PREFS_KEY_TRICKPLAY_EXPIRED_MS, 0);
    }

    public static synchronized void setTrickplayExpiredMs(Context context, long timeMs) {
        SoftPreconditions.checkState(sInitialized);
        getSharedPreferences(context)
                .edit()
                .putLong(TunerPreferences.PREFS_KEY_TRICKPLAY_EXPIRED_MS, timeMs)
                .apply();
    }
}
