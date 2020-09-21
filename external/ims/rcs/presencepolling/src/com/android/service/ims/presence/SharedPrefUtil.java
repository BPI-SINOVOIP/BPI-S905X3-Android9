/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import android.content.Context;
import android.content.SharedPreferences;

public class SharedPrefUtil {
    public static final String EAB_SHARED_PREF = "com.android.vt.eab";
    public static final String INIT_DONE = "init_done";
    private static final String CONTACT_CHANGED_PREF_KEY = "timestamp_change";
    private static final String CONTACT_DELETE_PREF_KEY = "timestamp_delete";
    private static final String CONTACT_PROFILE_CHANGED_PREF_KEY = "profile_timestamp_change";

    public static boolean isInitDone(Context context) {
        SharedPreferences eabPref = context.getSharedPreferences(
                EAB_SHARED_PREF, Context.MODE_PRIVATE);
        return eabPref.getBoolean(INIT_DONE, false);
    }

    public static void setInitDone(Context context, boolean initDone){
        SharedPreferences.Editor eabPref = context.getSharedPreferences(
                EAB_SHARED_PREF, Context.MODE_PRIVATE).edit();
        eabPref.putBoolean(INIT_DONE, initDone).commit();
    }

    public static long getLastContactChangedTimestamp(Context context, long time) {
        long contactModifySyncTime = 0;
        SharedPreferences pref = context.getSharedPreferences(EAB_SHARED_PREF,
                Context.MODE_PRIVATE);
        contactModifySyncTime = pref.getLong(CONTACT_CHANGED_PREF_KEY, time);
        return validateDeviceTimestamp(context, contactModifySyncTime);
    }

    public static void saveLastContactChangedTimestamp(Context context, long time) {
        SharedPreferences.Editor eabPref = context.getSharedPreferences(
                EAB_SHARED_PREF, Context.MODE_PRIVATE).edit();
        eabPref.putLong(CONTACT_CHANGED_PREF_KEY, time).commit();
    }

    public static long getLastProfileContactChangedTimestamp(Context context, long time) {
        long profileModifySyncTime = 0;
        SharedPreferences pref = context.getSharedPreferences(EAB_SHARED_PREF,
                Context.MODE_PRIVATE);
        profileModifySyncTime = pref.getLong(CONTACT_PROFILE_CHANGED_PREF_KEY, time);
        return validateDeviceTimestamp(context, profileModifySyncTime);
    }

    public static void saveLastProfileContactChangedTimestamp(Context context, long time) {
        SharedPreferences.Editor eabPref = context.getSharedPreferences(
                EAB_SHARED_PREF, Context.MODE_PRIVATE).edit();
        eabPref.putLong(CONTACT_PROFILE_CHANGED_PREF_KEY, time).commit();
    }

    public static long getLastContactDeletedTimestamp(Context context, long time) {
        long contactDeleteSyncTime = 0;
        SharedPreferences pref = context.getSharedPreferences(EAB_SHARED_PREF,
                Context.MODE_PRIVATE);
        contactDeleteSyncTime = pref.getLong(CONTACT_DELETE_PREF_KEY, time);
        return validateDeviceTimestamp(context, contactDeleteSyncTime);
    }

    public static void saveLastContactDeletedTimestamp(Context context, long time) {
        SharedPreferences.Editor eabPref = context.getSharedPreferences(
                EAB_SHARED_PREF, Context.MODE_PRIVATE).edit();
        eabPref.putLong(CONTACT_DELETE_PREF_KEY, time).commit();
    }

    private static long validateDeviceTimestamp(Context context, long lastSyncedTimestamp) {
        long deviceCurrentTimeMillis = System.currentTimeMillis();
        if((lastSyncedTimestamp != 0) && lastSyncedTimestamp > deviceCurrentTimeMillis) {
            SharedPreferences.Editor eabPref = context.getSharedPreferences(
                    EAB_SHARED_PREF, Context.MODE_PRIVATE).edit();
            eabPref.putLong(CONTACT_CHANGED_PREF_KEY, 0);
            eabPref.putLong(CONTACT_DELETE_PREF_KEY, 0);
            eabPref.putLong(CONTACT_PROFILE_CHANGED_PREF_KEY, 0);
            eabPref.commit();
            lastSyncedTimestamp = 0;
        }
        return lastSyncedTimestamp;
    }

    public static void resetEABSharedPref(Context context) {
        SharedPreferences.Editor eabPref = context.getSharedPreferences(
                EAB_SHARED_PREF, Context.MODE_PRIVATE).edit();
        eabPref.putBoolean(INIT_DONE, false);
        eabPref.putLong(CONTACT_CHANGED_PREF_KEY, 0);
        eabPref.putLong(CONTACT_DELETE_PREF_KEY, 0);
        eabPref.putLong(CONTACT_PROFILE_CHANGED_PREF_KEY, 0);
        eabPref.commit();
    }
}
