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
package com.android.car.dialer.livedata;

import android.arch.lifecycle.LiveData;
import android.content.ContentResolver;
import android.content.Context;
import android.content.CursorLoader;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;

import com.android.car.dialer.telecom.PhoneLoader;
import com.android.car.dialer.ui.CallLogListingTask;

import java.util.List;

/**
 * Live data which loads call history.
 */
public class CallHistoryLiveData extends LiveData<List<CallLogListingTask.CallLogItem>> {

    private final Context mContext;
    private final ContentResolver mContentResolver;
    private CursorLoader mCursorLoader;
    private CallLogContentObserver mCallLogContentObserver = new CallLogContentObserver(
            new Handler());

    public CallHistoryLiveData(Context context) {
        this.mContext = context;
        mContentResolver = context.getContentResolver();
    }

    @Override
    protected void onActive() {
        super.onActive();
        mCursorLoader = PhoneLoader.registerCallObserver(getCallHistoryFilterType(),
                mContext,
                (loader, cursor) -> {
                    CallLogListingTask task = new CallLogListingTask(mContext, cursor,
                            this::setValue);
                    task.execute();
                });

        mContentResolver.registerContentObserver(mCursorLoader.getUri(),
                false, mCallLogContentObserver);
    }

    @Override
    protected void onInactive() {
        super.onInactive();
        mContentResolver.unregisterContentObserver(mCallLogContentObserver);
    }

    protected int getCallHistoryFilterType() {
        return PhoneLoader.CALL_TYPE_ALL;
    }

    /**
     * A {@link ContentObserver} that is responsible for reloading the user's recent calls.
     */
    private class CallLogContentObserver extends ContentObserver {
        public CallLogContentObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange) {
            onChange(selfChange, null);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            mCursorLoader.startLoading();
        }
    }
}
