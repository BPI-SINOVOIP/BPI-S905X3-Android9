/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC TimerSuspendReceiver
 */

package com.droidlogic.droidlivetv;

import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.util.Log;

import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.SystemControlManager;

public class TimerSuspendReceiver extends BroadcastReceiver {
    private static final String TAG = "TimerSuspendReceiver";

    private Context mContext = null;
    private SystemControlManager mSystemControlManager = null;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive: " + intent);

        mContext = context;
        mSystemControlManager = SystemControlManager.getInstance();
        if (intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, false)) {
            mSystemControlManager.setProperty("persist.tv.sleep_timer", 0 + "");//clear it as acted
        }
        startSleepTimer(intent);
    }

    public void startSleepTimer (Intent intent) {
        Log.d(TAG, "startSleepTimer");
        Intent intentservice = new Intent(mContext, TimerSuspendService.class );
        intentservice.putExtra(DroidLogicTvUtils.KEY_ENABLE_NOSIGNAL_TIMEOUT, intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_NOSIGNAL_TIMEOUT, false));
        intentservice.putExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, intent.getBooleanExtra(DroidLogicTvUtils.KEY_ENABLE_SUSPEND_TIMEOUT, false));
        mContext.startService (intentservice);
    }
}
