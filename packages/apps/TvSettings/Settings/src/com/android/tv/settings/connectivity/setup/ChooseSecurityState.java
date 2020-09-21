/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.connectivity.setup;

import android.arch.lifecycle.ViewModelProviders;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.VisibleForTesting;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.WifiConfigHelper;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.util.List;

/**
 * State responsible for choosing security type.
 */
public class ChooseSecurityState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public ChooseSecurityState(FragmentActivity activity) {
        mActivity = activity;
    }

    @Override
    public void processForward() {
        mFragment = new ChooseSecurityFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new ChooseSecurityFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, false);
        }
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Fragment that needs user to select security type.
     */
    public static class ChooseSecurityFragment extends WifiConnectivityGuidedStepFragment {
        @VisibleForTesting
        static final long WIFI_SECURITY_TYPE_NONE = 100001;
        @VisibleForTesting
        static final long WIFI_SECURITY_TYPE_WEP = 100002;
        @VisibleForTesting
        static final long WIFI_SECURITY_TYPE_WPA = 100003;
        @VisibleForTesting
        static final long WIFI_SECURITY_TYPE_EAP = 100004;
        private StateMachine mStateMachine;
        private UserChoiceInfo mUserChoiceInfo;

        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.security_type),
                    null,
                    null,
                    null);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mUserChoiceInfo = ViewModelProviders
                    .of(getActivity())
                    .get(UserChoiceInfo.class);
            mStateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onCreateActions(List<GuidedAction> actions, Bundle savedInstanceState) {
            Context context = getActivity();
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_security_type_none)
                    .id(WIFI_SECURITY_TYPE_NONE)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_security_type_wep)
                    .id(WIFI_SECURITY_TYPE_WEP)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_security_type_wpa)
                    .id(WIFI_SECURITY_TYPE_WPA)
                    .build());
            actions.add(new GuidedAction.Builder(context)
                    .title(R.string.wifi_security_type_eap)
                    .id(WIFI_SECURITY_TYPE_EAP)
                    .build());
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == WIFI_SECURITY_TYPE_NONE) {
                mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_NONE);
            } else if (action.getId() == WIFI_SECURITY_TYPE_WEP) {
                mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_WEP);
            } else if (action.getId() == WIFI_SECURITY_TYPE_WPA) {
                mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_PSK);
            } else if (action.getId() == WIFI_SECURITY_TYPE_EAP) {
                mUserChoiceInfo.setWifiSecurity(AccessPoint.SECURITY_EAP);
            }
            WifiConfigHelper.setConfigKeyManagementBySecurity(
                    mUserChoiceInfo.getWifiConfiguration(),
                    mUserChoiceInfo.getWifiSecurity());
            if (action.getId() == WIFI_SECURITY_TYPE_NONE) {
                mStateMachine.getListener().onComplete(StateMachine.OPTIONS_OR_CONNECT);
            } else {
                mStateMachine.getListener().onComplete(StateMachine.PASSWORD);
            }
        }
    }
}
