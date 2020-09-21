/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.server.cts.device.statsd;

import android.accounts.Account;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SyncResult;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;

import org.junit.Assert;

import java.util.concurrent.CountDownLatch;

import javax.annotation.concurrent.GuardedBy;

/**
 * Sync adapter for the sync test.
 */
public class StatsdSyncAdapter extends AbstractThreadedSyncAdapter {
    private static final String TAG = "AtomTestsSyncAdapter";

    private static final int TIMEOUT_SECONDS = 60 * 2;

    private static CountDownLatch sLatch;

    private static final Object sLock = new Object();


    public StatsdSyncAdapter(Context context) {
        // No need for auto-initialization because we set isSyncable in the test anyway.
        super(context, /* autoInitialize= */ false);
    }

    @Override
    public void onPerformSync(Account account, Bundle extras, String authority,
            ContentProviderClient provider, SyncResult syncResult) {
        try {
            Thread.sleep(500);
        } catch (InterruptedException e) {
        }
        synchronized (sLock) {
            Log.i(TAG, "onPerformSync");
            sLock.notifyAll();
            if (sLatch != null) {
                sLatch.countDown();
            } else {
                Log.w(TAG, "sLatch is null, resetCountDownLatch probably should have been called");
            }
        }
    }

    /**
     * Request a sync on the given account, and wait for it.
     */
    public static void requestSync(Account account) throws Exception {
        final Bundle extras = new Bundle();
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_EXPEDITED, true);
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_IGNORE_BACKOFF, true);
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_IGNORE_SETTINGS, true);

        ContentResolver.requestSync(account, StatsdProvider.AUTHORITY, extras);
    }

    public static synchronized CountDownLatch resetCountDownLatch() {
        sLatch = new CountDownLatch(1);
        return sLatch;
    }
}
