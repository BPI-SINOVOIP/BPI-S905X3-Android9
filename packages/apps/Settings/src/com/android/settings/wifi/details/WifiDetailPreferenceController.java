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
package com.android.settings.wifi.details;

import static android.net.NetworkCapabilities.NET_CAPABILITY_CAPTIVE_PORTAL;
import static android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;

import android.app.Activity;
import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.Drawable;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.NetworkUtils;
import android.net.RouteInfo;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.support.v4.text.BidiFormatter;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;
import android.util.Log;
import android.widget.ImageView;
import android.widget.Toast;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.Utils;
import com.android.settings.applications.LayoutPreference;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.widget.ActionButtonPreference;
import com.android.settings.widget.EntityHeaderController;
import com.android.settings.wifi.WifiDetailPreference;
import com.android.settings.wifi.WifiDialog;
import com.android.settings.wifi.WifiDialog.WifiDialogListener;
import com.android.settings.wifi.WifiUtils;
import com.android.settingslib.core.AbstractPreferenceController;
import com.android.settingslib.core.instrumentation.MetricsFeatureProvider;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnPause;
import com.android.settingslib.core.lifecycle.events.OnResume;
import com.android.settingslib.wifi.AccessPoint;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.StringJoiner;
import java.util.stream.Collectors;

/**
 * Controller for logic pertaining to displaying Wifi information for the
 * {@link WifiNetworkDetailsFragment}.
 */
