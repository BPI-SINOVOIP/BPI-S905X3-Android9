/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC EthernetWifiSwitch
 */

package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

public class EthernetWifiSwitch extends BroadcastReceiver {
    private static final String TAG = "EthernetWifiSwitch";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);
        if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action)) {
            WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo ethInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
            if (ethInfo.getState() == NetworkInfo.State.CONNECTED) {
                int wifiState = wm.getWifiState();
                if ((wifiState == WifiManager.WIFI_STATE_ENABLING) ||
                    (wifiState == WifiManager.WIFI_STATE_ENABLED)) {
                    wm.setWifiEnabled(false);
                    SettingsPref.setWiFiSaveState(context, 1);
                }
            }
            else {
                if (1 == SettingsPref.getWiFiSaveState(context)) {
                    wm.setWifiEnabled(true);
                    SettingsPref.setWiFiSaveState(context, 0);
                }
            }
        }
    }
}

