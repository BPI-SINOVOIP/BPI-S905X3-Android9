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

package com.android.tv.settings.testutils;

import android.content.Context;
import android.net.wifi.WifiConfiguration;

import com.android.tv.settings.connectivity.WifiConfigHelper;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;


@Implements(value = WifiConfigHelper.class, inheritImplementationMethods = true)
public class ShadowWifiConfigHelper {
    private static WifiConfiguration sConfigForTest;

    @Implementation
    public static WifiConfiguration getConfiguration(Context context, String ssid, int security) {
        WifiConfiguration config = new WifiConfiguration();
        WifiConfigHelper.setConfigSsid(config, ssid);
        WifiConfigHelper.setConfigKeyManagementBySecurity(config, security);
        return config;
    }

    @Implementation
    public static boolean saveConfiguration(Context context, WifiConfiguration config) {
        sConfigForTest = config;
        return true;
    }

    @Implementation
    public static boolean isNetworkSaved(WifiConfiguration config) {
        return (config != null && sConfigForTest.networkId > -1);
    }
}
