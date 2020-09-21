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

import android.arch.lifecycle.ViewModel;
import android.net.IpConfiguration;
import android.net.LinkAddress;
import android.net.ProxyInfo;
import android.support.annotation.IntDef;
import android.text.TextUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.net.InetAddress;
import java.util.HashMap;

/**
 * Class that stores the page information for advanced flow.
 */
public class AdvancedOptionsFlowInfo extends ViewModel {
    public static final int ADVANCED_OPTIONS = 1;
    public static final int PROXY_SETTINGS = 2;
    public static final int PROXY_HOSTNAME = 3;
    public static final int PROXY_PORT = 4;
    public static final int PROXY_BYPASS = 5;
    public static final int IP_SETTINGS = 6;
    public static final int IP_ADDRESS = 7;
    public static final int NETWORK_PREFIX_LENGTH = 8;
    public static final int GATEWAY = 9;
    public static final int DNS1 = 10;
    public static final int DNS2 = 11;

    @IntDef({
            ADVANCED_OPTIONS,
            PROXY_SETTINGS,
            PROXY_HOSTNAME,
            PROXY_PORT,
            PROXY_BYPASS,
            IP_SETTINGS,
            IP_ADDRESS,
            NETWORK_PREFIX_LENGTH,
            GATEWAY,
            DNS1,
            DNS2
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface PAGE {
    }

    private IpConfiguration mIpConfiguration;
    private String mPrintableSsid;
    private boolean mSettingsFlow;
    private boolean mCanStart = false;

    private HashMap<Integer, CharSequence> mPageSummary = new HashMap<>();

    /**
     * Store the page summary into a HashMap.
     *
     * @param page The page as the key.
     * @param info The info as the value.
     */
    public void put(@PAGE int page, CharSequence info) {
        mPageSummary.put(page, info);
    }

    /**
     * Check if the summary of the queried page matches with expected string.
     *
     * @param choice The expected string.
     * @param page   The page queried.
     * @return true if matched.
     */
    public boolean choiceChosen(CharSequence choice, @PAGE int page) {
        if (!mPageSummary.containsKey(page)) {
            return false;
        }
        return TextUtils.equals(choice, mPageSummary.get(page));
    }

    /**
     * Check if the Hashmap contains the summary of a particular page.
     *
     * @param page the queried page
     * @return true if contains.
     */
    public boolean containsPage(@PAGE int page) {
        return mPageSummary.containsKey(page);
    }

    /**
     * Get summary of a page.
     *
     * @param page The queried page.
     * @return The summary of the page.
     */
    public String get(@PAGE int page) {
        if (!mPageSummary.containsKey(page)) {
            return "";
        }
        return mPageSummary.get(page).toString();
    }

    /**
     * Remove the summary of a page.
     *
     * @param page The page.
     */
    public void remove(@PAGE int page) {
        mPageSummary.remove(page);
    }

    public IpConfiguration getIpConfiguration() {
        return mIpConfiguration;
    }

    public void setIpConfiguration(IpConfiguration ipConfiguration) {
        this.mIpConfiguration = ipConfiguration;
    }

    public boolean isSettingsFlow() {
        return mSettingsFlow;
    }

    public void setSettingsFlow(boolean settingsFlow) {
        mSettingsFlow = settingsFlow;
    }

    public void setCanStart(boolean canStart) {
        this.mCanStart = canStart;
    }

    /**
     * Determine whether can start advanced flow.
     *
     * @return true if can.
     */
    public boolean canStart() {
        return mCanStart;
    }

    public String getPrintableSsid() {
        return mPrintableSsid;
    }

    public void setPrintableSsid(String ssid) {
        this.mPrintableSsid = ssid;
    }

    /**
     * Get the initial DNS.
     */
    public InetAddress getInitialDns(int index) {
        if (mIpConfiguration != null && mIpConfiguration.getStaticIpConfiguration() != null
                && mIpConfiguration.getStaticIpConfiguration().dnsServers == null) {
            try {
                return mIpConfiguration.getStaticIpConfiguration().dnsServers.get(index);
            } catch (IndexOutOfBoundsException e) {
                return null;
            }
        }
        return null;
    }

    /**
     * Get the initial gateway.
     */
    public InetAddress getInitialGateway() {
        if (mIpConfiguration != null && mIpConfiguration.getStaticIpConfiguration() != null) {
            return mIpConfiguration.getStaticIpConfiguration().gateway;
        } else {
            return null;
        }
    }

    /**
     * Get the initial static ip configuration address.
     */
    public LinkAddress getInitialLinkAddress() {
        if (mIpConfiguration != null && mIpConfiguration.getStaticIpConfiguration() != null) {
            return mIpConfiguration.getStaticIpConfiguration().ipAddress;
        } else {
            return null;
        }
    }

    /**
     * Get the initial proxy info.
     */
    public ProxyInfo getInitialProxyInfo() {
        if (mIpConfiguration != null) {
            return mIpConfiguration.getHttpProxy();
        } else {
            return null;
        }
    }
}
