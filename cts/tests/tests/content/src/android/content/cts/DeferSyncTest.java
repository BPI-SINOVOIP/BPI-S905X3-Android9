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

package android.content.cts;

import static com.android.cts.content.Utils.ALWAYS_SYNCABLE_AUTHORITY;
import static com.android.cts.content.Utils.NOT_ALWAYS_SYNCABLE_AUTHORITY;
import static com.android.cts.content.Utils.SYNC_TIMEOUT_MILLIS;
import static com.android.cts.content.Utils.allowSyncAdapterRunInBackgroundAndDataInBackground;
import static com.android.cts.content.Utils.disallowSyncAdapterRunInBackgroundAndDataInBackground;
import static com.android.cts.content.Utils.hasDataConnection;
import static com.android.cts.content.Utils.requestSync;
import static com.android.cts.content.Utils.withAccount;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;
import static org.mockito.ArgumentCaptor.forClass;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentResolver;
import android.os.Bundle;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;

import com.android.cts.content.AlwaysSyncableSyncService;
import com.android.cts.content.FlakyTestRule;
import com.android.cts.content.NotAlwaysSyncableSyncService;
import com.android.cts.content.StubActivity;
import com.android.cts.content.Utils;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class DeferSyncTest {
    @Rule
    public final TestRule flakyTestRule = new FlakyTestRule(3);

    @Rule
    public final ActivityTestRule<StubActivity> activity = new ActivityTestRule(StubActivity.class);

    @Before
    public void setUp() throws Exception {
        allowSyncAdapterRunInBackgroundAndDataInBackground();
    }

    @After
    public void tearDown() throws Exception {
        disallowSyncAdapterRunInBackgroundAndDataInBackground();
    }

    @Test
    public void noSyncsWhenDeferred() throws Exception {
        assumeTrue(hasDataConnection());

        AbstractThreadedSyncAdapter notAlwaysSyncableAdapter =
                NotAlwaysSyncableSyncService.getInstance(activity.getActivity()).setNewDelegate();
        AbstractThreadedSyncAdapter alwaysSyncableAdapter =
                AlwaysSyncableSyncService.getInstance(activity.getActivity()).setNewDelegate();

        when(alwaysSyncableAdapter.onUnsyncableAccount()).thenReturn(false);
        when(notAlwaysSyncableAdapter.onUnsyncableAccount()).thenReturn(false);

        try (Utils.ClosableAccount ignored = withAccount(activity.getActivity())) {
            requestSync(NOT_ALWAYS_SYNCABLE_AUTHORITY);
            requestSync(ALWAYS_SYNCABLE_AUTHORITY);

            Thread.sleep(SYNC_TIMEOUT_MILLIS);

            verify(notAlwaysSyncableAdapter, atLeast(1)).onUnsyncableAccount();
            verify(notAlwaysSyncableAdapter, never()).onPerformSync(any(), any(), any(), any(),
                    any());

            verify(alwaysSyncableAdapter, atLeast(1)).onUnsyncableAccount();
            verify(alwaysSyncableAdapter, never()).onPerformSync(any(), any(), any(), any(), any());
        }
    }

    @Test
    public void deferSyncAndMakeSyncable() throws Exception {
        assumeTrue(hasDataConnection());

        AbstractThreadedSyncAdapter adapter = NotAlwaysSyncableSyncService.getInstance(
                activity.getActivity()).setNewDelegate();
        when(adapter.onUnsyncableAccount()).thenReturn(false);

        try (Utils.ClosableAccount account = withAccount(activity.getActivity())) {
            verify(adapter, timeout(SYNC_TIMEOUT_MILLIS)).onUnsyncableAccount();

            // Enable the adapter by making the account/provider syncable
            ContentResolver.setIsSyncable(account.account, NOT_ALWAYS_SYNCABLE_AUTHORITY, 1);
            requestSync(NOT_ALWAYS_SYNCABLE_AUTHORITY);

            ArgumentCaptor<Bundle> extrasCaptor = forClass(Bundle.class);
            verify(adapter, timeout(SYNC_TIMEOUT_MILLIS)).onPerformSync(any(),
                    extrasCaptor.capture(), any(), any(), any());

            // As the adapter is made syncable, we should not get an initialization sync
            assertFalse(
                    extrasCaptor.getValue().containsKey(ContentResolver.SYNC_EXTRAS_INITIALIZE));
        }
    }

    @Test
    public void deferSyncAndReportIsReady() throws Exception {
        assumeTrue(hasDataConnection());

        AbstractThreadedSyncAdapter adapter = NotAlwaysSyncableSyncService.getInstance(
                activity.getActivity()).setNewDelegate();
        when(adapter.onUnsyncableAccount()).thenReturn(false);

        try (Utils.ClosableAccount ignored = withAccount(activity.getActivity())) {
            verify(adapter, timeout(SYNC_TIMEOUT_MILLIS)).onUnsyncableAccount();

            // Enable the adapter by returning true from onNewAccount
            when(adapter.onUnsyncableAccount()).thenReturn(true);
            requestSync(NOT_ALWAYS_SYNCABLE_AUTHORITY);
            verify(adapter, atLeast(1)).onUnsyncableAccount();

            ArgumentCaptor<Bundle> extrasCaptor = forClass(Bundle.class);
            verify(adapter, timeout(SYNC_TIMEOUT_MILLIS)).onPerformSync(any(),
                    extrasCaptor.capture(), any(), any(), any());

            // As the adapter is not syncable yet, we should get an initialization sync
            assertTrue(extrasCaptor.getValue().getBoolean(ContentResolver.SYNC_EXTRAS_INITIALIZE));
        }
    }

    @Test
    public void deferSyncAndReportIsReadyAlwaysSyncable() throws Exception {
        assumeTrue(hasDataConnection());

        AbstractThreadedSyncAdapter adapter = AlwaysSyncableSyncService.getInstance(
                activity.getActivity()).setNewDelegate();
        when(adapter.onUnsyncableAccount()).thenReturn(false);

        try (Utils.ClosableAccount ignored = withAccount(activity.getActivity())) {
            verify(adapter, timeout(SYNC_TIMEOUT_MILLIS)).onUnsyncableAccount();

            // Enable the adapter by returning true from onNewAccount
            when(adapter.onUnsyncableAccount()).thenReturn(true);
            requestSync(ALWAYS_SYNCABLE_AUTHORITY);
            verify(adapter, atLeast(1)).onUnsyncableAccount();

            ArgumentCaptor<Bundle> extrasCaptor = forClass(Bundle.class);
            verify(adapter, timeout(SYNC_TIMEOUT_MILLIS)).onPerformSync(any(),
                    extrasCaptor.capture(), any(), any(), any());

            // The adapter is always syncable, hence there is no init sync
            assertFalse(
                    extrasCaptor.getValue().containsKey(ContentResolver.SYNC_EXTRAS_INITIALIZE));
        }
    }

    @Test
    public void onNewAccountForEachAccount() throws Exception {
        assumeTrue(hasDataConnection());

        AbstractThreadedSyncAdapter adapter = NotAlwaysSyncableSyncService.getInstance(
                activity.getActivity()).setNewDelegate();
        when(adapter.onUnsyncableAccount()).thenReturn(true, false);

        try (Utils.ClosableAccount account1 = withAccount(activity.getActivity())) {
            try (Utils.ClosableAccount account2 = withAccount(activity.getActivity())) {
                verify(adapter, timeout(SYNC_TIMEOUT_MILLIS).atLeast(2)).onUnsyncableAccount();

                // Exactly account should have gotten the init-sync. No further syncs happen as
                // onNewAccount returns false again.
                ArgumentCaptor<Bundle> extrasCaptor = forClass(Bundle.class);
                verify(adapter, timeout(SYNC_TIMEOUT_MILLIS)).onPerformSync(any(),
                        extrasCaptor.capture(), any(), any(), any());
                assertTrue(
                        extrasCaptor.getValue().getBoolean(ContentResolver.SYNC_EXTRAS_INITIALIZE));
            }
        }
    }

}
