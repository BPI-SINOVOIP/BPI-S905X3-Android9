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

import com.android.internal.logging.nano.MetricsProto;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.setup.AdvancedWifiOptionsFlow;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.core.instrumentation.InstrumentedActivity;

/**
 * Allows the modification of advanced Wi-Fi settings
 */
public class EditProxySettingsActivity extends InstrumentedActivity implements
        State.FragmentChangeListener {

    private static final String TAG = "EditProxySettings";

    private static final int NETWORK_ID_ETHERNET = WifiConfiguration.INVALID_NETWORK_ID;
    private static final String EXTRA_NETWORK_ID = "network_id";

    public static Intent createIntent(Context context, int networkId) {
        return new Intent(context, EditProxySettingsActivity.class)
                .putExtra(EXTRA_NETWORK_ID, networkId);
    }

    private State mSaveState;
    private State mSaveSuccessState;
    private State mSaveFailedState;
    private StateMachine mStateMachine;
    private final StateMachine.Callback mStateMachineCallback = result -> {
        setResult(result);
        finish();
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.wifi_container);
        mStateMachine = ViewModelProviders.of(this).get(StateMachine.class);
        mStateMachine.setCallback(mStateMachineCallback);
        mSaveState = new SaveState(this);
        mSaveSuccessState = new SaveSuccessState(this);
        mSaveFailedState = new SaveFailedState(this);
        int networkId = getIntent().getIntExtra(EXTRA_NETWORK_ID, NETWORK_ID_ETHERNET);
        NetworkConfiguration netConfig;
        if (networkId == NETWORK_ID_ETHERNET) {
            netConfig = new EthernetConfig(this);
            ((EthernetConfig) netConfig).load();
        } else {
            netConfig = new WifiConfig(this);
            ((WifiConfig) netConfig).load(networkId);
        }
        EditSettingsInfo editSettingsInfo = ViewModelProviders.of(this).get(EditSettingsInfo.class);
        editSettingsInfo.setNetworkConfiguration(netConfig);
        AdvancedWifiOptionsFlow.createFlow(this, false, true, netConfig,
                null, mSaveState, AdvancedWifiOptionsFlow.START_PROXY_SETTINGS_PAGE);

        /* Save */
        mStateMachine.addState(
                mSaveState,
                StateMachine.RESULT_SUCCESS,
                mSaveSuccessState
        );
        mStateMachine.addState(
                mSaveState,
                StateMachine.RESULT_FAILURE,
                mSaveFailedState
        );
        mStateMachine.start(true);
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.DIALOG_WIFI_AP_EDIT;
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