public class WifiDetailPreferenceController extends AbstractPreferenceController
        implements PreferenceControllerMixin, WifiDialogListener, LifecycleObserver, OnPause,
        OnResume {

    private static final String TAG = "WifiDetailsPrefCtrl";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    @VisibleForTesting
    static final String KEY_HEADER = "connection_header";
    @VisibleForTesting
    static final String KEY_BUTTONS_PREF = "buttons";
    @VisibleForTesting
    static final String KEY_SIGNAL_STRENGTH_PREF = "signal_strength";
    @VisibleForTesting
    static final String KEY_LINK_SPEED = "link_speed";
    @VisibleForTesting
    static final String KEY_FREQUENCY_PREF = "frequency";
    @VisibleForTesting
    static final String KEY_SECURITY_PREF = "security";
    @VisibleForTesting
    static final String KEY_MAC_ADDRESS_PREF = "mac_address";
    @VisibleForTesting
    static final String KEY_IP_ADDRESS_PREF = "ip_address";
    @VisibleForTesting
    static final String KEY_GATEWAY_PREF = "gateway";
    @VisibleForTesting
    static final String KEY_SUBNET_MASK_PREF = "subnet_mask";
    @VisibleForTesting
    static final String KEY_DNS_PREF = "dns";
    @VisibleForTesting
    static final String KEY_IPV6_CATEGORY = "ipv6_category";
    @VisibleForTesting
    static final String KEY_IPV6_ADDRESSES_PREF = "ipv6_addresses";

    private AccessPoint mAccessPoint;
    private final ConnectivityManager mConnectivityManager;
    private final Fragment mFragment;
    private final Handler mHandler;
    private LinkProperties mLinkProperties;
    private Network mNetwork;
    private NetworkInfo mNetworkInfo;
    private NetworkCapabilities mNetworkCapabilities;
    private int mRssiSignalLevel = -1;
    private String[] mSignalStr;
    private WifiConfiguration mWifiConfig;
    private WifiInfo mWifiInfo;
    private final WifiManager mWifiManager;
    private final MetricsFeatureProvider mMetricsFeatureProvider;

    // UI elements - in order of appearance
    private ActionButtonPreference mButtonsPref;
    private EntityHeaderController mEntityHeaderController;
    private WifiDetailPreference mSignalStrengthPref;
    private WifiDetailPreference mLinkSpeedPref;
    private WifiDetailPreference mFrequencyPref;
    private WifiDetailPreference mSecurityPref;
    private WifiDetailPreference mMacAddressPref;
    private WifiDetailPreference mIpAddressPref;
    private WifiDetailPreference mGatewayPref;
    private WifiDetailPreference mSubnetPref;
    private WifiDetailPreference mDnsPref;
    private PreferenceCategory mIpv6Category;
    private Preference mIpv6AddressPref;

    private final IconInjector mIconInjector;
    private final IntentFilter mFilter;
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            switch (intent.getAction()) {
                case WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION:
                    if (!intent.getBooleanExtra(WifiManager.EXTRA_MULTIPLE_NETWORKS_CHANGED,
                            false /* defaultValue */)) {
                        // only one network changed
                        WifiConfiguration wifiConfiguration = intent
                                .getParcelableExtra(WifiManager.EXTRA_WIFI_CONFIGURATION);
                        if (mAccessPoint.matches(wifiConfiguration)) {
                            mWifiConfig = wifiConfiguration;
                        }
                    }
                    // fall through
                case WifiManager.NETWORK_STATE_CHANGED_ACTION:
                case WifiManager.RSSI_CHANGED_ACTION:
                    updateInfo();
                    break;
            }
        }
    };

    private final NetworkRequest mNetworkRequest = new NetworkRequest.Builder()
            .clearCapabilities().addTransportType(TRANSPORT_WIFI).build();

    // Must be run on the UI thread since it directly manipulates UI state.
    private final NetworkCallback mNetworkCallback = new NetworkCallback() {
        @Override
        public void onLinkPropertiesChanged(Network network, LinkProperties lp) {
            if (network.equals(mNetwork) && !lp.equals(mLinkProperties)) {
                mLinkProperties = lp;
                updateIpLayerInfo();
            }
        }

        private boolean hasCapabilityChanged(NetworkCapabilities nc, int cap) {
            // If this is the first time we get NetworkCapabilities, report that something changed.
            if (mNetworkCapabilities == null) return true;

            // nc can never be null, see ConnectivityService#callCallbackForRequest.
            return mNetworkCapabilities.hasCapability(cap) != nc.hasCapability(cap);
        }

        @Override
        public void onCapabilitiesChanged(Network network, NetworkCapabilities nc) {
            // If the network just validated or lost Internet access, refresh network state.
            // Don't do this on every NetworkCapabilities change because refreshNetworkState
            // sends IPCs to the system server from the UI thread, which can cause jank.
            if (network.equals(mNetwork) && !nc.equals(mNetworkCapabilities)) {
                if (hasCapabilityChanged(nc, NET_CAPABILITY_VALIDATED) ||
                        hasCapabilityChanged(nc, NET_CAPABILITY_CAPTIVE_PORTAL)) {
                    refreshNetworkState();
                }
                mNetworkCapabilities = nc;
                updateIpLayerInfo();
            }
        }

        @Override
        public void onLost(Network network) {
            if (network.equals(mNetwork)) {
                exitActivity();
            }
        }
    };

    public static WifiDetailPreferenceController newInstance(
            AccessPoint accessPoint,
            ConnectivityManager connectivityManager,
            Context context,
            Fragment fragment,
            Handler handler,
            Lifecycle lifecycle,
            WifiManager wifiManager,
            MetricsFeatureProvider metricsFeatureProvider) {
        return new WifiDetailPreferenceController(
                accessPoint, connectivityManager, context, fragment, handler, lifecycle,
                wifiManager, metricsFeatureProvider, new IconInjector(context));
    }

    @VisibleForTesting
        /* package */ WifiDetailPreferenceController(
            AccessPoint accessPoint,
            ConnectivityManager connectivityManager,
            Context context,
            Fragment fragment,
            Handler handler,
            Lifecycle lifecycle,
            WifiManager wifiManager,
            MetricsFeatureProvider metricsFeatureProvider,
            IconInjector injector) {
        super(context);

        mAccessPoint = accessPoint;
        mConnectivityManager = connectivityManager;
        mFragment = fragment;
        mHandler = handler;
        mSignalStr = context.getResources().getStringArray(R.array.wifi_signal);
        mWifiConfig = accessPoint.getConfig();
        mWifiManager = wifiManager;
        mMetricsFeatureProvider = metricsFeatureProvider;
        mIconInjector = injector;

        mFilter = new IntentFilter();
        mFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        mFilter.addAction(WifiManager.RSSI_CHANGED_ACTION);
        mFilter.addAction(WifiManager.CONFIGURED_NETWORKS_CHANGED_ACTION);

        lifecycle.addObserver(this);
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        // Returns null since this controller contains more than one Preference
        return null;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);

        setupEntityHeader(screen);

        mButtonsPref = ((ActionButtonPreference) screen.findPreference(KEY_BUTTONS_PREF))
                .setButton1Text(R.string.forget)
                .setButton1Positive(false)
                .setButton1OnClickListener(view -> forgetNetwork())
                .setButton2Text(R.string.wifi_sign_in_button_text)
                .setButton2Positive(true)
                .setButton2OnClickListener(view -> signIntoNetwork());

        mSignalStrengthPref =
                (WifiDetailPreference) screen.findPreference(KEY_SIGNAL_STRENGTH_PREF);
        mLinkSpeedPref = (WifiDetailPreference) screen.findPreference(KEY_LINK_SPEED);
        mFrequencyPref = (WifiDetailPreference) screen.findPreference(KEY_FREQUENCY_PREF);
        mSecurityPref = (WifiDetailPreference) screen.findPreference(KEY_SECURITY_PREF);

        mMacAddressPref = (WifiDetailPreference) screen.findPreference(KEY_MAC_ADDRESS_PREF);
        mIpAddressPref = (WifiDetailPreference) screen.findPreference(KEY_IP_ADDRESS_PREF);
        mGatewayPref = (WifiDetailPreference) screen.findPreference(KEY_GATEWAY_PREF);
        mSubnetPref = (WifiDetailPreference) screen.findPreference(KEY_SUBNET_MASK_PREF);
        mDnsPref = (WifiDetailPreference) screen.findPreference(KEY_DNS_PREF);

        mIpv6Category = (PreferenceCategory) screen.findPreference(KEY_IPV6_CATEGORY);
        mIpv6AddressPref = screen.findPreference(KEY_IPV6_ADDRESSES_PREF);

        mSecurityPref.setDetailText(mAccessPoint.getSecurityString(false /* concise */));
    }

    private void setupEntityHeader(PreferenceScreen screen) {
        LayoutPreference headerPref = (LayoutPreference) screen.findPreference(KEY_HEADER);
        mEntityHeaderController =
                EntityHeaderController.newInstance(
                        mFragment.getActivity(), mFragment,
                        headerPref.findViewById(R.id.entity_header));

        ImageView iconView = headerPref.findViewById(R.id.entity_header_icon);
        iconView.setBackground(
                mContext.getDrawable(R.drawable.ic_settings_widget_background));
        iconView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);

        mEntityHeaderController.setLabel(mAccessPoint.getSsidStr());
    }

    @Override
    public void onResume() {
        // Ensure mNetwork is set before any callbacks above are delivered, since our
        // NetworkCallback only looks at changes to mNetwork.
        mNetwork = mWifiManager.getCurrentNetwork();
        mLinkProperties = mConnectivityManager.getLinkProperties(mNetwork);
        mNetworkCapabilities = mConnectivityManager.getNetworkCapabilities(mNetwork);
        updateInfo();
        mContext.registerReceiver(mReceiver, mFilter);
        mConnectivityManager.registerNetworkCallback(mNetworkRequest, mNetworkCallback,
                mHandler);
    }

    @Override
    public void onPause() {
        mNetwork = null;
        mLinkProperties = null;
        mNetworkCapabilities = null;
        mNetworkInfo = null;
        mWifiInfo = null;
        mContext.unregisterReceiver(mReceiver);
        mConnectivityManager.unregisterNetworkCallback(mNetworkCallback);
    }

    private void updateInfo() {
        // No need to fetch LinkProperties and NetworkCapabilities, they are updated by the
        // callbacks. mNetwork doesn't change except in onResume.
        mNetworkInfo = mConnectivityManager.getNetworkInfo(mNetwork);
        mWifiInfo = mWifiManager.getConnectionInfo();
        if (mNetwork == null || mNetworkInfo == null || mWifiInfo == null) {
            exitActivity();
            return;
        }

        // Update whether the forget button should be displayed.
        mButtonsPref.setButton1Visible(canForgetNetwork());

        refreshNetworkState();

        // Update Connection Header icon and Signal Strength Preference
        refreshRssiViews();

        // MAC Address Pref
        mMacAddressPref.setDetailText(mWifiInfo.getMacAddress());

        // Link Speed Pref
        int linkSpeedMbps = mWifiInfo.getLinkSpeed();
        mLinkSpeedPref.setVisible(linkSpeedMbps >= 0);
        mLinkSpeedPref.setDetailText(mContext.getString(
                R.string.link_speed, mWifiInfo.getLinkSpeed()));

        // Frequency Pref
        final int frequency = mWifiInfo.getFrequency();
        String band = null;
        if (frequency >= AccessPoint.LOWER_FREQ_24GHZ
                && frequency < AccessPoint.HIGHER_FREQ_24GHZ) {
            band = mContext.getResources().getString(R.string.wifi_band_24ghz);
        } else if (frequency >= AccessPoint.LOWER_FREQ_5GHZ
                && frequency < AccessPoint.HIGHER_FREQ_5GHZ) {
            band = mContext.getResources().getString(R.string.wifi_band_5ghz);
        } else {
            Log.e(TAG, "Unexpected frequency " + frequency);
        }
        mFrequencyPref.setDetailText(band);

        updateIpLayerInfo();
    }

    private void exitActivity() {
        if (DEBUG) {
            Log.d(TAG, "Exiting the WifiNetworkDetailsPage");
        }
        mFragment.getActivity().finish();
    }

    private void refreshNetworkState() {
        mAccessPoint.update(mWifiConfig, mWifiInfo, mNetworkInfo);
        mEntityHeaderController.setSummary(mAccessPoint.getSettingsSummary())
                .done(mFragment.getActivity(), true /* rebind */);
    }

    private void refreshRssiViews() {
        int signalLevel = mAccessPoint.getLevel();

        if (mRssiSignalLevel == signalLevel) {
            return;
        }
        mRssiSignalLevel = signalLevel;
        Drawable wifiIcon = mIconInjector.getIcon(mRssiSignalLevel);

        wifiIcon.setTint(Utils.getColorAccent(mContext));
        mEntityHeaderController.setIcon(wifiIcon).done(mFragment.getActivity(), true /* rebind */);

        Drawable wifiIconDark = wifiIcon.getConstantState().newDrawable().mutate();
        wifiIconDark.setTint(mContext.getResources().getColor(
                R.color.wifi_details_icon_color, mContext.getTheme()));
        mSignalStrengthPref.setIcon(wifiIconDark);

        mSignalStrengthPref.setDetailText(mSignalStr[mRssiSignalLevel]);
    }

    private void updatePreference(WifiDetailPreference pref, String detailText) {
        if (!TextUtils.isEmpty(detailText)) {
            pref.setDetailText(detailText);
            pref.setVisible(true);
        } else {
            pref.setVisible(false);
        }
    }

    private void updateIpLayerInfo() {
        mButtonsPref.setButton2Visible(canSignIntoNetwork());
        mButtonsPref.setVisible(canSignIntoNetwork() || canForgetNetwork());

        if (mNetwork == null || mLinkProperties == null) {
            mIpAddressPref.setVisible(false);
            mSubnetPref.setVisible(false);
            mGatewayPref.setVisible(false);
            mDnsPref.setVisible(false);
            mIpv6Category.setVisible(false);
            return;
        }

        // Find IPv4 and IPv6 addresses.
        String ipv4Address = null;
        String subnet = null;
        StringJoiner ipv6Addresses = new StringJoiner("\n");

        for (LinkAddress addr : mLinkProperties.getLinkAddresses()) {
            if (addr.getAddress() instanceof Inet4Address) {
                ipv4Address = addr.getAddress().getHostAddress();
                subnet = ipv4PrefixLengthToSubnetMask(addr.getPrefixLength());
            } else if (addr.getAddress() instanceof Inet6Address) {
                ipv6Addresses.add(addr.getAddress().getHostAddress());
            }
        }

        // Find IPv4 default gateway.
        String gateway = null;
        for (RouteInfo routeInfo : mLinkProperties.getRoutes()) {
            if (routeInfo.isIPv4Default() && routeInfo.hasGateway()) {
                gateway = routeInfo.getGateway().getHostAddress();
                break;
            }
        }

        // Find all (IPv4 and IPv6) DNS addresses.
        String dnsServers = mLinkProperties.getDnsServers().stream()
                .map(InetAddress::getHostAddress)
                .collect(Collectors.joining("\n"));

        // Update UI.
        updatePreference(mIpAddressPref, ipv4Address);
        updatePreference(mSubnetPref, subnet);
        updatePreference(mGatewayPref, gateway);
        updatePreference(mDnsPref, dnsServers);

        if (ipv6Addresses.length() > 0) {
            mIpv6AddressPref.setSummary(
                    BidiFormatter.getInstance().unicodeWrap(ipv6Addresses.toString()));
            mIpv6Category.setVisible(true);
        } else {
            mIpv6Category.setVisible(false);
        }
    }

    private static String ipv4PrefixLengthToSubnetMask(int prefixLength) {
        try {
            InetAddress all = InetAddress.getByAddress(
                    new byte[] {(byte) 255, (byte) 255, (byte) 255, (byte) 255});
            return NetworkUtils.getNetworkPart(all, prefixLength).getHostAddress();
        } catch (UnknownHostException e) {
            return null;
        }
    }

    /**
     * Returns whether the network represented by this preference can be forgotten.
     */
    private boolean canForgetNetwork() {
        return (mWifiInfo != null && mWifiInfo.isEphemeral()) || canModifyNetwork();
    }

    /**
     * Returns whether the network represented by this preference can be modified.
     */
    public boolean canModifyNetwork() {
        return mWifiConfig != null && !WifiUtils.isNetworkLockedDown(mContext, mWifiConfig);
    }

    /**
     * Returns whether the user can sign into the network represented by this preference.
     */
    private boolean canSignIntoNetwork() {
        return WifiUtils.canSignIntoNetwork(mNetworkCapabilities);
    }

    /**
     * Forgets the wifi network associated with this preference.
     */
    private void forgetNetwork() {
        if (mWifiInfo != null && mWifiInfo.isEphemeral()) {
            mWifiManager.disableEphemeralNetwork(mWifiInfo.getSSID());
        } else if (mWifiConfig != null) {
            if (mWifiConfig.isPasspoint()) {
                mWifiManager.removePasspointConfiguration(mWifiConfig.FQDN);
            } else {
                mWifiManager.forget(mWifiConfig.networkId, null /* action listener */);
            }
        }
        mMetricsFeatureProvider.action(
                mFragment.getActivity(), MetricsProto.MetricsEvent.ACTION_WIFI_FORGET);
        mFragment.getActivity().finish();
    }

    /**
     * Sign in to the captive portal found on this wifi network associated with this preference.
     */
    private void signIntoNetwork() {
        mMetricsFeatureProvider.action(
                mFragment.getActivity(), MetricsProto.MetricsEvent.ACTION_WIFI_SIGNIN);
        mConnectivityManager.startCaptivePortalApp(mNetwork);
    }

    @Override
    public void onForget(WifiDialog dialog) {
        // can't forget network from a 'modify' dialog
    }

    @Override
    public void onSubmit(WifiDialog dialog) {
        if (dialog.getController() != null) {
            mWifiManager.save(dialog.getController().getConfig(), new WifiManager.ActionListener() {
                @Override
                public void onSuccess() {
                }

                @Override
                public void onFailure(int reason) {
                    Activity activity = mFragment.getActivity();
                    if (activity != null) {
                        Toast.makeText(activity,
                                R.string.wifi_failed_save_message,
                                Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }
    }

    /**
     * Wrapper for testing compatibility.
     */
    @VisibleForTesting
    static class IconInjector {
        private final Context mContext;

        public IconInjector(Context context) {
            mContext = context;
        }

        public Drawable getIcon(int level) {
            return mContext.getDrawable(Utils.getWifiIconResource(level)).mutate();
        }
    }
}
