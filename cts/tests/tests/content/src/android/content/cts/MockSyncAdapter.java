/*
 * Copyright (C) 2010 The Android Open Source Project
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

import android.accounts.Account;
import android.content.ContentResolver;
import android.content.ISyncAdapter;
import android.content.ISyncAdapterUnsyncableAccountCallback;
import android.content.ISyncContext;
import android.os.Bundle;
import android.os.RemoteException;

import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

public class MockSyncAdapter extends ISyncAdapter.Stub {

    private static MockSyncAdapter sSyncAdapter = null;

    private volatile ArrayList<Account> mAccounts = new ArrayList<Account>();
    private volatile String mAuthority;
    private volatile Bundle mExtras;
    private volatile boolean mInitialized;
    private volatile boolean mStartSync;
    private volatile boolean mCancelSync;
    private volatile CountDownLatch mLatch;

    public ArrayList<Account> getAccounts() {
        return mAccounts;
    }

    public String getAuthority() {
        return mAuthority;
    }

    public Bundle getExtras() {
        return mExtras;
    }

    public boolean isInitialized() {
        return mInitialized;
    }

    public boolean isStartSync() {
        return mStartSync;
    }

    public boolean isCancelSync() {
        return mCancelSync;
    }

    public void clearData() {
        mAccounts.clear();
        mAuthority = null;
        mExtras = null;
        mInitialized = false;
        mStartSync = false;
        mCancelSync = false;
        mLatch = null;
    }

    public void setLatch(CountDownLatch mLatch) {
        this.mLatch = mLatch;
    }

    @Override
    public void onUnsyncableAccount(ISyncAdapterUnsyncableAccountCallback cb)
            throws RemoteException {
        cb.onUnsyncableAccountDone(true);
    }

    public void startSync(ISyncContext syncContext, String authority, Account account,
            Bundle extras) throws RemoteException {

        mAccounts.add(account);
        mAuthority = authority;
        mExtras = extras;

        if (null != extras && extras.getBoolean(ContentResolver.SYNC_EXTRAS_INITIALIZE)) {
            mInitialized = true;
            mStartSync = false;
            mCancelSync = false;
        } else {
            mInitialized = false;
            mStartSync = true;
            mCancelSync = false;
        }

        countDownLatch();
    }

    public void cancelSync(ISyncContext syncContext) throws RemoteException {
        mAccounts.clear();
        mAuthority = null;
        mExtras = null;

        mInitialized = false;
        mStartSync = false;
        mCancelSync = true;

        countDownLatch();
    }

    public void initialize(android.accounts.Account account, java.lang.String authority)
            throws android.os.RemoteException {

        mAccounts.add(account);
        mAuthority = authority;

        mInitialized = true;
        mStartSync = false;
        mCancelSync = false;

        countDownLatch();
    }

    private void countDownLatch() {
        final CountDownLatch latch = mLatch;
        if (latch != null) {
            latch.countDown();
        }
    }

    public static MockSyncAdapter getMockSyncAdapter() {
        if (null == sSyncAdapter) {
            sSyncAdapter = new MockSyncAdapter();
        }
        return sSyncAdapter;
    }
}
