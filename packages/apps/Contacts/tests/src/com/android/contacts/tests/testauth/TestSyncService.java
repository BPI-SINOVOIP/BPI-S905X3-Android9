/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.contacts.tests.testauth;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public abstract class TestSyncService extends Service {

    private static TestSyncAdapter sSyncAdapter;

    @Override
    public void onCreate() {
        if (sSyncAdapter == null) {
            sSyncAdapter = new TestSyncAdapter(getApplicationContext(), true);
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return sSyncAdapter.getSyncAdapterBinder();
    }

    public static class Basic extends TestSyncService {
    }
}
