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

import java.util.Timer;
import java.util.TimerTask;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.ConnectivityManager;
import android.net.NetworkInfo.State;
import android.net.wifi.WifiManager;
import android.util.Log;
import android.content.SharedPreferences;
import com.amlogic.pppoe.PppoeOperation;
import com.droidlogic.pppoe.PppoeStateTracker;
import com.droidlogic.pppoe.PppoeService;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.SystemProperties;
import android.content.ServiceConnection;
import com.droidlogic.pppoe.IPppoeManager;
import android.content.ComponentName;
import com.droidlogic.app.SystemControlManager;

public class PppoeBroadcastReceiver extends BroadcastReceiver {
    private static final String TAG = "PppoeBroadcastReceiver";
    private static final String ACTION_BOOT_COMPLETED =
        "android.intent.action.BOOT_COMPLETED";
    public static final String ETH_STATE_CHANGED_ACTION =
        "android.net.ethernet.ETH_STATE_CHANGED";
    public static final int DELAY_TIME = 2000;
    private Handler mHandler = null;
    private boolean mAutoDialFlag = false;
    private String mInterfaceSelected = null;
    private String mUserName = null;
    private String mPassword = null;
    private PppoeOperation operation = null;
    private static boolean mFirstAutoDialDone = false;
    private Timer mMandatoryDialTimer = null;
    public static final int TYPE_PPPOE = 8;
    private static final String PPPOE_SERVICE = "pppoe";
    private int RETRY_MAX = 10;
    private Context mContext;
    private Intent intent = null;
    IPppoeManager mService = null;
    private SystemControlManager mSystemControlManager;

    void StartPppoeService(Context context)
    {
        boolean needPppoe = true;
        if (needPppoe) {
            final PppoeStateTracker pppoetracker;
            HandlerThread handlerThread = new HandlerThread("myHandlerThread");
            handlerThread.start();
            Handler handler = new Handler(handlerThread.getLooper());
            try {
                pppoetracker = new PppoeStateTracker(handlerThread.getLooper(),TYPE_PPPOE, PPPOE_SERVICE);
                PppoeService pppoe = new PppoeService(context,pppoetracker);
                Log.e(TAG, "start add service pppoe");
                try {
                    Class.forName("android.os.ServiceManager").getMethod("addService", new Class[] {String.class, IBinder.class })
                    .invoke(null, new Object[] { PPPOE_SERVICE, pppoe });
                }catch (Exception ex) {
                    Log.e(TAG, "addService " + PPPOE_SERVICE + " fail:" + ex);
                }
                Log.d(TAG, "end add service pppoe");
                pppoetracker.startMonitoring(context,handler);
                //  if (config.isDefault()) {
                    pppoetracker.reconnect();
                //  }
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "Problem creating TYPE_PPPOE tracker: " + e);
            }
        }
    }

    private String getNetworkInterfaceSelected(Context context)
    {
        SharedPreferences sharedata = context.getSharedPreferences("inputdata", 0);
        if (sharedata != null && sharedata.getAll().size() > 0)
        {
            return sharedata.getString(PppoeConfigDialog.INFO_NETWORK_INTERFACE_SELECTED, null);
        }
        return null;
    }


    private boolean getAutoDialFlag(Context context)
    {
        SharedPreferences sharedata = context.getSharedPreferences("inputdata", 0);
        if (sharedata != null && sharedata.getAll().size() > 0)
        {
            return sharedata.getBoolean(PppoeConfigDialog.INFO_AUTO_DIAL_FLAG, false);
        }
        return false;
    }

    private String getUserName(Context context)
    {
        SharedPreferences sharedata = context.getSharedPreferences("inputdata", 0);
        if (sharedata != null && sharedata.getAll().size() > 0)
        {
            return sharedata.getString(PppoeConfigDialog.INFO_USERNAME, null);
        }
        return null;
    }

    private String getPassword(Context context)
    {
        SharedPreferences sharedata = context.getSharedPreferences("inputdata", 0);
        if (sharedata != null && sharedata.getAll().size() > 0)
        {
            return sharedata.getString(PppoeConfigDialog.INFO_PASSWORD, null);
        }
        return null;
    }


    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        mInterfaceSelected = getNetworkInterfaceSelected(context);
        mAutoDialFlag = getAutoDialFlag(context);

        mUserName = getUserName(context);
        mPassword = getPassword(context);
        mSystemControlManager = SystemControlManager.getInstance();

        if (ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
            StartPppoeService(context);
            context.startService(new Intent(context,
                 MyPppoeService.class));
            mFirstAutoDialDone = true;
        }

        if (null == mInterfaceSelected
            || !mAutoDialFlag
            || null == mUserName
            || null == mPassword) {
            mFirstAutoDialDone = false;
            return;
        }

        if (mHandler == null) {
            mHandler = new PppoeHandler();
        }
        if (operation == null) {
            operation = new PppoeOperation();
        }
        Log.d(TAG , "onReceive :" +intent.getAction());
        if ("com.droidlogic.linkchange".equals(action) || mFirstAutoDialDone) {
            if (mFirstAutoDialDone) {
                mHandler.sendEmptyMessageDelayed(PPPoEActivity.MSG_MANDATORY_DIAL, DELAY_TIME);
                mFirstAutoDialDone = false;
            } else {
                if (!mInterfaceSelected.startsWith("eth") )
                    return;
                //Timeout after 5 seconds
                mHandler.sendEmptyMessageDelayed(PPPoEActivity.MSG_START_DIAL, DELAY_TIME);
            }
        }
    }


    void setPppoeRunningFlag()
    {
        mSystemControlManager.setProperty(PppoeConfigDialog.ETHERNETDHCP, "disabled");

        mSystemControlManager.setProperty(PppoeConfigDialog.PPPOERUNNING, "100");
        String propVal = mSystemControlManager.getProperty(PppoeConfigDialog.PPPOERUNNING);
        int n = 0;
        if (propVal.length() != 0) {
            try {
                n = Integer.parseInt(propVal);
                Log.d(TAG, "setPppoeRunningFlag as " + n);
            } catch (NumberFormatException e) {}
        } else {
            Log.d(TAG, "failed to setPppoeRunningFlag");
        }

        return;
    }

    private class PppoeHandler extends Handler
    {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);

            switch (msg.what) {
                case PPPoEActivity.MSG_MANDATORY_DIAL:
                    Log.d(TAG, "handleMessage: MSG_MANDATORY_DIAL");
                    setPppoeRunningFlag();
                    operation.terminate();
                    operation.disconnect();
                    mHandler.sendEmptyMessageDelayed(PPPoEActivity.MSG_START_DIAL, DELAY_TIME);
                break;

                case PPPoEActivity.MSG_START_DIAL:
                    Log.d(TAG, "handleMessage: MSG_START_DIAL");
                    setPppoeRunningFlag();
                    operation.connect(mInterfaceSelected, mUserName, mPassword);
                break;

                default:
                    Log.d(TAG, "handleMessage: " + msg.what);
                break;
            }
        }
    }
}

