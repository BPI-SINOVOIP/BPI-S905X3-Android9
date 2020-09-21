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

import android.content.Context;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;

/**
 * Used to get different wifi security types.
 */
public class WifiSecurityUtil {
    /**
     * Get security based on the {@link ScanResult}
     *
     * @param result {@link ScanResult}
     * @return the category of wifi security.
     */
    public static int getSecurity(ScanResult result) {
        if (result.capabilities.contains("WEP")) {
            return AccessPoint.SECURITY_WEP;
        } else if (result.capabilities.contains("PSK")) {
            return AccessPoint.SECURITY_PSK;
        } else if (result.capabilities.contains("EAP")) {
            return AccessPoint.SECURITY_EAP;
        }
        return AccessPoint.SECURITY_NONE;
    }

    /**
     * Get security based on {@link WifiConfiguration}
     *
     * @param config {@link WifiConfiguration}
     * @return the category of wifi security.
     */
    public static int getSecurity(WifiConfiguration config) {
        if (config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_PSK)) {
            return AccessPoint.SECURITY_PSK;
        }
        if (config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_EAP)
                || config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.IEEE8021X)) {
            return AccessPoint.SECURITY_EAP;
        }
        return (config.wepKeys[0] != null) ? AccessPoint.SECURITY_WEP : AccessPoint.SECURITY_NONE;
    }

    /**
     * Get the name of a specified wifi security.
     *
     * @param context      context of application.
     * @param wifiSecurity the value of wifiSecurity, defined in {@link AccessPoint}.
     * @return the name
     */
    public static String getName(Context context, int wifiSecurity) {
        switch (wifiSecurity) {
            case AccessPoint.SECURITY_WEP:
                return context.getString(R.string.wifi_security_type_wep);
            case AccessPoint.SECURITY_PSK:
                return context.getString(R.string.wifi_security_type_wpa);
            case AccessPoint.SECURITY_EAP:
                return context.getString(R.string.wifi_security_type_eap);
            case AccessPoint.SECURITY_NONE:
                return context.getString(R.string.wifi_security_type_none);
            default:
                return null;
        }
    }

    /**
     * Return whether wifiSecurity is open.
     *
     * @param wifiSecurity the value of wifiSecurity. Defined in {@link AccessPoint}.
     * @return true if open.
     */
    public static boolean isOpen(int wifiSecurity) {
        return wifiSecurity == AccessPoint.SECURITY_NONE;
    }
}
