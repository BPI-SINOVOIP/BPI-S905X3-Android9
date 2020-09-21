/******************************************************************
*
*Copyright (C) 2012  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/

package com.droidlogic.PPPoE;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;
import android.os.INetworkManagementService;
import android.os.ServiceManager;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.net.wifi.WifiManager;
import com.amlogic.pppoe.PppoeOperation;
import com.android.server.net.BaseNetworkObserver;
import com.droidlogic.app.SystemControlManager;

public class MyPppoeService extends Service
{
    private static final String TAG = "MyPppoeService";
    private static final String eth_device_sysfs = "/sys/class/ethernet/linkspeed";
    private NotificationManager mNM;
    private WifiManager mWifiManager;
    private Handler mPppoeHandler;
    private PppoeOperation operation = null;
    private InterfaceObserver mInterfaceObserver;
    private INetworkManagementService mNMService;
    private boolean mScreenon;
    private boolean mConnected;
    private int mTimes;
    public static final int MSG_PPPOE_START = 0xabcd0080;
    private static final int PPPOE_DELAYD = 10000;
    private Context mContext;
    private SystemControlManager mSystemControlManager;
    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");
        IBinder b = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
        mNMService = INetworkManagementService.Stub.asInterface(b);
        mScreenon=false;
        mContext=this;
        mNM = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mSystemControlManager = SystemControlManager.getInstance();
        operation = new PppoeOperation();
        mPppoeHandler = new PppoeHandler();
        mInterfaceObserver = new InterfaceObserver();
        try {
            mNMService.registerObserver(mInterfaceObserver);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not register InterfaceObserver " + e);
        }
        IntentFilter f = new IntentFilter();

        f.addAction(Intent.ACTION_SHUTDOWN);
        f.addAction(Intent.ACTION_SCREEN_OFF);
        f.addAction(Intent.ACTION_SCREEN_ON);
        registerReceiver(mShutdownReceiver, new IntentFilter(f));
    }

    @Override
    public void onDestroy() {
        //unregisteReceiver
        unregisterReceiver(mShutdownReceiver);
        // Cancel the persistent notification.
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
    private void updateInterfaceState(String iface, boolean up) {
        Intent intent = new Intent("com.droidlogic.linkchange");

        if (!iface.contains("eth0")) {
            return;
        }
        if (up) {
            this.sendBroadcast(intent);
        }
    }
    private boolean getAutoDialFlag(Context context) {
        SharedPreferences sharedata = context.getSharedPreferences("inputdata", 0);
        if (sharedata != null && sharedata.getAll().size() > 0)
        {
            return sharedata.getBoolean(PppoeConfigDialog.INFO_AUTO_DIAL_FLAG, false);
        }
        return false;
    }

    private boolean isEthDeviceAdded(Context context) {
        String str = mSystemControlManager.readSysFs(eth_device_sysfs);
        if (str == null)
            return false ;
        if (str.contains("unlink")) {
            return false;
        }else{
            return true;
        }
    }

    private class InterfaceObserver extends BaseNetworkObserver {
        @Override
        public void interfaceLinkStateChanged(String iface, boolean up) {
            Log.d(TAG , "interfaceLinkStateChanged+"+iface+"up"+up);
            if (iface.contains("eth0") && up) {
                if (!mScreenon) {
                    if (getAutoDialFlag(mContext)) {
                         mPppoeHandler.sendEmptyMessageDelayed(MSG_PPPOE_START, PPPOE_DELAYD);
                    }
                }
            } else if (iface.contains("ppp0")) {
                mConnected = up;
            }
        }
    }
    private class PppoeHandler extends Handler
    {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case MSG_PPPOE_START:
                    if (!mConnected) {
                        int wifiState = mWifiManager.getWifiState();
                        if ((wifiState == WifiManager.WIFI_STATE_ENABLING) ||
                            (wifiState == WifiManager.WIFI_STATE_ENABLED)) {
                            mWifiManager.setWifiEnabled(false);
                        } else {
                            if (isEthDeviceAdded(mContext))
                                updateInterfaceState("eth0", true);
                        }
                        if (mTimes < 3) {
                            mPppoeHandler.sendEmptyMessageDelayed(MSG_PPPOE_START, PPPOE_DELAYD+mTimes*1000);
                            mTimes++;
                        } else {
                            mScreenon = false;
                            mTimes = 0;
                        }
                    } else {
                        mScreenon = false;
                        mTimes = 0;
                    }
                    break;
                default:
                    Log.d(TAG, "handleMessage: " + msg.what);
                    break;
            }
        }
    }
    private BroadcastReceiver mShutdownReceiver =
        new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG , "onReceive :" +intent.getAction());
            mScreenon = false;
            if (getAutoDialFlag(mContext)) {
                if ((Intent.ACTION_SCREEN_OFF).equals(intent.getAction())) {
                    mScreenon = true;
                    operation.disconnect();
                }
                if ((Intent.ACTION_SCREEN_ON).equals(intent.getAction())) {
                    mScreenon = true;
                    mConnected = false;
                    mTimes=0;
                    mPppoeHandler.sendEmptyMessageDelayed(MSG_PPPOE_START, PPPOE_DELAYD);
                }
            }
        }
    };
}

