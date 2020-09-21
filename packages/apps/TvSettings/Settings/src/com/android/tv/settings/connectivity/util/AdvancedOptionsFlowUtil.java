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

package com.android.tv.settings.connectivity.util;

import android.arch.lifecycle.ViewModelProviders;
import android.net.IpConfiguration;
import android.net.LinkAddress;
import android.net.NetworkUtils;
import android.net.ProxyInfo;
import android.net.StaticIpConfiguration;
import android.support.v4.app.FragmentActivity;
import android.text.TextUtils;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.WifiConfigHelper;
import com.android.tv.settings.connectivity.setup.AdvancedOptionsFlowInfo;

import java.net.Inet4Address;

/**
 * Class that helps process proxy and IP settings.
 */
public class AdvancedOptionsFlowUtil {

    /**
     * Process Proxy Settings.
     *
     * @param activity the activity that calls this method.
     * @return 0 if successful, or other different values based on the result.
     */
    public static int processProxySettings(FragmentActivity activity) {
        AdvancedOptionsFlowInfo flowInfo = ViewModelProviders
                .of(activity)
                .get(AdvancedOptionsFlowInfo.class);
        IpConfiguration mIpConfiguration = flowInfo.getIpConfiguration();
        boolean hasProxySettings =
                (!flowInfo.containsPage(AdvancedOptionsFlowInfo.ADVANCED_OPTIONS)
                        || !flowInfo.choiceChosen(
                                activity.getString(R.string.wifi_action_advanced_no),
                                AdvancedOptionsFlowInfo.ADVANCED_OPTIONS))
                && !flowInfo.choiceChosen(activity.getString(R.string.wifi_action_proxy_none),
                                            AdvancedOptionsFlowInfo.PROXY_SETTINGS);
        mIpConfiguration.setProxySettings(hasProxySettings
                ? IpConfiguration.ProxySettings.STATIC : IpConfiguration.ProxySettings.NONE);
        if (hasProxySettings) {
            String host = flowInfo.get(AdvancedOptionsFlowInfo.PROXY_HOSTNAME);
            String portStr = flowInfo.get(AdvancedOptionsFlowInfo.PROXY_PORT);
            String exclusionList = flowInfo.get(AdvancedOptionsFlowInfo.PROXY_BYPASS);
            int port = 0;
            int result;
            try {
                port = Integer.parseInt(portStr);
                result = WifiConfigHelper.validate(host, portStr, exclusionList);
            } catch (NumberFormatException e) {
                result = R.string.proxy_error_invalid_port;
            }
            if (result == 0) {
                mIpConfiguration.setHttpProxy(new ProxyInfo(host, port, exclusionList));
            } else {
                return result;
            }
        } else {
            mIpConfiguration.setHttpProxy(null);
        }
        return 0;
    }

    /**
     * Process Ip Settings.
     *
     * @param activity the activity that calls this method.
     * @return 0 if successful, or other different values based on the result.
     */
    public static int processIpSettings(FragmentActivity activity) {
        AdvancedOptionsFlowInfo flowInfo = ViewModelProviders
                .of(activity)
                .get(AdvancedOptionsFlowInfo.class);
        IpConfiguration mIpConfiguration = flowInfo.getIpConfiguration();
        boolean hasIpSettings =
                (!flowInfo.containsPage(AdvancedOptionsFlowInfo.ADVANCED_OPTIONS)
                        || !flowInfo.choiceChosen(
                                activity.getString(R.string.wifi_action_advanced_no),
                                AdvancedOptionsFlowInfo.ADVANCED_OPTIONS))
                && !flowInfo.choiceChosen(activity.getString(R.string.wifi_action_dhcp),
                                            AdvancedOptionsFlowInfo.IP_SETTINGS);
        mIpConfiguration.setIpAssignment(hasIpSettings
                        ? IpConfiguration.IpAssignment.STATIC
                        : IpConfiguration.IpAssignment.DHCP);
        if (hasIpSettings) {
            StaticIpConfiguration staticConfig = new StaticIpConfiguration();
            mIpConfiguration.setStaticIpConfiguration(staticConfig);
            String ipAddr = flowInfo.get(AdvancedOptionsFlowInfo.IP_ADDRESS);
            if (TextUtils.isEmpty(ipAddr)) {
                return R.string.wifi_ip_settings_invalid_ip_address;
            }
            Inet4Address inetAddr;
            try {
                inetAddr = (Inet4Address) NetworkUtils.numericToInetAddress(ipAddr);
            } catch (IllegalArgumentException | ClassCastException e) {
                return R.string.wifi_ip_settings_invalid_ip_address;
            }

            try {
                int networkPrefixLength = Integer.parseInt(flowInfo.get(
                        AdvancedOptionsFlowInfo.NETWORK_PREFIX_LENGTH));
                if (networkPrefixLength < 0 || networkPrefixLength > 32) {
                    return R.string.wifi_ip_settings_invalid_network_prefix_length;
                }
                staticConfig.ipAddress = new LinkAddress(inetAddr, networkPrefixLength);
            } catch (NumberFormatException e) {
                return R.string.wifi_ip_settings_invalid_ip_address;
            }

            String gateway = flowInfo.get(AdvancedOptionsFlowInfo.GATEWAY);
            if (!TextUtils.isEmpty(gateway)) {
                try {
                    staticConfig.gateway =
                            NetworkUtils.numericToInetAddress(gateway);
                } catch (IllegalArgumentException | ClassCastException e) {
                    return R.string.wifi_ip_settings_invalid_gateway;
                }
            }

            String dns1 = flowInfo.get(AdvancedOptionsFlowInfo.DNS1);
            if (!TextUtils.isEmpty(dns1)) {
                try {
                    staticConfig.dnsServers.add(
                            NetworkUtils.numericToInetAddress(dns1));
                } catch (IllegalArgumentException | ClassCastException e) {
                    return R.string.wifi_ip_settings_invalid_dns;
                }
            }

            String dns2 = flowInfo.get(AdvancedOptionsFlowInfo.DNS2);
            if (!TextUtils.isEmpty(dns2)) {
                try {
                    staticConfig.dnsServers.add(
                            NetworkUtils.numericToInetAddress(dns2));
                } catch (IllegalArgumentException | ClassCastException e) {
                    return R.string.wifi_ip_settings_invalid_dns;
                }
            }
        } else {
            mIpConfiguration.setStaticIpConfiguration(null);
        }
        return 0;
    }
}
