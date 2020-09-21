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
package com.android.tv.util;

import android.os.SystemClock;
import android.content.Context;
import android.util.Log;

import com.android.tv.TvSingletons;
import com.android.tv.common.util.Clock;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvTime;
/**
 * An interface through which system clocks can be read. The {@link #SYSTEM} implementation
 * must be used for all non-test cases.
 */
public class TvClock implements Clock {
    private static final String TAG = "TvClock";
    private static final boolean DEBUG = false;
    private static final String PROP_SET_TVTIME_ENABLED = "persist.tv.usetvtime";

    private Context mContext;
    private SystemControlManager mSystemControlManager;
    private TvTime mTvTime;
    private boolean isTvTimeEnabled;

    public TvClock(Context context) {
        mContext = context;
        mSystemControlManager = TvSingletons.getSingletons(context).getSystemControlManager();
        mTvTime = TvSingletons.getSingletons(context).getTvTime();
        isTvTimeEnabled = mSystemControlManager.getPropertyBoolean(PROP_SET_TVTIME_ENABLED, true);
    }

    public long currentTimeMillis() {
        if (isTvTimeEnabled) {
            return mTvTime.getTime();
        }else {
            return System.currentTimeMillis();
        }
    }

    public long elapsedRealtime() {
        return SystemClock.elapsedRealtime();
    }

    public long uptimeMillis() {
            return SystemClock.uptimeMillis();
    }

    public void sleep(long ms) {
        SystemClock.sleep(ms);
    }

    public long getCurrentStreamTime(boolean streamtime) {
        long result = System.currentTimeMillis();
        if (streamtime) {
            result = result + getStreamTimeDiff();
        }
        if (DEBUG) {
            Log.d(TAG, "getCurrentStreamTime istvstream " + streamtime + " time:" + Utils.toTimeString(result));
        }
        return result;
    }

    public long getStreamTimeDiff() {
        long result = 0;
        result = mTvTime.getDiffTime();
        if (DEBUG) {
            Log.d(TAG, "getStreamTimeDiff = " + result);
        }
        return result;
    }
}
