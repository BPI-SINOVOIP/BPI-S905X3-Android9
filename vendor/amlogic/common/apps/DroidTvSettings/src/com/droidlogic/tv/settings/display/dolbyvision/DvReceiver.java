/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DvReceiver
 */

package com.droidlogic.tv.settings.display.dolbyvision;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
//import android.os.Process;
import android.widget.Toast;

public class DvReceiver extends BroadcastReceiver {
    //static final String ACTION = "android.intent.action.BOOT_COMPLETED";

    @Override
    public void onReceive (Context context, Intent intent) {
        if (intent.getAction().equalsIgnoreCase(Intent.ACTION_BOOT_COMPLETED)) {
            Intent serviceIntent = new Intent(context, DolbyVisionService.class);
            context.startService(serviceIntent);
        }
    }

}


