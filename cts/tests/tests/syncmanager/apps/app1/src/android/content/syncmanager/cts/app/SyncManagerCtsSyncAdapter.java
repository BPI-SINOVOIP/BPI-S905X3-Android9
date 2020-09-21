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

import static org.mockito.Mockito.mock;

import android.accounts.Account;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SyncResult;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.SetResult;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.SetResult.Result;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.SyncInvocation;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;

import com.android.compatibility.common.util.ParcelUtils;

import com.google.protobuf.ByteString;

import java.util.ArrayList;
import java.util.List;

/**
 * Sync adapter for the sync test.
 */
public class SyncManagerCtsSyncAdapter extends AbstractThreadedSyncAdapter {
    private static final String TAG = "SyncManagerCtsSyncAdapter";

    public SyncManagerCtsSyncAdapter(Context context) {
        super(context, /* autoInitialize= */ false);
    }

    private static final Object sLock = new Object();

    private static List<SyncInvocation> sSyncInvocations = new ArrayList<>();

    private static SetResult sResult;

    public interface SyncInterceptor {
        SyncResult onPerformSync(Account account, Bundle extras, String authority,
                SyncResult syncResult);
    }

    public static final SyncInterceptor sSyncInterceptor = mock(SyncInterceptor.class);

    @Override
    public void onPerformSync(Account account, Bundle extras, String authority,
            ContentProviderClient provider, SyncResult syncResult) {
        try {
            extras.size(); // Force unparcel extras.
            Log.i(TAG, "onPerformSync: account=" + account + " authority=" + authority
                    + " extras=" + extras);

            synchronized (sSyncInvocations) {
                sSyncInvocations.add(SyncInvocation.newBuilder()
                        .setTime(SystemClock.elapsedRealtime())
                        .setAccountName(account.name)
                        .setAccountType(account.type)
                        .setAuthority(authority)
                        .setExtras(ByteString.copyFrom(ParcelUtils.toBytes(extras))).build());
            }

            if (extras.getBoolean(ContentResolver.SYNC_EXTRAS_INITIALIZE)) {
                getContext().getContentResolver().setIsSyncable(account, authority, 1);
            }

            sSyncInterceptor.onPerformSync(account, extras, authority, syncResult);

            synchronized (sLock) {
                if ((sResult == null) || sResult.getResult() == Result.OK) {
                    syncResult.stats.numInserts++;

                } else if (sResult.getResult() == Result.SOFT_ERROR) {
                    syncResult.stats.numIoExceptions++;

                } else if (sResult.getResult() == Result.HARD_ERROR) {
                    syncResult.stats.numAuthExceptions++;
                }
            }

        } catch (Throwable th) {
            Log.e(TAG, "Exception in onPerformSync", th);
        }
        Log.i(TAG, "onPerformSyncFinishing: account=" + account + " authority=" + authority
                + " extras=" + extras + " result=" + syncResult);
    }

    public static List<SyncInvocation> getSyncInvocations() {
        synchronized (sLock) {
            return new ArrayList<>(sSyncInvocations);
        }
    }

    public static void clearSyncInvocations() {
        synchronized (sLock) {
            sSyncInvocations.clear();
        }
    }

    public static void setResult(SetResult result) {
        synchronized (sLock) {
            sResult = result;
        }
    }
}
