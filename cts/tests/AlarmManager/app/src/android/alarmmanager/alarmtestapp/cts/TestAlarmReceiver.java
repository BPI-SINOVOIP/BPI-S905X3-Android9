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

package android.alarmmanager.alarmtestapp.cts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class TestAlarmReceiver extends BroadcastReceiver{
    private static final String TAG = TestAlarmReceiver.class.getSimpleName();
    private static final String PACKAGE_NAME = "android.alarmmanager.alarmtestapp.cts";
    public static final String ACTION_REPORT_ALARM_EXPIRED = PACKAGE_NAME + ".action.ALARM_EXPIRED";
    public static final String EXTRA_ALARM_COUNT = PACKAGE_NAME + ".extra.ALARM_COUNT";

    @Override
    public void onReceive(Context context, Intent intent) {
        final int count = intent.getIntExtra(Intent.EXTRA_ALARM_COUNT, 1);
        Log.d(TAG, "Alarm expired " + count + " times");
        final Intent reportAlarmIntent = new Intent(ACTION_REPORT_ALARM_EXPIRED);
        reportAlarmIntent.putExtra(EXTRA_ALARM_COUNT, count);
        reportAlarmIntent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY);
        reportAlarmIntent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        context.sendBroadcast(reportAlarmIntent);
    }
}
