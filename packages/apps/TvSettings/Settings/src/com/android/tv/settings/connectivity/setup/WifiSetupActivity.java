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

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.arch.lifecycle.ViewModelProviders;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentTransaction;

import com.android.settingslib.wifi.WifiTracker;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.util.ThemeHelper;
import com.android.tv.settings.util.TransitionUtils;

/**
 * Wi-Fi settings during initial setup for a large no-touch device.
 */
public class WifiSetupActivity extends FragmentActivity implements State.FragmentChangeListener {
    private static final String TAG = "WifiSetupActivity";
    private static final String EXTRA_SHOW_SUMMARY = "extra_show_summary";
    private static final String EXTRA_SHOW_SKIP_NETWORK = "extra_show_skip_network";
    private static final String EXTRA_MOVING_FORWARD = "movingForward";

    private boolean mShowFirstFragmentForwards;
    private boolean mResultOk = false;
    private final StateMachine.Callback mStateMachineCallback = new StateMachine.Callback() {
        @Override
        public void onFinish(int result) {
            setResult(result);
            mResultOk = result == Activity.RESULT_OK;
            finish();
        }
    };
    private WifiTracker mWifiTracker;
    private NetworkListInfo mNetworkListInfo;
    private UserChoiceInfo mUserChoiceInfo;
    private StateMachine mStateMachine;
    private State mChooseSecurityState;
    private State mConnectAuthFailureState;
    private State mConnectFailedState;
    private State mConnectRejectedByApState;
    private State mConnectState;
    private State mConnectTimeOutState;
    private State mEnterPasswordState;
    private State mEnterSsidState;
    private State mKnownNetworkState;
    private State mSelectWifiState;
    private State mSuccessState;
    private State mSummaryConnectedNonWifiState;
    private State mSummaryConnectedWifiState;
    private State mSummaryNotConnectedState;
    private State mOptionsOrConnectState;
    private State mAddPageBasedOnNetworkChoiceState;
    private State mAddStartState;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.wifi_container);
        // fade in
        ObjectAnimator animator = TransitionUtils.createActivityFadeInAnimator(getResources(),
                true);
        animator.setTarget(findViewById(R.id.wifi_container));
        animator.start();

        mStateMachine = ViewModelProviders.of(this).get(StateMachine.class);
        mStateMachine.setCallback(mStateMachineCallback);
        mNetworkListInfo = ViewModelProviders.of(this).get(NetworkListInfo.class);
        mNetworkListInfo.initNetworkRefreshTime();
        mUserChoiceInfo = ViewModelProviders.of(this).get(UserChoiceInfo.class);

        WifiTracker.WifiListener wifiListener = new WifiTracker.WifiListener() {
            @Override
            public void onWifiStateChanged(int state) {

            }

            @Override
            public void onConnectedChanged() {

            }

            @Override
            public void onAccessPointsChanged() {
                long currentTime = System.currentTimeMillis();
                if (mStateMachine.getCurrentState() == mSelectWifiState
                        && currentTime >= mNetworkListInfo.getNextNetworkRefreshTime()) {
                    ((SelectWifiState.SelectWifiFragment) mSelectWifiState.getFragment())
                            .updateNetworkList();
                    mNetworkListInfo.updateNextNetworkRefreshTime();
                }
            }
        };
        mWifiTracker = new WifiTracker(this, wifiListener, true, true);
        mNetworkListInfo.setWifiTracker(mWifiTracker);
        boolean showSummary = getIntent().getBooleanExtra(EXTRA_SHOW_SUMMARY, false);
        mNetworkListInfo.setShowSkipNetwork(
                getIntent().getBooleanExtra(EXTRA_SHOW_SKIP_NETWORK, false));

        // If we are not moving forwards during the setup flow, we need to show the first fragment
        // with the reverse animation.
        mShowFirstFragmentForwards = getIntent().getBooleanExtra(EXTRA_MOVING_FORWARD, true);

        mKnownNetworkState = new KnownNetworkState(this);
        mSelectWifiState = new SelectWifiState(this);
        mEnterSsidState = new EnterSsidState(this);
        mChooseSecurityState = new ChooseSecurityState(this);
        mEnterPasswordState = new EnterPasswordState(this);
        mConnectState = new ConnectState(this);
        mConnectTimeOutState = new ConnectTimeOutState(this);
        mConnectRejectedByApState = new ConnectRejectedByApState(this);
        mConnectFailedState = new ConnectFailedState(this);
        mConnectAuthFailureState = new ConnectAuthFailureState(this);
        mSuccessState = new SuccessState(this);
        mOptionsOrConnectState = new OptionsOrConnectState(this);
        mAddPageBasedOnNetworkChoiceState = new AddPageBasedOnNetworkState(this);
        mAddStartState = new AddStartState(this);
        mSelectWifiState = new SelectWifiState(this);

        if (showSummary) {
            addSummaryState();
        } else {
            mStateMachine.setStartState(mSelectWifiState);
        }

        AdvancedWifiOptionsFlow.createFlow(this, true, false, null,
                mOptionsOrConnectState, mConnectState, AdvancedWifiOptionsFlow.START_DEFAULT_PAGE);

        // Define the transition between different states.
        /* KnownNetwork */
        mStateMachine.addState(
                mKnownNetworkState,
                StateMachine.ADD_START,
                mAddStartState);
        mStateMachine.addState(
                mKnownNetworkState,
                StateMachine.SELECT_WIFI,
                mSelectWifiState);

        /* Add start page */
        mStateMachine.addState(
                mAddStartState,
                StateMachine.PASSWORD,
                mEnterPasswordState);
        mStateMachine.addState(
                mAddStartState,
                StateMachine.CONNECT,
                mConnectState);

        /* Select Wi-Fi */
        mStateMachine.addState(
                mSelectWifiState,
                StateMachine.ADD_PAGE_BASED_ON_NETWORK_CHOICE,
                mAddPageBasedOnNetworkChoiceState);

        /* Add page based on network choice*/
        mStateMachine.addState(
                mAddPageBasedOnNetworkChoiceState,
                StateMachine.OTHER_NETWORK,
                mEnterSsidState);
        mStateMachine.addState(
                mAddPageBasedOnNetworkChoiceState,
                StateMachine.KNOWN_NETWORK,
                mKnownNetworkState);
        mStateMachine.addState(
                mAddPageBasedOnNetworkChoiceState,
                StateMachine.ADD_START,
                mAddStartState);

        /* Enter SSID */
        mStateMachine.addState(
                mEnterSsidState,
                StateMachine.CONTINUE,
                mChooseSecurityState);

        /* Choose Security */
        mStateMachine.addState(
                mChooseSecurityState,
                StateMachine.OPTIONS_OR_CONNECT,
                mOptionsOrConnectState);
        mStateMachine.addState(
                mChooseSecurityState,
                StateMachine.PASSWORD,
                mEnterPasswordState);

        /* Enter Password */
        mStateMachine.addState(
                mEnterPasswordState,
                StateMachine.OPTIONS_OR_CONNECT,
                mOptionsOrConnectState);

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
                mSelectWifiState
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
                mSelectWifiState
        );

        /* Connect Rejected By AP */
        mStateMachine.addState(
                mConnectRejectedByApState,
                StateMachine.TRY_AGAIN,
                mOptionsOrConnectState);
        mStateMachine.addState(
                mConnectRejectedByApState,
                StateMachine.SELECT_WIFI,
                mSelectWifiState);

        /* Connect Authentication failure */
        mStateMachine.addState(
                mConnectAuthFailureState,
                StateMachine.TRY_AGAIN,
                mAddPageBasedOnNetworkChoiceState);
        mStateMachine.addState(
                mConnectAuthFailureState,
                StateMachine.SELECT_WIFI,
                mSelectWifiState);

        /* Summary Not Connected */
        mStateMachine.addState(
                mSummaryNotConnectedState,
                StateMachine.SELECT_WIFI,
                mSelectWifiState);

        /* Summary Connected */
        mStateMachine.addState(
                mSummaryConnectedWifiState,
                StateMachine.SELECT_WIFI,
                mSelectWifiState);

        mStateMachine.start(mShowFirstFragmentForwards);
    }

    @Override
    public void onBackPressed() {
        mStateMachine.back();
    }

    @Override
    public void onResume() {
        super.onResume();
        mWifiTracker.onStart();
    }

    @Override
    public void onPause() {
        super.onPause();
        mWifiTracker.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mWifiTracker.onDestroy();
    }

    @Override
    public void finish() {
        Animator animator;

        // Choose finish animation based on whether we are in Setup or Settings and really
        // finish this activity when the animation is complete.
        if (ThemeHelper.fromSetupWizard(getIntent())) {
            animator = mResultOk
                    ? AnimatorInflater.loadAnimator(this, R.anim.setup_fragment_open_out)
                    : AnimatorInflater.loadAnimator(this, R.anim.setup_fragment_close_out);
        } else {
            animator = TransitionUtils.createActivityFadeOutAnimator(getResources(), true);
        }

        animator.setTarget(findViewById(R.id.wifi_container));
        animator.addListener(new Animator.AnimatorListener() {

            @Override
            public void onAnimationStart(Animator animation) {
            }

            @Override
            public void onAnimationRepeat(Animator animation) {
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                doFinish();
            }

            @Override
            public void onAnimationCancel(Animator animation) {
            }
        });
        animator.start();
    }

    private void doFinish() {
        super.finish();
    }

    private void addSummaryState() {
        ConnectivityManager connectivityManager = (ConnectivityManager) getSystemService(
                Context.CONNECTIVITY_SERVICE);
        NetworkInfo currentConnection = connectivityManager.getActiveNetworkInfo();
        boolean isConnected = (currentConnection != null) && currentConnection.isConnected();
        mSummaryConnectedWifiState = new SummaryConnectedWifiState(this);
        mSummaryConnectedNonWifiState = new SummaryConnectedNonWifiState(this);
        mSummaryNotConnectedState = new SummaryNotConnectedState(this);

        if (isConnected) {
            if (currentConnection.getType() == ConnectivityManager.TYPE_WIFI) {
                WifiInfo currentWifiConnection = mWifiTracker.getManager().getConnectionInfo();
                String connectedNetwork = WifiInfo.removeDoubleQuotes(
                        currentWifiConnection.getSSID());
                if (connectedNetwork == null) {
                    connectedNetwork = getString(R.string.wifi_summary_unknown_network);
                }
                mUserChoiceInfo.setConnectedNetwork(connectedNetwork);
                mStateMachine.setStartState(mSummaryConnectedWifiState);
            } else {
                mStateMachine.setStartState(mSummaryConnectedNonWifiState);
            }
        } else {
            mStateMachine.setStartState(mSummaryNotConnectedState);
        }
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
        // TODO: Add accessiblity titles
    }

    @Override
    public void onFragmentChange(Fragment newFragment, boolean movingForward) {
        updateView(newFragment, movingForward);
    }
}
