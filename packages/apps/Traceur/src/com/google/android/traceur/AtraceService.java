/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.traceur;

import com.google.android.collect.Sets;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.preference.PreferenceManager;

import java.io.File;

public class AtraceService extends IntentService {

    private static String INTENT_ACTION_START_TRACING = "com.android.traceur.START_TRACING";
    private static String INTENT_ACTION_STOP_TRACING = "com.android.traceur.STOP_TRACING";

    private static String INTENT_EXTRA_FILENAME = "filename";
    private static String INTENT_EXTRA_TAGS= "tags";
    private static String INTENT_EXTRA_BUFFER = "buffer";
    private static String INTENT_EXTRA_APPS = "apps";

    private static int TRACE_NOTIFICATION = 1;
    private static int SAVING_TRACE_NOTIFICATION = 2;

    public static void startTracing(final Context context,
            String tags, int bufferSizeKb, boolean apps) {
        Intent intent = new Intent(context, AtraceService.class);
        intent.setAction(INTENT_ACTION_START_TRACING);
        intent.putExtra(INTENT_EXTRA_TAGS, tags);
        intent.putExtra(INTENT_EXTRA_BUFFER, bufferSizeKb);
        intent.putExtra(INTENT_EXTRA_APPS, apps);
        context.startService(intent);
    }

    public static void stopTracing(final Context context) {
        Intent intent = new Intent(context, AtraceService.class);
        intent.setAction(INTENT_ACTION_STOP_TRACING);
        intent.putExtra(INTENT_EXTRA_FILENAME, AtraceUtils.getOutputFilename());
        context.startService(intent);
    }

    public AtraceService() {
        super("AtraceService");
        setIntentRedelivery(true);
    }

    @Override
    public void onHandleIntent(Intent intent) {
        if (intent.getAction().equals(INTENT_ACTION_START_TRACING)) {
            startTracingInternal(intent.getStringExtra(INTENT_EXTRA_TAGS),
                intent.getIntExtra(INTENT_EXTRA_BUFFER,
                    Integer.parseInt(getApplicationContext()
                        .getString(R.string.default_buffer_size))),
                intent.getBooleanExtra(INTENT_EXTRA_APPS, false));
        } else if (intent.getAction().equals(INTENT_ACTION_STOP_TRACING)) {
            stopTracingInternal(intent.getStringExtra(INTENT_EXTRA_FILENAME));
        }
    }

    private void startTracingInternal(String tags, int bufferSizeKb, boolean appTracing) {
        Context context = getApplicationContext();
        Intent stopIntent = new Intent(Receiver.STOP_ACTION,
            null, context, Receiver.class);

        String title = context.getString(R.string.trace_is_being_recorded);
        String msg = context.getString(R.string.tap_to_stop_tracing);

        Notification notification =
            new Notification.Builder(context, Receiver.NOTIFICATION_CHANNEL)
                .setSmallIcon(R.drawable.stat_sys_adb)
                .setContentTitle(title)
                .setTicker(title)
                .setContentText(msg)
                .setContentIntent(
                    PendingIntent.getBroadcast(context, 0, stopIntent, 0))
                .setOngoing(true)
                .setLocalOnly(true)
                .setColor(getColor(
                    com.android.internal.R.color.system_notification_accent_color))
                .build();

        startForeground(TRACE_NOTIFICATION, notification);

        if (AtraceUtils.atraceStart(tags, bufferSizeKb, appTracing)) {
            stopForeground(Service.STOP_FOREGROUND_DETACH);
        } else {
            // Starting the trace was unsuccessful, so ensure that tracing
            // is stopped and the preference is reset.
            AtraceUtils.atraceStop();
            PreferenceManager.getDefaultSharedPreferences(context)
                .edit().putBoolean(context.getString(R.string.pref_key_tracing_on),
                        false).apply();
            QsService.updateTile();
            stopForeground(Service.STOP_FOREGROUND_REMOVE);
        }
    }

    private void stopTracingInternal(String outputFilename) {
        NotificationManager notificationManager =
            getSystemService(NotificationManager.class);

        Notification notification =
            new Notification.Builder(this, Receiver.NOTIFICATION_CHANNEL)
                .setSmallIcon(R.drawable.stat_sys_adb)
                .setContentTitle(getString(R.string.saving_trace))
                .setTicker(getString(R.string.saving_trace))
                .setLocalOnly(true)
                .setProgress(1, 0, true)
                .setColor(getColor(
                    com.android.internal.R.color.system_notification_accent_color))
                .build();

        startForeground(SAVING_TRACE_NOTIFICATION, notification);

        notificationManager.cancel(TRACE_NOTIFICATION);

        File file = AtraceUtils.getOutputFile(outputFilename);

        if (AtraceUtils.atraceDump(file)) {
            FileSender.postNotification(getApplicationContext(), file);
        }

        stopForeground(Service.STOP_FOREGROUND_REMOVE);
    }
}
