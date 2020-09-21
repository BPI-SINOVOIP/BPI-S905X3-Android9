/*
 * Copyright (C) 2016 The Android Open Source Project
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

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.accounts.Account;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.Context;
import android.content.SyncResult;
import android.os.Bundle;

public class SyncAdapter extends AbstractThreadedSyncAdapter {
    private final Object mLock = new Object();
    private AbstractThreadedSyncAdapter mDelegate;

    public AbstractThreadedSyncAdapter setNewDelegate() {
        AbstractThreadedSyncAdapter delegate = mock(AbstractThreadedSyncAdapter.class);
        when(delegate.onUnsyncableAccount()).thenCallRealMethod();

        synchronized (mLock) {
            mDelegate = delegate;
        }

        return delegate;
    }

    public SyncAdapter(Context context, boolean autoInitialize) {
        super(context, autoInitialize);
    }

    private AbstractThreadedSyncAdapter getCopyOfDelegate() {
        synchronized (mLock) {
            return mDelegate;
        }
    }

    @Override
    public void onPerformSync(Account account, Bundle extras, String authority,
            ContentProviderClient provider, SyncResult syncResult) {
        AbstractThreadedSyncAdapter delegate = getCopyOfDelegate();

        if (delegate != null) {
            delegate.onPerformSync(account, extras, authority, provider, syncResult);
        }
    }

    @Override
    public boolean onUnsyncableAccount() {
        AbstractThreadedSyncAdapter delegate = getCopyOfDelegate();

        if (delegate == null) {
            return true;
        } else {
            return delegate.onUnsyncableAccount();
        }
    }
}
