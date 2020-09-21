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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.VisibleForTesting;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.util.Log;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.ConnectivityListener;
import com.android.tv.settings.connectivity.WifiConfigHelper;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;

import java.lang.ref.WeakReference;

/**
 * State responsible for showing the connect page.
 */
public class ConnectState implements State {
    private final FragmentActivity mActivity;
    private Fragment mFragment;

    public ConnectState(FragmentActivity wifiSetupActivity) {
        this.mActivity = wifiSetupActivity;
    }

    @Override
    public void processForward() {
        UserChoiceInfo userChoiceInfo = ViewModelProviders.of(mActivity).get(UserChoiceInfo.class);
        WifiConfiguration wifiConfig = userChoiceInfo.getWifiConfiguration();
        String title = mActivity.getString(
                R.string.wifi_connecting,
                wifiConfig.getPrintableSsid());
        mFragment = ConnectToWifiFragment.newInstance(title, true);
        if (!WifiConfigHelper.isNetworkSaved(wifiConfig)) {
            AdvancedOptionsFlowInfo advFlowInfo =
                    ViewModelProviders.of(mActivity).get(AdvancedOptionsFlowInfo.class);
            if (advFlowInfo.getIpConfiguration() != null) {
                wifiConfig.setIpConfiguration(advFlowInfo.getIpConfiguration());
            }
        }
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        StateMachine stateMachine = ViewModelProviders.of(mActivity).get(StateMachine.class);
        stateMachine.back();
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Connects to the wifi network specified by the given configuration.
     */
    public static class ConnectToWifiFragment extends MessageFragment
            implements ConnectivityListener.Listener {

        @VisibleForTesting
        static final int MSG_TIMEOUT = 1;
        @VisibleForTesting
        static final int CONNECTION_TIMEOUT = 15000;
        private static final String TAG = "ConnectToWifiFragment";
        private static final boolean DEBUG = false;
        @VisibleForTesting
        StateMachine mStateMachine;
        @VisibleForTesting
        WifiConfiguration mWifiConfiguration;
        @VisibleForTesting
        WifiManager mWifiManager;
        @VisibleForTesting
        Handler mHandler;
        private ConnectivityListener mConnectivityListener;
        private BroadcastReceiver mReceiver;
        private boolean mWasAssociating;
        private boolean mWasAssociated;
        private boolean mWasHandshaking;
        private boolean mConnected;

        /**
         * Obtain a new instance of ConnectToWifiFragment.
         *
         * @param title                 title of fragment.
         * @param showProgressIndicator whether show progress indicator.
         * @return new instance.
         */
        public static ConnectToWifiFragment newInstance(String title,
                boolean showProgressIndicator) {
            ConnectToWifiFragment fragment = new ConnectToWifiFragment();
            Bundle args = new Bundle();
            addArguments(args, title, showProgressIndicator);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public void onCreate(Bundle icicle) {
            super.onCreate(icicle);
            mConnectivityListener = new ConnectivityListener(getActivity(), this);
            UserChoiceInfo userChoiceInfo = ViewModelProviders
                    .of(getActivity()).get(UserChoiceInfo.class);
            mWifiConfiguration = userChoiceInfo.getWifiConfiguration();
            mStateMachine = ViewModelProviders
                    .of(getActivity()).get(StateMachine.class);

            mWifiManager = ((WifiManager) getActivity().getApplicationContext()
                    .getSystemService(Context.WIFI_SERVICE));
            mHandler = new MessageHandler(this);

            IntentFilter filter = new IntentFilter();
            filter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
            mReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (WifiManager.SUPPLICANT_STATE_CHANGED_ACTION.equals(intent.getAction())) {
                        SupplicantState state = intent.getParcelableExtra(
                                WifiManager.EXTRA_NEW_STATE);
                        if (DEBUG) {
                            Log.d(TAG, "Got supplicant state: " + state.name());
                        }
                        switch (state) {
                            case ASSOCIATING:
                                mWasAssociating = true;
                                break;
                            case ASSOCIATED:
                                mWasAssociated = true;
                                break;
                            case COMPLETED:
                                // this just means the supplicant has connected, now
                                // we wait for the rest of the framework to catch up
                                break;
                            case DISCONNECTED:
                            case DORMANT:
                                if (mWasAssociated || mWasHandshaking) {
                                    notifyListener(mWasHandshaking ? StateMachine.RESULT_BAD_AUTH
                                            : StateMachine.RESULT_UNKNOWN_ERROR);
                                }
                                break;
                            case INTERFACE_DISABLED:
                            case UNINITIALIZED:
                                notifyListener(StateMachine.RESULT_UNKNOWN_ERROR);
                                break;
                            case FOUR_WAY_HANDSHAKE:
                            case GROUP_HANDSHAKE:
                                mWasHandshaking = true;
                                break;
                            case INACTIVE:
                                if (mWasAssociating && !mWasAssociated) {
                                    // If we go inactive after 'associating' without ever having
                                    // been 'associated', the AP(s) must have rejected us.
                                    notifyListener(StateMachine.RESULT_REJECTED_BY_AP);
                                    break;
                                }
                            case INVALID:
                                break;
                            case SCANNING:
                                break;
                            default:
                                return;
                        }
                        mHandler.removeMessages(MSG_TIMEOUT);
                        mHandler.sendEmptyMessageDelayed(MSG_TIMEOUT, CONNECTION_TIMEOUT);
                    }
                }
            };
            getActivity().registerReceiver(mReceiver, filter);
            mConnectivityListener.start();
        }

        @Override
        public void onResume() {
            super.onResume();
            proceedDependOnNetworkState();
        }

        @VisibleForTesting
        void proceedDependOnNetworkState() {
            if (isNetworkConnected()) {
                mConnected = true;
                notifyListener(StateMachine.RESULT_SUCCESS);
            } else {
                int networkId = mWifiManager.addNetwork(mWifiConfiguration);
                if (networkId == -1) {
                    if (DEBUG) {
                        Log.d(TAG, "Failed to add network!");
                    }
                    notifyListener(StateMachine.RESULT_UNKNOWN_ERROR);
                } else if (!mWifiManager.enableNetwork(networkId, true)) {
                    if (DEBUG) {
                        Log.d(TAG, "Failed to enable network id " + networkId + "!");
                    }
                    notifyListener(StateMachine.RESULT_UNKNOWN_ERROR);
                } else if (!mWifiManager.reconnect()) {
                    if (DEBUG) {
                        Log.d(TAG, "Failed to reconnect!");
                    }
                    notifyListener(StateMachine.RESULT_UNKNOWN_ERROR);
                } else {
                    mHandler.sendEmptyMessageDelayed(MSG_TIMEOUT, CONNECTION_TIMEOUT);
                }
            }
        }

        @Override
        public void onDestroy() {
            if (!mConnected) {
                mWifiManager.disconnect();
            }
            getActivity().unregisterReceiver(mReceiver);
            mConnectivityListener.stop();
            mConnectivityListener.destroy();
            mHandler.removeMessages(MSG_TIMEOUT);
            super.onDestroy();
        }

        @Override
        public void onConnectivityChange() {
            if (DEBUG) Log.d(TAG, "Connectivity changed");
            if (!isResumed()) {
                return;
            }
            if (isNetworkConnected()) {
                mConnected = true;
                notifyListener(StateMachine.RESULT_SUCCESS);
            }
        }

        private void notifyListener(int result) {
            if (mStateMachine.getCurrentState() instanceof ConnectState) {
                mStateMachine.getListener().onComplete(result);
            }
        }

        private boolean isNetworkConnected() {
            ConnectivityManager connMan = getActivity().getSystemService(ConnectivityManager.class);
            NetworkInfo netInfo = connMan.getActiveNetworkInfo();
            if (netInfo == null) {
                if (DEBUG) Log.d(TAG, "NetworkInfo is null; network is not connected");
                return false;
            }

            if (DEBUG) Log.d(TAG, "NetworkInfo: " + netInfo.toString());
            if (netInfo.isConnected() && netInfo.getType() == ConnectivityManager.TYPE_WIFI) {
                WifiInfo currentConnection = mWifiManager.getConnectionInfo();
                if (DEBUG) {
                    Log.d(TAG, "Connected to "
                            + ((currentConnection == null)
                            ? "nothing" : currentConnection.getSSID()));
                }
                if (currentConnection != null
                        && currentConnection.getSSID().equals(mWifiConfiguration.SSID)) {
                    return true;
                }
            } else {
                if (DEBUG) Log.d(TAG, "Network is not connected");
            }
            return false;
        }

        private static class MessageHandler extends Handler {

            private final WeakReference<ConnectToWifiFragment> mFragmentRef;

            MessageHandler(ConnectToWifiFragment fragment) {
                mFragmentRef = new WeakReference<>(fragment);
            }

            @Override
            public void handleMessage(Message msg) {
                if (DEBUG) Log.d(TAG, "Timeout waiting on supplicant state change");

                final ConnectToWifiFragment fragment = mFragmentRef.get();
                if (fragment == null) {
                    return;
                }

                if (fragment.isNetworkConnected()) {
                    if (DEBUG) Log.d(TAG, "Fake timeout; we're actually connected");
                    fragment.mConnected = true;
                    fragment.notifyListener(StateMachine.RESULT_SUCCESS);
                } else {
                    if (DEBUG) Log.d(TAG, "Timeout is real; telling the listener");
                    fragment.notifyListener(StateMachine.RESULT_TIMEOUT);
                }
            }
        }
    }
}
