/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.connectivity;

import android.arch.lifecycle.ViewModelProviders;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiConfiguration;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.setup.AddStartState;
import com.android.tv.settings.connectivity.setup.AdvancedWifiOptionsFlow;
import com.android.tv.settings.connectivity.setup.ConnectAuthFailureState;
import com.android.tv.settings.connectivity.setup.ConnectFailedState;
import com.android.tv.settings.connectivity.setup.ConnectRejectedByApState;
import com.android.tv.settings.connectivity.setup.ConnectState;
import com.android.tv.settings.connectivity.setup.ConnectTimeOutState;
import com.android.tv.settings.connectivity.setup.EnterPasswordState;
import com.android.tv.settings.connectivity.setup.KnownNetworkState;
import com.android.tv.settings.connectivity.setup.OptionsOrConnectState;
import com.android.tv.settings.connectivity.setup.SuccessState;
import com.android.tv.settings.connectivity.setup.UserChoiceInfo;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.connectivity.util.WifiSecurityUtil;
import com.android.tv.settings.core.instrumentation.InstrumentedActivity;

/**
 * Add a wifi network where we already know the ssid/security; normal post-install settings.
 */
public class WifiConnectionActivity extends InstrumentedActivity implements
        State.FragmentChangeListener {
    private static final String TAG = "WifiConnectionActivity";

    private static final String EXTRA_WIFI_SSID = "wifi_ssid";
    private static final String EXTRA_WIFI_SECURITY_NAME = "wifi_security_name";

    public static Intent createIntent(Context context, AccessPoint result, int security) {
        return new Intent(context, WifiConnectionActivity.class)
                .putExtra(EXTRA_WIFI_SSID, result.getSsidStr())
                .putExtra(EXTRA_WIFI_SECURITY_NAME, security);
    }

    public static Intent createIntent(Context context, AccessPoint result) {
        final int security = result.getSecurity();
        return createIntent(context, result, security);
    }

    public static Intent createIntent(Context context, WifiConfiguration configuration) {
        final int security = WifiSecurityUtil.getSecurity(configuration);
        final String ssid = configuration.getPrintableSsid();
        return new Intent(context, WifiConnectionActivity.class)
                .putExtra(EXTRA_WIFI_SSID, ssid)
                .putExtra(EXTRA_WIFI_SECURITY_NAME, security);
    }

    private WifiConfiguration mConfiguration;
    private int mWifiSecurity;
    private StateMachine mStateMachine;
    private State mConnectAuthFailureState;
    private State mConnectFailedState;
    private State mConnectRejectedByApState;
    private State mConnectState;
    private State mConnectTimeOutState;
    private State mEnterPasswordState;
    private State mKnownNetworkState;
    private State mSuccessState;
    private State mOptionsOrConnectState;
    private State mAddStartState;
    private State mFinishState;

    private final StateMachine.Callback mStateMachineCallback = new StateMachine.Callback() {
        @Override
        public void onFinish(int result) {
            setResult(result);
            finish();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.wifi_container);
        mStateMachine = ViewModelProviders.of(this).get(StateMachine.class);
        mStateMachine.setCallback(mStateMachineCallback);
        mKnownNetworkState = new KnownNetworkState(this);
        mEnterPasswordState = new EnterPasswordState(this);
        mConnectState = new ConnectState(this);
        mConnectTimeOutState = new ConnectTimeOutState(this);
        mConnectRejectedByApState = new ConnectRejectedByApState(this);
        mConnectFailedState = new ConnectFailedState(this);
        mConnectAuthFailureState = new ConnectAuthFailureState(this);
        mSuccessState = new SuccessState(this);
        mOptionsOrConnectState = new OptionsOrConnectState(this);
        mAddStartState = new AddStartState(this);
        mFinishState = new FinishState(this);

        /* KnownNetwork */
        mStateMachine.addState(
                mKnownNetworkState,
                StateMachine.ADD_START,
                mAddStartState);
        mStateMachine.addState(
                mKnownNetworkState,
                StateMachine.SELECT_WIFI,
                mFinishState);

        /* Add Start */
        mStateMachine.addState(
                mAddStartState,
                StateMachine.PASSWORD,
                mEnterPasswordState);
        mStateMachine.addState(
                mAddStartState,
                StateMachine.CONNECT,
                mConnectState);

        /* Enter Password */
        mStateMachine.addState(
                mEnterPasswordState,
                StateMachine.OPTIONS_OR_CONNECT,
                mOptionsOrConnectState);

        /* Option or Connect */
        mStateMachine.addState(
                mOptionsOrConnectState,
                StateMachine.CONNECT,
                mConnectState);

        /* Connect */
        mStateMachine.addState(
                mConnectState,
                StateMachine.RESULT_REJECTED_BY_AP,
                mConnectRejectedByApState);
        mStateMachine.addState(
                mConnectState,
                StateMachine.RESULT_UNKNOWN_ERROR,
                mConnectFailedState);
        mStateMachine.addState(
                mConnectState,
                StateMachine.RESULT_TIMEOUT,
                mConnectTimeOutState);
        mStateMachine.addState(
                mConnectState,
                StateMachine.RESULT_BAD_AUTH,
                mConnectAuthFailureState);
        mStateMachine.addState(
                mConnectState,
                StateMachine.RESULT_SUCCESS,
                mSuccessState);

        /* Connect Failed */
        mStateMachine.addState(
                mConnectFailedState,
                StateMachine.TRY_AGAIN,
                mOptionsOrConnectState
        );
        mStateMachine.addState(
                mConnectFailedState,
                StateMachine.SELECT_WIFI,
                mFinishState
        );

        /* Connect Timeout */
        mStateMachine.addState(
                mConnectTimeOutState,
                StateMachine.TRY_AGAIN,
                mOptionsOrConnectState
        );
        mStateMachine.addState(
                mConnectTimeOutState,
                StateMachine.SELECT_WIFI,
                mFinishState
        );

        /* Connect Rejected By AP */
        mStateMachine.addState(
                mConnectRejectedByApState,
                StateMachine.TRY_AGAIN,
                mOptionsOrConnectState);
        mStateMachine.addState(
                mConnectRejectedByApState,
                StateMachine.SELECT_WIFI,
                mFinishState);

        /*Connect Auth Failure */
        mStateMachine.addState(
                mConnectAuthFailureState,
                StateMachine.TRY_AGAIN,
                mOptionsOrConnectState
        );
        mStateMachine.addState(
                mConnectAuthFailureState,
                StateMachine.SELECT_WIFI,
                mFinishState
        );

        mWifiSecurity = getIntent().getIntExtra(EXTRA_WIFI_SECURITY_NAME, 0);
        mConfiguration = WifiConfigHelper.getConfiguration(
                this, getIntent().getStringExtra(EXTRA_WIFI_SSID), mWifiSecurity);

        AdvancedWifiOptionsFlow.createFlow(
                this, false, true, null,
                mOptionsOrConnectState, mConnectState, AdvancedWifiOptionsFlow.START_DEFAULT_PAGE);
        UserChoiceInfo userChoiceInfo =
                    ViewModelProviders.of(this).get(UserChoiceInfo.class);
        userChoiceInfo.setWifiConfiguration(mConfiguration);
        userChoiceInfo.setWifiSecurity(mWifiSecurity);
        if (WifiConfigHelper.isNetworkSaved(mConfiguration)) {
            mStateMachine.setStartState(mKnownNetworkState);
        } else {
            mStateMachine.setStartState(mAddStartState);
        }
        mStateMachine.start(true);
    }

    @Override
    public void onBackPressed() {
        mStateMachine.back();
    }

    private void updateView(android.support.v4.app.Fragment fragment, boolean movingForward) {
        if (fragment != null) {
            FragmentTransaction updateTransaction = getSupportFragmentManager().beginTransaction();
            if (movingForward) {
                updateTransaction.setTransition(FragmentTransaction.TRANSIT_FRAGMENT_OPEN);
            } else {
                updateTransaction.setTransition(FragmentTransaction.TRANSIT_FRAGMENT_CLOSE);
            }
            updateTransaction.replace(R.id.wifi_container, fragment, TAG);
            updateTransaction.commit();
        }
    }

    @Override
    public void onFragmentChange(Fragment newFragment, boolean movingForward) {
        updateView(newFragment, movingForward);
    }

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.SETTINGS_TV_WIFI_ADD_KNOWN_CATEGORY;
    }
}
