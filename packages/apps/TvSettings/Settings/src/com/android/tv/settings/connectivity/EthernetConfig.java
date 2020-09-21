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

import android.content.Context;
import android.net.EthernetManager;
import android.net.IpConfiguration;
import android.net.wifi.WifiManager;

import com.android.tv.settings.R;

/**
 * Ethernet configuration that implements NetworkConfiguration.
 */
class EthernetConfig implements NetworkConfiguration {
    private final EthernetManager mEthernetManager;
    private IpConfiguration mIpConfiguration;
    private final String mName;
    private String mInterfaceName;

    EthernetConfig(Context context) {
        mEthernetManager = (EthernetManager) context.getSystemService(Context.ETHERNET_SERVICE);
        mIpConfiguration = new IpConfiguration();
        mName = context.getResources().getString(R.string.connectivity_ethernet);
    }

    @Override
    public void setIpConfiguration(IpConfiguration configuration) {
        mIpConfiguration = configuration;
    }

    @Override
    public IpConfiguration getIpConfiguration() {
        return mIpConfiguration;
    }

    @Override
    public void save(WifiManager.ActionListener listener) {
        if (mInterfaceName != null) {
            mEthernetManager.setConfiguration(mInterfaceName, mIpConfiguration);
        }

        if (listener != null) {
            listener.onSuccess();
        }
    }

    /**
     * Load IpConfiguration from system.
     */
    public void load() {
        String[] ifaces = mEthernetManager.getAvailableInterfaces();
        if (ifaces.length > 0) {
            mInterfaceName = ifaces[0];
            mIpConfiguration = mEthernetManager.getConfiguration(mInterfaceName);
        }
    }

    @Override
    public String getPrintableName() {
        return mName;
    }

}
