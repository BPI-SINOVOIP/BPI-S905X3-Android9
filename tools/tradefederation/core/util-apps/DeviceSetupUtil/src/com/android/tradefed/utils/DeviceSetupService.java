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

import android.app.Activity;
import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.os.PowerManager;
import android.provider.Settings;
import android.util.Log;

/**
 * A service that sets up the device for testing by dismissing the keyguard
 * and keep the screen on.
 */
public class DeviceSetupService extends Service {
    PowerManager.WakeLock wl;

    private static final int SCREEN_OFF_TIMEOUT = -1; //never timeout

    @Override
    public void onCreate() {
        Log.v(DeviceSetupIntentReceiver.TAG, "Setup service started.");
        // dismiss keyguard
        KeyguardManager keyguardManager = (KeyguardManager) getSystemService(
                Activity.KEYGUARD_SERVICE);
        KeyguardLock lock = keyguardManager.newKeyguardLock(KEYGUARD_SERVICE);
        lock.disableKeyguard();

        // change screen off timeout
        Settings.System.putInt(
                getContentResolver(), Settings.System.SCREEN_OFF_TIMEOUT, SCREEN_OFF_TIMEOUT);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // keep screen on
        Log.v(DeviceSetupIntentReceiver.TAG, "Acquiring wake lock.");
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wl = pm.newWakeLock(
                PowerManager.SCREEN_DIM_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP,
                DeviceSetupIntentReceiver.TAG);
        wl.acquire();
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onDestroy() {
        if (wl != null){
            wl.release();
        }
        Log.v(DeviceSetupIntentReceiver.TAG, "Setup service stopped.");
    }
}
