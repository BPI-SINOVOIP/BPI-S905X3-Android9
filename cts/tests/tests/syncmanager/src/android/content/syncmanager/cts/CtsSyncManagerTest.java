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
package android.content.syncmanager.cts;

import static android.content.syncmanager.cts.common.Values.ACCOUNT_1_A;
import static android.content.syncmanager.cts.common.Values.APP1_AUTHORITY;
import static android.content.syncmanager.cts.common.Values.APP1_PACKAGE;

import static com.android.compatibility.common.util.BundleUtils.makeBundle;
import static com.android.compatibility.common.util.ConnectivityUtils.assertNetworkConnected;
import static com.android.compatibility.common.util.SettingsUtils.putGlobalSetting;
import static com.android.compatibility.common.util.SystemUtil.runCommandAndPrintOnLogcat;

import static junit.framework.TestCase.assertEquals;

import static org.junit.Assert.assertTrue;

import android.accounts.Account;
import android.app.usage.UsageStatsManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.AddAccount;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.ClearSyncInvocations;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.GetSyncInvocations;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.RemoveAllAccounts;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.SetResult;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Request.SetResult.Result;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.Response;
import android.content.syncmanager.cts.SyncManagerCtsProto.Payload.SyncInvocation;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import com.android.compatibility.common.util.AmUtils;
import com.android.compatibility.common.util.BatteryUtils;
import com.android.compatibility.common.util.OnFailureRule;
import com.android.compatibility.common.util.ParcelUtils;
import com.android.compatibility.common.util.SystemUtil;
import com.android.compatibility.common.util.TestUtils;
import com.android.compatibility.common.util.TestUtils.BooleanSupplierWithThrow;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.Statement;


// TODO Don't run if no network is available.

@LargeTest
@RunWith(AndroidJUnit4.class)
public class CtsSyncManagerTest {
    private static final String TAG = "CtsSyncManagerTest";

    public static final int DEFAULT_TIMEOUT_SECONDS = 30;

    public static final boolean DEBUG = false;
    private static final int TIMEOUT_MS = 10 * 60 * 1000;

    @Rule
    public final OnFailureRule mDumpOnFailureRule = new OnFailureRule(TAG) {
        @Override
        protected void onTestFailure(Statement base, Description description, Throwable t) {
            runCommandAndPrintOnLogcat(TAG, "dumpsys content");
            runCommandAndPrintOnLogcat(TAG, "dumpsys jobscheduler");
        }
    };

    protected final BroadcastRpc mRpc = new BroadcastRpc();

    Context mContext;
    ContentResolver mContentResolver;

    @Before
    public void setUp() throws Exception {
        assertNetworkConnected(InstrumentationRegistry.getContext());

        BatteryUtils.runDumpsysBatteryUnplug();

        AmUtils.setStandbyBucket(APP1_PACKAGE, UsageStatsManager.STANDBY_BUCKET_ACTIVE);

        mContext = InstrumentationRegistry.getContext();
        mContentResolver = mContext.getContentResolver();

        ContentResolver.setMasterSyncAutomatically(true);

        Thread.sleep(1000); // Don't make the system too busy...
    }

    @After
    public void tearDown() throws Exception {
        resetSyncConfig();
        BatteryUtils.runDumpsysBatteryReset();
    }

    private static void resetSyncConfig() {
        putGlobalSetting("sync_manager_constants", "null");
    }

    private static void writeSyncConfig(
            int initialSyncRetryTimeInSeconds,
            float retryTimeIncreaseFactor,
            int maxSyncRetryTimeInSeconds,
            int maxRetriesWithAppStandbyExemption) {
        putGlobalSetting("sync_manager_constants",
                "initial_sync_retry_time_in_seconds=" + initialSyncRetryTimeInSeconds + "," +
                "retry_time_increase_factor=" + retryTimeIncreaseFactor + "," +
                "max_sync_retry_time_in_seconds=" + maxSyncRetryTimeInSeconds + "," +
                "max_retries_with_app_standby_exemption=" + maxRetriesWithAppStandbyExemption);
    }

    /** Return the part of "dumpsys content" that's relevant to the current sync status. */
    private String getSyncDumpsys() {
        final String out = SystemUtil.runCommandAndExtractSection("dumpsys content",
                "^Active Syncs:.*", false,
                "^Sync Statistics", false);
        return out;
    }

    private void waitUntil(String message, BooleanSupplierWithThrow predicate) throws Exception {
        TestUtils.waitUntil(message, TIMEOUT_MS, predicate);
    }

    private void removeAllAccounts() throws Exception {
        mRpc.invoke(APP1_PACKAGE,
                rb -> rb.setRemoveAllAccounts(RemoveAllAccounts.newBuilder()));

        Thread.sleep(1000);

        AmUtils.waitForBroadcastIdle();

        waitUntil("Dumpsys still mentions " + ACCOUNT_1_A,
                () -> !getSyncDumpsys().contains(ACCOUNT_1_A.name));

        Thread.sleep(1000);
    }

    private void clearSyncInvocations(String packageName) throws Exception {
        mRpc.invoke(packageName,
                rb -> rb.setClearSyncInvocations(ClearSyncInvocations.newBuilder()));
    }

