/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.tradefed.utils;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * A BroadcastReceiver that starts a service to dismiss the keyguard.
 *
 * adb shell am broadcast -a com.android.tradefed.actor.DEVICE_SETUP
 */
public class DeviceSetupIntentReceiver extends BroadcastReceiver {

    public static final String
            DEVICE_SETUP_INTENT = "com.android.tradefed.utils.DEVICE_SETUP";
    public static final String TAG = "DeviceSetupUtil";
    private final Intent intent = new Intent();

    @Override
    public void onReceive(Context context, Intent intent) {
        if (DEVICE_SETUP_INTENT.equals(intent.getAction())) {
            Log.v(TAG, "Starting Service");
            intent.setClass(context, DeviceSetupService.class);
            context.startService(intent);
        } else if (Intent.ACTION_POWER_DISCONNECTED.equals(intent.getAction())) {
            // when power is disconnected assume the usb is disconnect,
            // stop the service from holding the wake lock
            Log.v(TAG, "Stopping Service");
            intent.setClass(context, DeviceSetupService.class);
            context.stopService(intent);
        } else if (Intent.ACTION_POWER_CONNECTED.equals(intent.getAction())) {
            // when power is connected assume the usb is connected,
            // restart the service to acquire the wake lock
            Log.v(TAG, "Re-starting Service");
            intent.setClass(context, DeviceSetupService.class);
            context.startService(intent);
        }
        Log.v(TAG, "Recevied: " + intent.getAction());
    }
}
