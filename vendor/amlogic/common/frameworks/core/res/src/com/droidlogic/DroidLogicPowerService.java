/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DroidLogicPowerService
 */

package com.droidlogic;

import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.util.Log;

import com.droidlogic.app.SystemControlManager;

public class DroidLogicPowerService extends Service {
    private static final String TAG = "DroidLogicPowerService";
    private SystemControlManager mSystemControlManager = null;
    private boolean mWifiDisableWhenSuspend = false;
    private static final int POWER_SUSPEND_OFF = 0;
    private static final int POWER_SUSPEND_ON = 1;
    private static final int POWER_SUSPEND_SHUTDOWN = 2;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "action: " + action);
            if (Intent.ACTION_SCREEN_ON.equals(action)) {
                setSuspendState(POWER_SUSPEND_OFF);
                setWifiState(context, true);
            } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                setSuspendState(POWER_SUSPEND_ON);
                setWifiState(context, false);
            } else if (Intent.ACTION_SHUTDOWN.equals(action)) {
                setSuspendState(POWER_SUSPEND_SHUTDOWN);
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mSystemControlManager = SystemControlManager.getInstance();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SHUTDOWN);
        registerReceiver (mReceiver, filter);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void setWifiState(Context context, boolean state) {
        if (mSystemControlManager.getPropertyBoolean("ro.vendor.platform.wifi.suspend", false) == false) {
            return;
        }

        WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);

        if (state) {
            if (mWifiDisableWhenSuspend == true) {
                try {
                    wm.setWifiEnabled(true);
                    mWifiDisableWhenSuspend = false;
                } catch (Exception e) {
                    /* ignore - local call */
                }
            }
        } else {
            int wifiState = wm.getWifiState();
            if (wifiState == WifiManager.WIFI_STATE_ENABLING
                    || wifiState == WifiManager.WIFI_STATE_ENABLED) {
                try {
                    wm.setWifiEnabled(false);
                    mWifiDisableWhenSuspend = true;
                } catch (Exception e) {
                    /* ignore - local call */
                }
                try {
                    Thread.sleep(2300);
                } catch (InterruptedException ignore) {
                }
            }
        }

        Log.d(TAG, "setWifiState: " + state);
    }

    private static final String KILL_ESM_PATH = "/sys/module/tvin_hdmirx/parameters/hdcp22_kill_esm";
    private static final String VIDEO_GLOBAL_OUTPUT_PATH = "/sys/class/video/video_global_output";

    private void setSuspendState(int state) {
        if (mSystemControlManager.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false) == false) {
            return;
        }

        if (state == POWER_SUSPEND_SHUTDOWN) {
            mSystemControlManager.writeSysFs(VIDEO_GLOBAL_OUTPUT_PATH, "0");
            mSystemControlManager.writeSysFs(KILL_ESM_PATH, "1");
        }

        if (state == POWER_SUSPEND_ON) {
            mSystemControlManager.setBootenv("ubootenv.var.suspend", "on");
        } else if (state == POWER_SUSPEND_OFF) {
            mSystemControlManager.setBootenv("ubootenv.var.suspend", "off");
        } else if (state == POWER_SUSPEND_SHUTDOWN) {
            mSystemControlManager.setBootenv("ubootenv.var.suspend", "shutdown");
        }

        Log.d(TAG, "setSuspendState: " + state);
    }
}
