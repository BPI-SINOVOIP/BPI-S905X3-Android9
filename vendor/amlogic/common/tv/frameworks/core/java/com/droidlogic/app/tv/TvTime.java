/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.Context;
import android.provider.Settings;
import android.os.SystemClock;
import android.util.Log;

import java.util.Date;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.DaylightSavingTime;
import com.droidlogic.app.tv.TvControlDataManager;

public class TvTime{
    private long diff = 0;
    private Context mContext;

    private final static String TV_KEY_TVTIME = "dtvtime";
    private final static String PROP_SET_SYSTIME_ENABLED = "persist.tv.getdtvtime.isneed";
    private TvControlDataManager mTvControlDataManager = null;
    private SystemControlManager mSystemControlManager = null;
    private final static String TV_STREAM_TIME = "vendor.sys.tv.stream.realtime";//to fit for dtvkit

    public TvTime(Context context){
        mContext = context;
        mTvControlDataManager = TvControlDataManager.getInstance(mContext);
        mSystemControlManager=  SystemControlManager.getInstance();
    }

    public synchronized void setTime(long time){
        Date sys = new Date();

        diff = time - sys.getTime();
        if (mSystemControlManager.getPropertyBoolean(PROP_SET_SYSTIME_ENABLED, false)
                && (Math.abs(diff) > 1000)) {
            SystemClock.setCurrentTimeMillis(time);
            diff = 0;
            DaylightSavingTime daylightSavingTime = DaylightSavingTime.getInstance();
            daylightSavingTime.updateDaylightSavingTimeForce();
        }

        //mTvControlDataManager.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, diff);
        mSystemControlManager.setProperty(TV_STREAM_TIME, String.valueOf(diff));
    }


    public synchronized long getTime(){
        Date sys = new Date();
        //diff = mTvControlDataManager.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);
        diff = mSystemControlManager.getPropertyLong(TV_STREAM_TIME, 0);

        return sys.getTime() + diff;
    }


    public synchronized long getDiffTime(){
        //return mTvControlDataManager.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);
        return mSystemControlManager.getPropertyLong(TV_STREAM_TIME, 0);
    }

    public synchronized void setDiffTime(long diff){
        this.diff = diff;
        //mTvControlDataManager.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, this.diff);
        mSystemControlManager.setProperty(TV_STREAM_TIME, String.valueOf(this.diff));
    }
}

