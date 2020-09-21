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

package com.android.settings.wifi.tether;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.provider.Settings;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.text.BidiFormatter;

import com.android.settings.R;
import com.android.settings.Utils;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnStart;
import com.android.settingslib.core.lifecycle.events.OnStop;

public class WifiTetherPreferenceController extends AbstractPreferenceController
        implements PreferenceControllerMixin, LifecycleObserver, OnStart, OnStop {

    private static final String WIFI_TETHER_SETTINGS = "wifi_tether";
    private static final IntentFilter AIRPLANE_INTENT_FILTER = new IntentFilter(
            Intent.ACTION_AIRPLANE_MODE_CHANGED);
    private static final int ID_NULL = -1;

    private final ConnectivityManager mConnectivityManager;
    private final String[] mWifiRegexs;
    private final WifiManager mWifiManager;
    private final Lifecycle mLifecycle;
    private int mSoftApState;
    @VisibleForTesting
    Preference mPreference;
    @VisibleForTesting
    WifiTetherSoftApManager mWifiTetherSoftApManager;

    public WifiTetherPreferenceController(Context context, Lifecycle lifecycle) {
        this(context, lifecycle, true /* initSoftApManager */);
    }

    @VisibleForTesting
    WifiTetherPreferenceController(Context context, Lifecycle lifecycle,
            boolean initSoftApManager) {
        super(context);
        mConnectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        mWifiRegexs = mConnectivityManager.getTetherableWifiRegexs();
        mLifecycle = lifecycle;
        if (lifecycle != null) {
            lifecycle.addObserver(this);
        }
        if (initSoftApManager) {
            initWifiTetherSoftApManager();
        }
    }

    @Override
    public boolean isAvailable() {
        return mWifiRegexs != null
                && mWifiRegexs.length != 0
                && !Utils.isMonkeyRunning();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mPreference = screen.findPreference(WIFI_TETHER_SETTINGS);
        if (mPreference == null) {
            // unavailable
            return;
        }
    }

    @Override
    public String getPreferenceKey() {
        return WIFI_TETHER_SETTINGS;
    }

    @Override
    public void onStart() {
        if (mPreference != null) {
            mContext.registerReceiver(mReceiver, AIRPLANE_INTENT_FILTER);
            clearSummaryForAirplaneMode();
            if (mWifiTetherSoftApManager != null) {
                mWifiTetherSoftApManager.registerSoftApCallback();
            }
        }
    }

    @Override
    public void onStop() {
        if (mPreference != null) {
            mContext.unregisterReceiver(mReceiver);
            if (mWifiTetherSoftApManager != null) {
                mWifiTetherSoftApManager.unRegisterSoftApCallback();
            }
        }
    }

    @VisibleForTesting
    void initWifiTetherSoftApManager() {
        // This manager only handles the number of connected devices, other parts are handled by
        // normal BroadcastReceiver in this controller
        mWifiTetherSoftApManager = new WifiTetherSoftApManager(mWifiManager,
                new WifiTetherSoftApManager.WifiTetherSoftApCallback() {
                    @Override
                    public void onStateChanged(int state, int failureReason) {
                        mSoftApState = state;
                        handleWifiApStateChanged(state, failureReason);
                    }

                    @Override
                    public void onNumClientsChanged(int numClients) {
                        if (mPreference != null
                                && mSoftApState == WifiManager.WIFI_AP_STATE_ENABLED) {
                            // Only show the number of clients when state is on
                            mPreference.setSummary(mContext.getResources().getQuantityString(
                                    R.plurals.wifi_tether_connected_summary, numClients,
                                    numClients));
                        }
                    }
                });
    }

    //
    // Everything below is copied from WifiApEnabler
    //
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(action)) {
                clearSummaryForAirplaneMode(R.string.wifi_hotspot_off_subtext);
            }
        }
    };

    @VisibleForTesting
    void handleWifiApStateChanged(int state, int reason) {
        switch (state) {
            case WifiManager.WIFI_AP_STATE_ENABLING:
                mPreference.setSummary(R.string.wifi_tether_starting);
                break;
            case WifiManager.WIFI_AP_STATE_ENABLED:
                WifiConfiguration wifiConfig = mWifiManager.getWifiApConfiguration();
                updateConfigSummary(wifiConfig);
                break;
            case WifiManager.WIFI_AP_STATE_DISABLING:
                mPreference.setSummary(R.string.wifi_tether_stopping);
                break;
            case WifiManager.WIFI_AP_STATE_DISABLED:
                mPreference.setSummary(R.string.wifi_hotspot_off_subtext);
                clearSummaryForAirplaneMode();
                break;
            default:
                if (reason == WifiManager.SAP_START_FAILURE_NO_CHANNEL) {
                    mPreference.setSummary(R.string.wifi_sap_no_channel_error);
                } else {
                    mPreference.setSummary(R.string.wifi_error);
                }
                clearSummaryForAirplaneMode();
        }
    }

    private void updateConfigSummary(WifiConfiguration wifiConfig) {
        final String s = mContext.getString(
                com.android.internal.R.string.wifi_tether_configure_ssid_default);

        mPreference.setSummary(mContext.getString(R.string.wifi_tether_enabled_subtext,
                BidiFormatter.getInstance().unicodeWrap(
                        (wifiConfig == null) ? s : wifiConfig.SSID)));
    }

    private void clearSummaryForAirplaneMode() {
        clearSummaryForAirplaneMode(ID_NULL);
    }

    private void clearSummaryForAirplaneMode(int defaultId) {
        boolean isAirplaneMode = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
        if (isAirplaneMode) {
            mPreference.setSummary(R.string.wifi_tether_disabled_by_airplane);
        } else if (defaultId != ID_NULL){
            mPreference.setSummary(defaultId);
        }
    }
    //
    // Everything above is copied from WifiApEnabler
    //
}
