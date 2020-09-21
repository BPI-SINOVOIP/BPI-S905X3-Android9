/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput;

import com.droidlogic.app.tv.DroidLogicTvUtils;

import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;
import android.content.Context;
import android.preference.PreferenceManager;

public class Utils {
    private static final boolean DEBUG = true;
    private static final String PREF_KEY_LAST_WATCHED_CHANNEL_URI = "last_watched_channel_uri";

    public static void logd(String tag, String msg) {
        if (DEBUG)
            Log.d(tag, msg);
    }

    public static void loge(String tag, String msg) {
        if (DEBUG)
            Log.e(tag, msg);
    }

    public static int getChannelId(Uri uri) {
        return DroidLogicTvUtils.getChannelId(uri);
    }

    public static void setLastWatchedChannelUri(Context context, Uri uri) {
        PreferenceManager.getDefaultSharedPreferences(context).edit()
                .putString(PREF_KEY_LAST_WATCHED_CHANNEL_URI, uri.toString()).apply();
    }
    public static String getLastWatchedChannelUri(Context context) {
        return PreferenceManager.getDefaultSharedPreferences(context)
                .getString(PREF_KEY_LAST_WATCHED_CHANNEL_URI, null);
    }

}
