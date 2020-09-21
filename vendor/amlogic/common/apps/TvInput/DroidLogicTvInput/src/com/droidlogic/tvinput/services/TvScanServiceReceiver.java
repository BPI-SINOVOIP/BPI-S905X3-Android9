/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput.services;

import android.os.SystemProperties;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;



public class TvScanServiceReceiver extends BroadcastReceiver {
    static final String TAG = "TvScanServiceReceiver";
    private static final String ACTION_BOOT_COMPLETED ="android.intent.action.BOOT_COMPLETED";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            {
                Log.d(TAG,"TvScanService Start*******************************************");
                context.startService(new Intent(context, TvScanService.class));
            }
        }
    }
}
