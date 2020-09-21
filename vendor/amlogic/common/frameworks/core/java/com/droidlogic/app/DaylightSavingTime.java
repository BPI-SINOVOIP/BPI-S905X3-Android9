/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DaylightSavingTime
 */

package com.droidlogic.app;

import android.os.SystemClock;
//import android.os.ServiceManager;
import android.app.AlarmManager;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.util.Log;
import android.text.TextUtils;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;

import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;
import java.lang.String;

public class DaylightSavingTime {

    private static final String TAG = "DaylightSavingTime";

    private static final String DAYLIGHT_SAVING_TIME = "persist.vendor.sys.daylight.saving";
    private static final String IN_DAYLIGHT_SAVING_TIME = "persist.vendor.sys.in.daylight.saving";
    private static final int DAYLIGHT_TIME_AUTO = 0;
    private static final int DAYLIGHT_TIME_ON = 1;
    private static final int DAYLIGHT_TIME_OFF = 2;
    private static final int DEFAULT_DAYLIGHT_TIME = DAYLIGHT_TIME_OFF;
    private static final int DEFAULT_IN_DAYLIGHT_TIME = 0;

    private SystemControlManager mSystemControlManager;
    private static DaylightSavingTime mDaylightSavingTime = null;

    private DaylightSavingTime() {
        mSystemControlManager = SystemControlManager.getInstance();
    }

    public static synchronized DaylightSavingTime getInstance() {

        if (mDaylightSavingTime == null) {
            mDaylightSavingTime = new DaylightSavingTime();
        }

        return mDaylightSavingTime;
    }

    public void setDaylightSavingTime(int value) {

        Calendar calendar = Calendar.getInstance();
        boolean isInDaylightTimeZone = calendar.getTimeZone().
                inDaylightTime(calendar.getTime());
        boolean hasInDaylightTime = hasInDaylightSavingTime();

        Log.d(TAG, "setDaylightSavingTime = " + value);
        Log.d(TAG, "hasInDaylightTime = " + hasInDaylightTime);
        Log.d(TAG, "isInDaylightTimeZone = " + isInDaylightTimeZone);

        if (value == DAYLIGHT_TIME_AUTO) {
            if (isInDaylightTimeZone && !hasInDaylightTime) {
                updateDaylightSavingTime(calendar, 1);
            } else if (!isInDaylightTimeZone && hasInDaylightTime) {
                updateDaylightSavingTime(calendar, -1);
            }
        } else if (value == DAYLIGHT_TIME_ON) {
            if (!hasInDaylightTime) {
                updateDaylightSavingTime(calendar, 1);
            }
        } else if (value == DAYLIGHT_TIME_OFF) {
            if (hasInDaylightTime) {
                updateDaylightSavingTime(calendar, -1);
            }
        } else {
            Log.d(TAG, "setDaylightSavingTime error value!!!");
            return;
        }

        mSystemControlManager.setProperty(DAYLIGHT_SAVING_TIME, String.valueOf(value));
    }

    public int getDaylightSavingTime() {
        int value = mSystemControlManager.getPropertyInt(DAYLIGHT_SAVING_TIME,
                DEFAULT_DAYLIGHT_TIME);

        return value;
    }

    public boolean hasInDaylightSavingTime() {
        int value = mSystemControlManager.getPropertyInt(IN_DAYLIGHT_SAVING_TIME,
                DEFAULT_IN_DAYLIGHT_TIME);

        return (value == 1);
    }

    public synchronized void updateDaylightSavingTime(Calendar calendar, int diff) {

        calendar.add(Calendar.HOUR, diff);
        //AlarmManager alarmManager = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        //alarmManager.setTime(calendar.getTimeInMillis());
        SystemClock.setCurrentTimeMillis(calendar.getTimeInMillis());
        mSystemControlManager.setProperty(IN_DAYLIGHT_SAVING_TIME,
                String.valueOf((diff == 1) ? 1 : 0));
    }

    public void updateDaylightSavingTimeForce() {
        Log.d(TAG, "updateDaylightSavingTimeForce");

        int daylightTime = getDaylightSavingTime();
        Calendar calendar = Calendar.getInstance();
        boolean isInDaylightTimeZone = calendar.getTimeZone().inDaylightTime(calendar.getTime());

        if ((isInDaylightTimeZone && daylightTime == DAYLIGHT_TIME_AUTO)
                || daylightTime == DAYLIGHT_TIME_ON) {
            updateDaylightSavingTime(calendar, 1);
        }
    }
}
