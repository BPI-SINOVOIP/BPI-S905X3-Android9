/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.cts.verifier.notifications;


import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.net.Uri;
import android.service.notification.ConditionProviderService;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class MockConditionProvider extends ConditionProviderService {
    static final String TAG = "MockConditionProvider";

    public static final ComponentName COMPONENT_NAME =
            new ComponentName("com.android.cts.verifier", MockConditionProvider.class.getName());
    static final String PATH = "mock_cp";
    static final String QUERY = "query_item";

    private ArrayList<Uri> mSubscriptions = new ArrayList<>();
    private boolean mConnected = false;
    private BroadcastReceiver mReceiver;
    private static MockConditionProvider sConditionProviderInstance = null;

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "created");

        mSubscriptions = new ArrayList<>();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mConnected = false;
        if (mReceiver != null) {
            unregisterReceiver(mReceiver);
        }
        mReceiver = null;
        Log.d(TAG, "destroyed");
        sConditionProviderInstance = null;
    }

    public boolean isConnected() {
        return mConnected;
    }

    public static MockConditionProvider getInstance() {
        return sConditionProviderInstance;
    }

    public void resetData() {
        mSubscriptions.clear();
    }

    public List<Uri> getSubscriptions() {
        return mSubscriptions;
    }

    public static Uri toConditionId(String queryValue) {
        return new Uri.Builder().scheme("scheme")
                .appendPath(PATH)
                .appendQueryParameter(QUERY, queryValue)
                .build();
    }

    @Override
    public void onConnected() {
        Log.d(TAG, "connected");
        mConnected = true;
        sConditionProviderInstance = this;
    }

    @Override
    public void onSubscribe(Uri conditionId) {
        Log.d(TAG, "subscribed to " + conditionId);
        mSubscriptions.add(conditionId);
    }

    @Override
    public void onUnsubscribe(Uri conditionId) {
        Log.d(TAG, "unsubscribed from " + conditionId);
        mSubscriptions.remove(conditionId);
    }
}
