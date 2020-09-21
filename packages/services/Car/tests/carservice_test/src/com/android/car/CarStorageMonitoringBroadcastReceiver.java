/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.car;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.SystemClock;
import android.util.Log;

import java.time.Duration;

public class CarStorageMonitoringBroadcastReceiver extends BroadcastReceiver {
    private static final String TAG = CarStorageMonitoringBroadcastReceiver.class.getSimpleName();
    private static Intent mLastIntent;
    private static final Object mSync = new Object();

    static void deliver(Intent intent) {
        Log.i(TAG, "onReceive, intent: " + intent);
        synchronized (mSync) {
            mLastIntent = intent;
            mSync.notify();
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.i(TAG, "onReceive, intent: " + intent);
        synchronized (mSync) {
            mLastIntent = intent;
            mSync.notify();
        }
    }

    static Intent reset() {
        Log.i(TAG, "reset");
        synchronized (mSync) {
            Intent lastIntent = mLastIntent;
            mLastIntent = null;
            return lastIntent;
        }
    }

    static boolean waitForIntent(Duration duration) {
        long start = SystemClock.elapsedRealtime();
        long end = start + duration.toMillis();
        synchronized (mSync) {
            while (mLastIntent == null && SystemClock.elapsedRealtime() < end) {
                try {
                    mSync.wait(10L);
                } catch (InterruptedException e) {
                    // ignore
                }
            }
        }

        return (mLastIntent != null);
    }
}
