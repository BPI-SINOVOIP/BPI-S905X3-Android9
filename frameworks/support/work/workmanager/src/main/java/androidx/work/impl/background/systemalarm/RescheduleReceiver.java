/*
 * Copyright 2018 The Android Open Source Project
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

package androidx.work.impl.background.systemalarm;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.work.WorkManager;
import androidx.work.impl.WorkManagerImpl;

import java.util.concurrent.TimeUnit;

/**
 * Reschedules alarms on BOOT_COMPLETED and other similar scenarios.
 */
public class RescheduleReceiver extends BroadcastReceiver {

    private static final String TAG = "RescheduleReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (Build.VERSION.SDK_INT >= WorkManagerImpl.MIN_JOB_SCHEDULER_API_LEVEL) {
            if (WorkManager.getInstance() == null) {
                // WorkManager has not already been initialized.
                Log.e(TAG,
                        "Cannot reschedule jobs. WorkManager needs to be initialized via a "
                                + "ContentProvider#onCreate() or an Application#onCreate().");
            } else {
                // This helps set up rescheduling of Jobs with JobScheduler. We are doing nothing
                // for 10 seconds, to give ForceStopRunnable a chance to reschedule.
                Handler handler = new Handler(Looper.getMainLooper());
                final PendingResult pendingResult = goAsync();
                handler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        pendingResult.finish();
                    }
                }, TimeUnit.SECONDS.toMillis(10));
            }
        } else {
            Intent reschedule = CommandHandler.createRescheduleIntent(context);
            context.startService(reschedule);
        }
    }
}
