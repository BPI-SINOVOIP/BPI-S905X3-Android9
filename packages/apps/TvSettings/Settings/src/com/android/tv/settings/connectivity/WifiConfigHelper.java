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

package com.android.tv.settings.connectivity;

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.AuthAlgorithm;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.WifiSecurityUtil;

import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Helper class that deals with Wi-fi configuration.
 */
public final class WifiConfigHelper {

    private static final String TAG = "WifiConfigHelper";
    private static final boolean DEBUG = false;

    // Allows underscore char to supports proxies that do not
    // follow the spec
    private static final String HC = "a-zA-Z0-9\\_";

    // Matches blank input, ips, and domain names
    private static final String HOSTNAME_REGEXP =
            "^$|^[" + HC + "]+(\\-[" + HC + "]+)*(\\.[" + HC + "]+(\\-[" + HC + "]+)*)*$";
    private static final Pattern HOSTNAME_PATTERN;
    private static final String EXCLUSION_REGEXP =
            "$|^(\\*)?\\.?[" + HC + "]+(\\-[" + HC + "]+)*(\\.[" + HC + "]+(\\-[" + HC + "]+)*)*$";
    private static final Pattern EXCLUSION_PATTERN;

    static {
        HOSTNAME_PATTERN = Pattern.compile(HOSTNAME_REGEXP);
        EXCLUSION_PATTERN = Pattern.compile(EXCLUSION_REGEXP);
    }

    private WifiConfigHelper() {
    }

    /**
     * Set configuration ssid.
     *
     * @param config configuration
     * @param ssid   network ssid
     */
    public static void setConfigSsid(WifiConfiguration config, String ssid) {
        config.SSID = AccessPoint.convertToQuotedString(ssid);
    }

    /**
     * Set configuration key managment by security.
     */
    public static void setConfigKeyManagementBySecurity(
            WifiConfiguration config, int security) {
        config.allowedKeyManagement.clear();
        config.allowedAuthAlgorithms.clear();
        switch (security) {
            case AccessPoint.SECURITY_NONE:
                config.allowedKeyManagement.set(KeyMgmt.NONE);
                break;
            case AccessPoint.SECURITY_WEP:
                config.allowedKeyManagement.set(KeyMgmt.NONE);
                config.allowedAuthAlgorithms.set(AuthAlgorithm.OPEN);
                config.allowedAuthAlgorithms.set(AuthAlgorithm.SHARED);
                break;
            case AccessPoint.SECURITY_PSK:
                config.allowedKeyManagement.set(KeyMgmt.WPA_PSK);
                break;
            case AccessPoint.SECURITY_EAP:
                config.allowedKeyManagement.set(KeyMgmt.WPA_EAP);
                config.allowedKeyManagement.set(KeyMgmt.IEEE8021X);
                break;
        }
    }

    /**
     * validate syntax of hostname and port entries
     *
     * @return 0 on success, string resource ID on failure
     */
    public static int validate(String hostname, String port, String exclList) {
        Matcher match = HOSTNAME_PATTERN.matcher(hostname);
        String[] exclListArray = exclList.split(",");

        if (!match.matches()) return R.string.proxy_error_invalid_host;

        for (String excl : exclListArray) {
            Matcher m = EXCLUSION_PATTERN.matcher(excl);
            if (!m.matches()) return R.string.proxy_error_invalid_exclusion_list;
        }

        if (hostname.length() > 0 && port.length() == 0) {
            return R.string.proxy_error_empty_port;
        }

        if (port.length() > 0) {
            if (hostname.length() == 0) {
                return R.string.proxy_error_empty_host_set_port;
            }
            int portVal = -1;
            try {
                portVal = Integer.parseInt(port);
            } catch (NumberFormatException ex) {
                return R.string.proxy_error_invalid_port;
            }
            if (portVal <= 0 || portVal > 0xFFFF) {
                return R.string.proxy_error_invalid_port;
            }
        }
        return 0;
    }

    /**
     * Get {@link WifiConfiguration} based upon the {@link WifiManager} and networkId.
     * @param wifiManager
     * @param networkId the id of the network.
     * @return the {@link WifiConfiguration} of the specified network.
     */
    public static WifiConfiguration getWifiConfiguration(WifiManager wifiManager, int networkId) {
        List<WifiConfiguration> configuredNetworks = wifiManager.getConfiguredNetworks();
        if (configuredNetworks != null) {
            for (WifiConfiguration configuredNetwork : configuredNetworks) {
                if (configuredNetwork.networkId == networkId) {
                    return configuredNetwork;
                }
            }
        }
        return null;
    }

    /**
     * Did this config come out of the supplicant?  NOT "Is the config currently in the supplicant?"
     */
    public static boolean isNetworkSaved(WifiConfiguration config) {
        return config != null && config.networkId > -1;
    }

    /**
     * Return the configured network that matches the ssid/security pair, or create one.
     */
    public static WifiConfiguration getConfiguration(Context context, String ssid, int security) {
        WifiConfiguration config = getFromConfiguredNetworks(context, ssid, security);

        if (config == null) {
            // No configured network found; populate a new one with the provided ssid / security.
            config = new WifiConfiguration();
            setConfigSsid(config, ssid);
            setConfigKeyManagementBySecurity(config, security);
        }
        return config;
    }

    /**
     * Save a wifi configuration.
     */
    public static boolean saveConfiguration(Context context, WifiConfiguration config) {
        if (config == null) {
            return false;
        }

        WifiManager wifiMan = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        int networkId = wifiMan.addNetwork(config);
        if (networkId == -1) {
            if (DEBUG) Log.e(TAG, "failed to add network: " + config.toString());
            return false;
        }

        if (!wifiMan.enableNetwork(networkId, false)) {
            if (DEBUG) Log.e(TAG, "enable network failed: " + networkId + "; " + config.toString());
            return false;
        }

        if (!wifiMan.saveConfiguration()) {
            if (DEBUG) Log.e(TAG, "failed to save: " + config.toString());
            return false;
        }

        if (DEBUG) Log.d(TAG, "saved network: " + config.toString());
        return true;
    }

    /**
     * @return A matching WifiConfiguration from the list of configured
     * networks, or null if no matching network is found.
     */
    private static WifiConfiguration getFromConfiguredNetworks(Context context,
            String ssid,
            int security) {
        WifiManager wifiMan = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        List<WifiConfiguration> configuredNetworks = wifiMan.getConfiguredNetworks();
        if (configuredNetworks != null) {
            for (WifiConfiguration configuredNetwork : configuredNetworks) {
                if (configuredNetwork == null || configuredNetwork.SSID == null) {
                    continue;  // Does this ever really happen?
                }

                // If the SSID and the security match, that's our network.
                String configuredSsid = WifiInfo.removeDoubleQuotes(configuredNetwork.SSID);
                if (TextUtils.equals(configuredSsid, ssid)) {
                    int configuredSecurity = WifiSecurityUtil.getSecurity(configuredNetwork);
                    if (configuredSecurity == security) {
                        return configuredNetwork;
                    }
                }
            }
        }

        return null;
    }
}
