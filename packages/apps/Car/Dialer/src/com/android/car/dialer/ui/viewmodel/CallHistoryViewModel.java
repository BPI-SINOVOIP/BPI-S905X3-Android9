package com.android.car.dialer.ui.viewmodel;
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
import android.app.Application;
import android.arch.lifecycle.AndroidViewModel;
import android.arch.lifecycle.LiveData;
import android.content.Context;
import android.support.annotation.NonNull;

import com.android.car.dialer.livedata.CallHistoryLiveData;
import com.android.car.dialer.livedata.MissedCallHistoryLiveData;
import com.android.car.dialer.ui.CallLogListingTask;

import java.util.List;

/**
 * View model for CallHistoryFragment which provides call history live data.
 */
public class CallHistoryViewModel extends AndroidViewModel {
    private CallHistoryLiveData mCallHistoryLiveData;
    private MissedCallHistoryLiveData mMissedCallHistoryLiveData;

    private Context mContext;

    public CallHistoryViewModel(
            @NonNull Application application) {
        super(application);
        mContext = application;
    }

    public LiveData<List<CallLogListingTask.CallLogItem>> getCallHistory() {
        if (mCallHistoryLiveData == null) {
            mCallHistoryLiveData = new CallHistoryLiveData(mContext);
        }
        return mCallHistoryLiveData;
    }

    public LiveData<List<CallLogListingTask.CallLogItem>> getMissedCallHistory() {
        if (mMissedCallHistoryLiveData == null) {
            mMissedCallHistoryLiveData = new MissedCallHistoryLiveData(mContext);
        }

        return mMissedCallHistoryLiveData;
    }
}
