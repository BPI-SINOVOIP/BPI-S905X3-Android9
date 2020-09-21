/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC SettingsPref
 */

package com.droidlogic;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

public class SettingsPref {
    private static final String FIRST_RUN           = "first_run";
    private static final String WIFI_SAVE_STATE     = "wifi_save_state";
    private static final String SAVED_BUILD_DATE    = "saved_build_date";
    private static final String SAVED_BOOT_COMPLETED_STATUS    = "saved_boot_completed_status";

    public static void setFirstRun(Context c, boolean firstRun) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        SharedPreferences.Editor editor = sp.edit();
        editor.putBoolean(FIRST_RUN, firstRun);
        editor.commit();
    }

    public static boolean getFirstRun(Context c) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        return sp.getBoolean(FIRST_RUN, true);
    }

    public static int getWiFiSaveState(Context c) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        return sp.getInt(WIFI_SAVE_STATE, 0);
    }

    public static void setWiFiSaveState(Context c, int state) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        SharedPreferences.Editor editor = sp.edit();
        editor.putInt(WIFI_SAVE_STATE, state);
        editor.commit();
    }

    public static String getSavedBuildDate(Context c) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        return sp.getString(SAVED_BUILD_DATE, "");
    }

    public static void setSavedBuildDate(Context c, String date) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        SharedPreferences.Editor editor = sp.edit();
        editor.putString(SAVED_BUILD_DATE, date);
        editor.commit();
    }

    public static void setSavedBootCompletedStatus(Context c, boolean state) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        SharedPreferences.Editor editor = sp.edit();
        editor.putBoolean(SAVED_BOOT_COMPLETED_STATUS, state);
        editor.commit();
    }

    public static boolean getSavedBootCompletedStatus(Context c) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(c);
        return sp.getBoolean(SAVED_BOOT_COMPLETED_STATUS, false);
    }
}