    private void addAccountAndLetInitialSyncRun(Account account, String authority)
            throws Exception {
        // Add the first account, which will trigger an initial sync.
        mRpc.invoke(APP1_PACKAGE,
                rb -> rb.setAddAccount(AddAccount.newBuilder().setName(account.name)));

        waitUntil("Syncable isn't initialized",
                () -> ContentResolver.getIsSyncable(account, authority) == 1);

        waitUntil("Periodic sync should set up",
                () -> ContentResolver.getPeriodicSyncs(account, authority).size() == 1);
        assertEquals("Periodic should be 24h",
                24 * 60 * 60, ContentResolver.getPeriodicSyncs(account, authority).get(0).period);
    }

    @Test
    public void testInitialSync() throws Exception {
        removeAllAccounts();

        mRpc.invoke(APP1_PACKAGE, rb -> rb.setClearSyncInvocations(
                ClearSyncInvocations.newBuilder()));

        // Add the first account, which will trigger an initial sync.
        addAccountAndLetInitialSyncRun(ACCOUNT_1_A, APP1_AUTHORITY);

        // Check the sync request parameters.

        Response res = mRpc.invoke(APP1_PACKAGE,
                rb -> rb.setGetSyncInvocations(GetSyncInvocations.newBuilder()));
        assertEquals(1, res.getSyncInvocations().getSyncInvocationsCount());

        SyncInvocation si = res.getSyncInvocations().getSyncInvocations(0);

        assertEquals(ACCOUNT_1_A.name, si.getAccountName());
        assertEquals(ACCOUNT_1_A.type, si.getAccountType());
        assertEquals(APP1_AUTHORITY, si.getAuthority());

        Bundle extras = ParcelUtils.fromBytes(si.getExtras().toByteArray());
        assertTrue(extras.getBoolean(ContentResolver.SYNC_EXTRAS_INITIALIZE));
    }

    @Test
    public void testSoftErrorRetriesActiveApp() throws Exception {
        removeAllAccounts();

        // Let the initial sync happen.
        addAccountAndLetInitialSyncRun(ACCOUNT_1_A, APP1_AUTHORITY);

        writeSyncConfig(2, 1, 2, 3);

        clearSyncInvocations(APP1_PACKAGE);

        AmUtils.setStandbyBucket(APP1_PACKAGE, UsageStatsManager.STANDBY_BUCKET_ACTIVE);

        // Set soft error.
        mRpc.invoke(APP1_PACKAGE, rb ->
                rb.setSetResult(SetResult.newBuilder().setResult(Result.SOFT_ERROR)));

        Bundle b = makeBundle(
                "testSoftErrorRetriesActiveApp", true,
                ContentResolver.SYNC_EXTRAS_IGNORE_SETTINGS, true);

        ContentResolver.requestSync(ACCOUNT_1_A, APP1_AUTHORITY, b);

        // First sync + 3 retries == 4, so should be called more than 4 times.
        // But it's active, so it should retry more than that.
        waitUntil("Should retry more than 3 times.", () -> {
            final Response res = mRpc.invoke(APP1_PACKAGE,
                    rb -> rb.setGetSyncInvocations(GetSyncInvocations.newBuilder()));
            final int calls =  res.getSyncInvocations().getSyncInvocationsCount();
            Log.i(TAG, "NumSyncInvocations=" + calls);
            return calls > 4; // Arbitrarily bigger than 4.
        });
    }

    // WIP This test doesn't work yet.
//    @Test
//    public void testSoftErrorRetriesFrequentApp() throws Exception {
//        runTest(() -> {
//            removeAllAccounts();
//
//            // Let the initial sync happen.
//            addAccountAndLetInitialSyncRun(ACCOUNT_1_A, APP1_AUTHORITY);
//
//            writeSyncConfig(2, 1, 2, 3);
//
//            clearSyncInvocations(APP1_PACKAGE);
//
//            AmUtils.setStandbyBucket(APP1_PACKAGE, UsageStatsManager.STANDBY_BUCKET_FREQUENT);
//
//            // Set soft error.
//            mRpc.invoke(APP1_PACKAGE, rb ->
//                    rb.setSetResult(SetResult.newBuilder().setResult(Result.SOFT_ERROR)));
//
//            Bundle b = makeBundle(
//                    "testSoftErrorRetriesFrequentApp", true,
//                    ContentResolver.SYNC_EXTRAS_IGNORE_SETTINGS, true);
//
//            ContentResolver.requestSync(ACCOUNT_1_A, APP1_AUTHORITY, b);
//
//            waitUntil("Should retry more than 3 times.", () -> {
//                final Response res = mRpc.invoke(APP1_PACKAGE,
//                        rb -> rb.setGetSyncInvocations(GetSyncInvocations.newBuilder()));
//                final int calls =  res.getSyncInvocations().getSyncInvocationsCount();
//                Log.i(TAG, "NumSyncInvocations=" + calls);
//                return calls >= 4; // First sync + 3 retries == 4, so at least 4 times.
//            });
//
//            Thread.sleep(10_000);
//
//            // One more retry is okay because of how the job scheduler throttle jobs, but no further.
//            final Response res = mRpc.invoke(APP1_PACKAGE,
//                    rb -> rb.setGetSyncInvocations(GetSyncInvocations.newBuilder()));
//            final int calls =  res.getSyncInvocations().getSyncInvocationsCount();
//            assertTrue("# of syncs must be equal or less than 5, but was " + calls, calls <= 5);
//        });
//    }
}
