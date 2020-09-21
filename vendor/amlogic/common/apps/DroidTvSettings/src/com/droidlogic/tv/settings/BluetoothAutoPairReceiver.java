/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC BluetoothAutoPairReceiver
 */

package com.droidlogic.tv.settings;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.SystemProperties;
import android.content.BroadcastReceiver;
import android.content.pm.PackageManager;
import android.content.ComponentName;
import android.content.pm.PackageInfo;
import java.util.List;

public class BluetoothAutoPairReceiver extends BroadcastReceiver {
    private static final String TAG = "BluetoothAutoPairReceiver";
    private static final boolean DEBUG = true;

    private void Log(String msg) {
        if (DEBUG) {
            Log.i(TAG, msg);
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            Log("Received ACTION_BOOT_COMPLETED");
            if (!isAvilible(context,"com.google.android.tungsten.setupwraith")) {
                Log(" aosp need autoconnectBT!");
                Intent BtSetupIntent = new Intent(context,BtSetupActivity.class);
                BtSetupIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(BtSetupIntent);
            } else {
                Log("gms no need autoconnectBT from receive!");
            }
        }
    }

    private boolean isAvilible( Context context, String packageName ){
        final PackageManager packageManager = context.getPackageManager();
        List<PackageInfo> pinfo = packageManager.getInstalledPackages(0);
        for ( int i = 0; i < pinfo.size(); i++ )
        {
                if (pinfo.get(i).packageName.equalsIgnoreCase(packageName)) {
                    return true;
                }
        }
        return false;
    }
}
