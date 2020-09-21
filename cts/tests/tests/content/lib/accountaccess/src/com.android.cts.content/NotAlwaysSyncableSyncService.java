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
package com.android.cts.content;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;

public class NotAlwaysSyncableSyncService extends Service {
    private static final Object sLock = new Object();
    private static SyncAdapter sInstance;

    public static SyncAdapter getInstance(Context context) {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new SyncAdapter(context.getApplicationContext(), false);
            }
        }

        return sInstance;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return getInstance(this).getSyncAdapterBinder();
    }
}
