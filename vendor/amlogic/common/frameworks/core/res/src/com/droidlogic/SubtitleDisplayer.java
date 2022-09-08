/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic;


import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;

import android.util.Log;

import com.droidlogic.app.SubtitleManager;


public class SubtitleDisplayer extends Service {

    private static String TAG = "SubtitleDisplayer";
    private int mServiceStartId = -1;
    private Context mContext = null;
    private boolean mDebug = false;

    SubtitleManager mSubtitleManager;

    public SubtitleDisplayer(Context context) {
        Log.i(TAG, "[SubtitleDisplayer]");
        mContext = context;
    }

    public SubtitleDisplayer() {
       Log.i(TAG, "[SubtitleDisplayer]");
    }

    @Override
    public void onCreate() {
        Log.i(TAG, "[onCreate]");
        super.onCreate();

        /* true means, this create fallback subtitle manager */
        mSubtitleManager = new SubtitleManager(this, true, R.drawable.loading);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "[onStartCommand]");
        mServiceStartId = startId;

        // start fallback now.
        mSubtitleManager.startFallbackDisplay();
        return START_STICKY;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.i(TAG, "[onUnbind]");
        stopSelf(mServiceStartId);
        return super.onUnbind(intent);
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "[onDestroy]");
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "[onBind]");
        return null;
    }
}

