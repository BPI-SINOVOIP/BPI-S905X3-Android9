/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
package com.android.settings.applications;

import android.content.Context;
import android.os.RemoteException;
import android.os.ServiceManager;
import com.android.internal.telephony.ISms;
import com.android.internal.telephony.SmsUsageMonitor;
import com.android.settingslib.applications.ApplicationsState;
import com.android.settingslib.applications.ApplicationsState.AppEntry;
import com.android.settingslib.applications.ApplicationsState.AppFilter;

import java.util.ArrayList;

/**
 * Connects the info provided by ApplicationsState and premium sms permission state.
 */
public class AppStateSmsPremBridge extends AppStateBaseBridge {

    private final Context mContext;
    private final ISms mSmsManager;

    public AppStateSmsPremBridge(Context context, ApplicationsState appState, Callback callback) {
        super(appState, callback);
        mContext = context;
        mSmsManager = ISms.Stub.asInterface(ServiceManager.getService("isms"));
    }

    @Override
    protected void loadAllExtraInfo() {
        ArrayList<AppEntry> apps = mAppSession.getAllApps();
        final int N = apps.size();
        for (int i = 0; i < N; i++) {
            AppEntry app = apps.get(i);
            updateExtraInfo(app, app.info.packageName, app.info.uid);
        }
    }

    @Override
    protected void updateExtraInfo(AppEntry app, String pkg, int uid) {
        app.extraInfo = getState(pkg);
    }

    public SmsState getState(String pkg) {
        final SmsState state = new SmsState();
        state.smsState = getSmsState(pkg);
        return state;
    }

    private int getSmsState(String pkg) {
        try {
            return mSmsManager.getPremiumSmsPermission(pkg);
        } catch (RemoteException e) {
            return SmsUsageMonitor.PREMIUM_SMS_PERMISSION_UNKNOWN;
        }
    }

    public void setSmsState(String pkg, int state) {
        try {
            mSmsManager.setPremiumSmsPermission(pkg, state);
        } catch (RemoteException e) {
        }
    }

    public static class SmsState {
        public int smsState;

        public boolean isGranted() {
            return smsState == SmsUsageMonitor.PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW;
        }
    }

    public static final AppFilter FILTER_APP_PREMIUM_SMS = new AppFilter() {
        @Override
        public void init() {
        }

        @Override
        public boolean filterApp(AppEntry info) {
            return info.extraInfo instanceof SmsState && ((SmsState) info.extraInfo).smsState
                    != SmsUsageMonitor.PREMIUM_SMS_PERMISSION_UNKNOWN;
        }
    };
}
