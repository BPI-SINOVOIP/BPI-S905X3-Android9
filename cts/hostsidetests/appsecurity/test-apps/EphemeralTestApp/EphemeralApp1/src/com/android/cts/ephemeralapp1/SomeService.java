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

package com.android.cts.ephemeralapp1;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import java.util.function.IntConsumer;

public class SomeService extends Service {
    private static IntConsumer sCallback;

    public static void setOnStartCommandCallback(IntConsumer callback) {
        sCallback = callback;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        final Notification.Builder builder = new Notification.Builder(this, "foo")
                .setSmallIcon(android.R.drawable.sym_def_app_icon)
                .setContentTitle("foo")
                .setWhen(System.currentTimeMillis())
                .setOngoing(true);
        try {
            startForeground(1, builder.build());
            stopSelf(startId);
        } catch (Exception e) {
            sCallback.accept(0);
            return START_NOT_STICKY;
        }
        sCallback.accept(1);
        return START_NOT_STICKY;
    }
}
