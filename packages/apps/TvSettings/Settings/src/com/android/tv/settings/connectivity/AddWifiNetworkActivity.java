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
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.setup.AdvancedWifiOptionsFlow;
import com.android.tv.settings.connectivity.setup.ChooseSecurityState;
import com.android.tv.settings.connectivity.setup.ConnectAuthFailureState;
import com.android.tv.settings.connectivity.setup.ConnectFailedState;
import com.android.tv.settings.connectivity.setup.ConnectRejectedByApState;
import com.android.tv.settings.connectivity.setup.ConnectState;
import com.android.tv.settings.connectivity.setup.ConnectTimeOutState;
import com.android.tv.settings.connectivity.setup.EnterPasswordState;
import com.android.tv.settings.connectivity.setup.EnterSsidState;
import com.android.tv.settings.connectivity.setup.OptionsOrConnectState;
import com.android.tv.settings.connectivity.setup.SuccessState;
import com.android.tv.settings.connectivity.setup.UserChoiceInfo;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.core.instrumentation.InstrumentedActivity;

/**
 * Manual-style add wifi network (the kind you'd use for adding a hidden or out-of-range network.)
 */
public class AddWifiNetworkActivity extends InstrumentedActivity
            implements State.FragmentChangeListener {
    private static final String TAG = "AddWifiNetworkActivity";
    private State mChooseSecurityState;
    private State mConnectAuthFailureState;
    private State mConnectFailedState;
    private State mConnectRejectedByApState;
    private State mConnectState;
    private State mConnectTimeOutState;
    private State mEnterPasswordState;
    private State mEnterSsidState;
    private State mSuccessState;
    private State mOptionsOrConnectState;
    private State mEnterAdvancedFlowOrRetryState;
    private State mFinishState;
    private StateMachine mStateMachine;
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
        UserChoiceInfo userChoiceInfo = ViewModelProviders.of(this).get(UserChoiceInfo.class);
        userChoiceInfo.getWifiConfiguration().hiddenSSID = true;

        mEnterSsidState = new EnterSsidState(this);
        mChooseSecurityState = new ChooseSecurityState(this);
        mEnterPasswordState = new EnterPasswordState(this);
        mConnectState = new ConnectState(this);
        mSuccessState = new SuccessState(this);
        mOptionsOrConnectState = new OptionsOrConnectState(this);
        mConnectTimeOutState = new ConnectTimeOutState(this);
        mConnectRejectedByApState = new ConnectRejectedByApState(this);
        mConnectFailedState = new ConnectFailedState(this);
        mConnectAuthFailureState = new ConnectAuthFailureState(this);
        mFinishState = new FinishState(this);
        mEnterAdvancedFlowOrRetryState = new EnterAdvancedFlowOrRetryState(this);

        AdvancedWifiOptionsFlow.createFlow(
                this, true, true, null, mOptionsOrConnectState,
                mConnectState, AdvancedWifiOptionsFlow.START_DEFAULT_PAGE);
        AdvancedWifiOptionsFlow.createFlow(
                this, true, true, null, mEnterAdvancedFlowOrRetryState,
                mConnectState, AdvancedWifiOptionsFlow.START_DEFAULT_PAGE);

        /* Enter SSID */
        mStateMachine.addState(
                mEnterSsidState,
                StateMachine.CONTINUE,
                mChooseSecurityState
        );

        /* Choose security */
        mStateMachine.addState(
                mChooseSecurityState,
                StateMachine.OPTIONS_OR_CONNECT,
                mOptionsOrConnectState
        );
        mStateMachine.addState(
                mChooseSecurityState,
                StateMachine.PASSWORD,
                mEnterPasswordState
        );

        /* Enter Password */
        mStateMachine.addState(
                mEnterPasswordState,
                StateMachine.OPTIONS_OR_CONNECT,
                mOptionsOrConnectState
        );

        /* Options or Connect */
        mStateMachine.addState(
                mOptionsOrConnectState,
                StateMachine.CONNECT,
                mConnectState
        );

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

        /* Connect Auth Failure */
        mStateMachine.addState(
                mConnectAuthFailureState,
                StateMachine.TRY_AGAIN,
                mEnterAdvancedFlowOrRetryState);
        mStateMachine.addState(
                mConnectRejectedByApState,
                StateMachine.SELECT_WIFI,
                mFinishState);

        /* Enter Advanced Flow or Retry */
        mStateMachine.addState(
                mEnterAdvancedFlowOrRetryState,
                StateMachine.CONTINUE,
                mEnterSsidState);
        mStateMachine.setStartState(mEnterSsidState);
        mStateMachine.start(true);
    }

    @Override
    public int getMetricsCategory() {
        // do not log visibility.
        return MetricsProto.MetricsEvent.ACTION_WIFI_ADD_NETWORK;
    }

    @Override
    public void onBackPressed() {
        mStateMachine.back();
    }

    private void updateView(Fragment fragment, boolean movingForward) {
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
}
