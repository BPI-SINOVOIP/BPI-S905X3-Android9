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
package android.content.syncmanager.cts.app;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

/**
 * Service for the sync test.
 */
public class SyncManagerCtsSyncService extends Service {

    private static SyncManagerCtsSyncAdapter sAdapter;

    @Override
    public synchronized IBinder onBind(Intent intent) {
        if (sAdapter == null) {
            sAdapter = new SyncManagerCtsSyncAdapter(getApplicationContext());
        }
        return sAdapter.getSyncAdapterBinder();
    }
}
