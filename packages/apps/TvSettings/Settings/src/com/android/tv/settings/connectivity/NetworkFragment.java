/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.connectivity;

import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiConfiguration;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.provider.Settings;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceManager;
import android.support.v7.preference.TwoStatePreference;
import android.util.Log;
import android.text.TextUtils;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settingslib.wifi.AccessPoint;
import com.android.settingslib.wifi.AccessPointPreference;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * Fragment for controlling network connectivity
 */
public class NetworkFragment extends SettingsPreferenceFragment implements
        ConnectivityListener.Listener, ConnectivityListener.WifiNetworkListener,
        AccessPoint.AccessPointListener {

    private static final String TAG = "NetworkFragment";

    private static final String KEY_WIFI_ENABLE = "wifi_enable";
    private static final String KEY_WIFI_LIST = "wifi_list";
    private static final String KEY_WIFI_COLLAPSE = "wifi_collapse";
    private static final String KEY_WIFI_OTHER = "wifi_other";
    private static final String KEY_WIFI_ADD = "wifi_add";
    private static final String KEY_WIFI_ALWAYS_SCAN = "wifi_always_scan";
    private static final String KEY_ETHERNET = "ethernet";
    private static final String KEY_ETHERNET_STATUS = "ethernet_status";
    private static final String KEY_ETHERNET_PROXY = "ethernet_proxy";
    private static final String KEY_ETHERNET_DHCP = "ethernet_dhcp";

    private static final int INITIAL_UPDATE_DELAY = 500;

    private static final String BROADCAST_ACTION = "android.action.updateui";

    private ConnectivityListener mConnectivityListener;
    private AccessPointPreference.UserBadgeCache mUserBadgeCache;

    private TwoStatePreference mEnableWifiPref;
    private CollapsibleCategory mWifiNetworksCategory;
    private Preference mCollapsePref;
    private Preference mAddPref;
    private TwoStatePreference mAlwaysScan;
    private PreferenceCategory mEthernetCategory;
    private Preference mEthernetStatusPref;
    private Preference mEthernetProxyPref;
    private Preference mEthernetDhcpPref;

    private final Handler mHandler = new Handler();
    private long mNoWifiUpdateBeforeMillis;
    private Runnable mInitialUpdateWifiListRunnable = new Runnable() {
        @Override
        public void run() {
            mNoWifiUpdateBeforeMillis = 0;
            updateWifiList();
        }
    };

    private final BroadcastReceiver mUIReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(BROADCAST_ACTION)) {
                Log.d(TAG, "BROADCAST_ACTION");
                mConnectivityListener.updateConnectivityStatus();
                updateConnectivity();
            }
        }
    };

    public static NetworkFragment newInstance() {
        return new NetworkFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mConnectivityListener = new ConnectivityListener(getContext(), this, getLifecycle());
        mUserBadgeCache =
                new AccessPointPreference.UserBadgeCache(getContext().getPackageManager());
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onStart() {
        super.onStart();
        IntentFilter filter = new IntentFilter();
        filter.addAction(BROADCAST_ACTION);
        getContext().registerReceiver(mUIReceiver, filter);

        mConnectivityListener.setWifiListener(this);
        mNoWifiUpdateBeforeMillis = SystemClock.elapsedRealtime() + INITIAL_UPDATE_DELAY;
        updateWifiList();
    }

    @Override
    public void onResume() {
        super.onResume();
        // There doesn't seem to be an API to listen to everything this could cover, so
        // tickle it here and hope for the best.
        mConnectivityListener.updateConnectivityStatus();
        updateConnectivity();
    }

    @Override
    public void onStop() {
        super.onStop();
        getContext().unregisterReceiver(mUIReceiver);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getPreferenceManager().setPreferenceComparisonCallback(
                new PreferenceManager.SimplePreferenceComparisonCallback());
        setPreferencesFromResource(R.xml.network, null);

        mEnableWifiPref = (TwoStatePreference) findPreference(KEY_WIFI_ENABLE);
        mWifiNetworksCategory = (CollapsibleCategory) findPreference(KEY_WIFI_LIST);
        mCollapsePref = findPreference(KEY_WIFI_COLLAPSE);
        mAddPref = findPreference(KEY_WIFI_ADD);
        mAlwaysScan = (TwoStatePreference) findPreference(KEY_WIFI_ALWAYS_SCAN);

        mEthernetCategory = (PreferenceCategory) findPreference(KEY_ETHERNET);
        mEthernetStatusPref = findPreference(KEY_ETHERNET_STATUS);
        mEthernetProxyPref = findPreference(KEY_ETHERNET_PROXY);
        mEthernetProxyPref.setIntent(EditProxySettingsActivity.createIntent(getContext(),
                WifiConfiguration.INVALID_NETWORK_ID));
        mEthernetDhcpPref = findPreference(KEY_ETHERNET_DHCP);
        mEthernetDhcpPref.setIntent(EditIpSettingsActivity.createIntent(getContext(),
                WifiConfiguration.INVALID_NETWORK_ID));
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference.getKey() == null) {
            return super.onPreferenceTreeClick(preference);
        }
        switch (preference.getKey()) {
            case KEY_WIFI_ENABLE:
                mConnectivityListener.setWifiEnabled(mEnableWifiPref.isChecked());
                if (mMetricsFeatureProvider != null) {
                    if (mEnableWifiPref.isChecked()) {
                        mMetricsFeatureProvider.action(getContext(),
                                MetricsProto.MetricsEvent.ACTION_WIFI_ON);
                    } else {
                        // Log if user was connected at the time of switching off.
                        mMetricsFeatureProvider.action(getContext(),
                                MetricsProto.MetricsEvent.ACTION_WIFI_OFF,
                                mConnectivityListener.isWifiConnected());
                    }
                }
                return true;
            case KEY_WIFI_COLLAPSE:
                final boolean collapse = !mWifiNetworksCategory.isCollapsed();
                mCollapsePref.setTitle(collapse
                        ? R.string.wifi_setting_see_all : R.string.wifi_setting_see_fewer);
                mWifiNetworksCategory.setCollapsed(collapse);
                return true;
            case KEY_WIFI_ALWAYS_SCAN:
                Settings.Global.putInt(getActivity().getContentResolver(),
                        Settings.Global.WIFI_SCAN_ALWAYS_AVAILABLE,
                        mAlwaysScan.isChecked() ? 1 : 0);
                return true;
            case KEY_ETHERNET_STATUS:
                return true;
            case KEY_WIFI_ADD:
                mMetricsFeatureProvider.action(getActivity(),
                        MetricsProto.MetricsEvent.ACTION_WIFI_ADD_NETWORK);
                break;
        }
        return super.onPreferenceTreeClick(preference);
    }

    private void updateConnectivity() {
        if (!isAdded()) {
            return;
        }

        final boolean wifiEnabled = mConnectivityListener.isWifiEnabledOrEnabling();
        mEnableWifiPref.setChecked(wifiEnabled);

        mWifiNetworksCategory.setVisible(wifiEnabled);
        mCollapsePref.setVisible(wifiEnabled && mWifiNetworksCategory.shouldShowCollapsePref());
        mAddPref.setVisible(wifiEnabled);

        if (!wifiEnabled) {
            updateWifiList();
        }

        int scanAlwaysAvailable = 0;
        try {
            scanAlwaysAvailable = Settings.Global.getInt(getContext().getContentResolver(),
                    Settings.Global.WIFI_SCAN_ALWAYS_AVAILABLE);
        } catch (Settings.SettingNotFoundException e) {
            // Ignore
        }
        mAlwaysScan.setChecked(scanAlwaysAvailable == 1);

        final boolean ethernetAvailable = mConnectivityListener.isEthernetAvailable();
        mEthernetCategory.setVisible(ethernetAvailable);
        mEthernetStatusPref.setVisible(ethernetAvailable);
        mEthernetProxyPref.setVisible(ethernetAvailable);
        mEthernetDhcpPref.setVisible(ethernetAvailable);

        if (ethernetAvailable) {
            final boolean ethernetConnected =
                    mConnectivityListener.isEthernetConnected();
            String ethernetIpAddress = mConnectivityListener.getEthernetIpAddress();
            if (ethernetConnected || !TextUtils.isEmpty(ethernetIpAddress)) {
                mEthernetStatusPref.setTitle(R.string.connected);
            } else {
                mEthernetStatusPref.setTitle(R.string.not_connected);
            }
            mEthernetStatusPref.setSummary(ethernetIpAddress);
        }
    }

    private void updateWifiList() {
        if (!isAdded()) {
            return;
        }

        if (!mConnectivityListener.isWifiEnabledOrEnabling()) {
            mWifiNetworksCategory.removeAll();
            mNoWifiUpdateBeforeMillis = 0;
            return;
        }

        final long now = SystemClock.elapsedRealtime();
        if (mNoWifiUpdateBeforeMillis > now) {
            mHandler.removeCallbacks(mInitialUpdateWifiListRunnable);
            mHandler.postDelayed(mInitialUpdateWifiListRunnable,
                    mNoWifiUpdateBeforeMillis - now);
            return;
        }

        final int existingCount = mWifiNetworksCategory.getRealPreferenceCount();
        final Set<Preference> toRemove = new HashSet<>(existingCount);
        for (int i = 0; i < existingCount; i++) {
            toRemove.add(mWifiNetworksCategory.getPreference(i));
        }

        final Context themedContext = getPreferenceManager().getContext();
        final Collection<AccessPoint> accessPoints = mConnectivityListener.getAvailableNetworks();
        int index = 0;
        for (final AccessPoint accessPoint : accessPoints) {
            accessPoint.setListener(this);
            AccessPointPreference pref = (AccessPointPreference) accessPoint.getTag();
            if (pref == null) {
                pref = new AccessPointPreference(accessPoint, themedContext, mUserBadgeCache,
                        false);
                accessPoint.setTag(pref);
            } else {
                toRemove.remove(pref);
            }
            if (accessPoint.isActive()) {
                pref.setFragment(WifiDetailsFragment.class.getName());
                WifiDetailsFragment.prepareArgs(pref.getExtras(), accessPoint);
                pref.setIntent(null);
            } else {
                pref.setFragment(null);
                pref.setIntent(WifiConnectionActivity.createIntent(getContext(), accessPoint));
            }
            pref.setOrder(index++);
            // Double-adding is harmless
            mWifiNetworksCategory.addPreference(pref);
        }

        for (final Preference preference : toRemove) {
            mWifiNetworksCategory.removePreference(preference);
        }

        mCollapsePref.setVisible(mWifiNetworksCategory.shouldShowCollapsePref());
    }

    @Override
    public void onConnectivityChange() {
        updateConnectivity();
    }

    @Override
    public void onWifiListChanged() {
        updateWifiList();
    }

    @Override
    public void onAccessPointChanged(AccessPoint accessPoint) {
        ((AccessPointPreference) accessPoint.getTag()).refresh();
    }

    @Override
    public void onLevelChanged(AccessPoint accessPoint) {
        ((AccessPointPreference) accessPoint.getTag()).onLevelChanged();
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.SETTINGS_NETWORK_CATEGORY;
    }
}
