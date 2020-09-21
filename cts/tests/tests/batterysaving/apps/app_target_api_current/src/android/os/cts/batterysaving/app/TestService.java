/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.os.cts.batterysaving.app;

import static android.os.cts.batterysaving.common.Values.KEY_REQUEST_FOREGROUND;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import java.util.concurrent.atomic.AtomicReference;

public class TestService extends Service {
    private static final String TAG = "TestService";

    public static final AtomicReference<Intent> LastStartIntent = new AtomicReference();

    private final String getNotificationChannelId() {
        return new ComponentName(this, TestService.class).toShortString();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        Log.d(TAG, "TestService.TestService: intent=" + intent);

        if (intent.getBooleanExtra(KEY_REQUEST_FOREGROUND, false)) {
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(new NotificationChannel(
                    getNotificationChannelId(), getNotificationChannelId(),
                    NotificationManager.IMPORTANCE_DEFAULT));
            Notification notification =
                    new Notification.Builder(getApplicationContext(), getNotificationChannelId())
                            .setContentTitle("FgService")
                            .setSmallIcon(android.R.drawable.ic_popup_sync)
                            .build();

            startForeground(1, notification);
        }

        LastStartIntent.set(intent);

        stopSelf();

        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
