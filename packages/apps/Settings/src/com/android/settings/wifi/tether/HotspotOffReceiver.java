
package com.android.settings.wifi.tether;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.util.Log;

/**
 * This receiver catches when quick settings turns off the hotspot, so we can
 * cancel the alarm in that case.  All other cancels are handled in tethersettings.
 */
public class HotspotOffReceiver extends BroadcastReceiver {

    private static final String TAG = "HotspotOffReceiver";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    private Context mContext;
    private boolean mRegistered;

    public HotspotOffReceiver(Context context) {
        mContext = context;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (WifiManager.WIFI_AP_STATE_CHANGED_ACTION.equals(intent.getAction())) {
            WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            if (wifiManager.getWifiApState() == WifiManager.WIFI_AP_STATE_DISABLED) {
                if (DEBUG) Log.d(TAG, "TetherService.cancelRecheckAlarmIfNecessary called");
                // The hotspot has been turned off, we don't need to recheck tethering.
                TetherService.cancelRecheckAlarmIfNecessary(
                        context, ConnectivityManager.TETHERING_WIFI);
            }
        }
    }

    public void register() {
        if (!mRegistered) {
            mContext.registerReceiver(this,
                new IntentFilter(WifiManager.WIFI_AP_STATE_CHANGED_ACTION));
            mRegistered = true;
        }
    }

    public void unregister() {
        if (mRegistered) {
            mContext.unregisterReceiver(this);
            mRegistered = false;
        }
    }
}
