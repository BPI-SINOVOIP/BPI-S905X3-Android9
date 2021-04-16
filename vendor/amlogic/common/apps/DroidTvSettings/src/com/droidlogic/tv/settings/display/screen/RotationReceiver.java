/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BluetoothAutoPairReceiver
 */

package com.droidlogic.tv.settings.display.screen;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.SystemProperties;
import android.provider.Settings;
import android.content.BroadcastReceiver;
import android.content.pm.PackageManager;
import android.content.ComponentName;
import android.content.pm.PackageInfo;
import java.util.List;

public class RotationReceiver extends BroadcastReceiver {
    private static final String TAG = "RotationReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
             int isAuto = android.provider.Settings.System.getInt(context.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 0);
             if (isAuto == 1) {
                 setRotation(context, 0);
             } else {
                 int rotation = SystemProperties.getInt("persist.sys.rotation", 0);
                 setRotation(context, rotation);
             }
        }
    }
    private void setRotation(Context context, int val) {
        android.provider.Settings.System.putInt(context.getContentResolver(), Settings.System.USER_ROTATION, val);
	SystemProperties.set("persist.sys.rotation", String.valueOf(val));
    }
}
