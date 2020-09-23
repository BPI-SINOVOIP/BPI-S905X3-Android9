/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

package com.droidlogic.pppoe;

import java.util.List;

import android.content.Context;
import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.net.wifi.IWifiManager;
import android.os.Handler;
import android.os.RemoteException;
import android.util.Slog;
import android.net.DhcpInfo;
import com.droidlogic.pppoe.PppoeDevInfo;
import com.droidlogic.pppoe.IPppoeManager;
import com.droidlogic.pppoe.PppoeService;

public class PppoeManager {
    public static final String TAG = "PppoeManager";
    public static final int PPPOE_DEVICE_SCAN_RESULT_READY = 0;
    public static final String PPPOE_STATE_CHANGED_ACTION =
            "com.droidlogic.pppoe.PPPOE_STATE_CHANGED";
    public static final String NETWORK_STATE_CHANGED_ACTION =
            "com.droidlogic.pppoe.STATE_CHANGE";

//  public static final String ACTION_PPPOE_NETWORK = "android.net.pppoe.PPPOE_NET_CHG";


    public static final String EXTRA_NETWORK_INFO = "networkInfo";
    public static final String EXTRA_PPPOE_STATE = "pppoe_state";
    public static final String EXTRA_PPPOE_ERRCODE = "pppoe_errcode";
    public static final String PROP_VAL_PPP_NOERR = "0:0";
    public static final String EXTRA_PREVIOUS_PPPOE_STATE = "previous_pppoe_state";

    public static final int PPPOE_STATE_UNKNOWN = 0;
    public static final int PPPOE_STATE_DISABLED = 1;
    public static final int PPPOE_STATE_ENABLED = 2;

    IPppoeManager mService;
    private final Context mContext;


    public PppoeManager(IPppoeManager service, Context context) {
        Slog.i(TAG, "Init Pppoe Manager");
        mService = service;
        mContext = context;

    }

    public boolean isPppoeConfigured() {
        try {
            return mService.isPppoeConfigured();
        } catch (RemoteException e) {
            Slog.i(TAG, "Can not check pppoe config state");
        }
        return false;
    }

    public PppoeDevInfo getSavedPppoeConfig() {
        try {
            return mService.getSavedPppoeConfig();
        } catch (RemoteException e) {
            Slog.i(TAG, "Can not get pppoe config");
        }
        return null;
    }

    public void UpdatePppoeDevInfo(PppoeDevInfo info) {
        try {
            mService.UpdatePppoeDevInfo(info);
        } catch (RemoteException e) {
            Slog.i(TAG, "Can not update pppoe device info");
        }
    }

    public String[] getDeviceNameList() {
        try {
            return mService.getDeviceNameList();
        } catch (RemoteException e) {
            return null;
        }
    }

    public void setPppoeEnabled(boolean enable) {
        try {
            mService.setPppoeState(enable ? PPPOE_STATE_ENABLED:PPPOE_STATE_DISABLED);
        } catch (RemoteException e) {
            Slog.i(TAG,"Can not set new state");
        }
    }

    public int getPppoeState( ) {
        try {
            return mService.getPppoeState();
        } catch (RemoteException e) {
            return 0;
        }
    }

    public boolean pppoeConfigured() {
        try {
            return mService.isPppoeConfigured();
        } catch (RemoteException e) {
            return false;
        }
    }

    public DhcpInfo getDhcpInfo() {
        try {
            return mService.getDhcpInfo();
        } catch (RemoteException e) {
            return null;
        }
    }

    public int getTotalInterface() {
        try {
            return mService.getTotalInterface();
        } catch (RemoteException e) {
            return 0;
        }
    }

    public void pppoeSetDefaultConf() {
        try {
            mService.setPppoeMode(PppoeDevInfo.PPPOE_CONN_MODE_DHCP);
        } catch (RemoteException e) {
        }
    }

    public boolean isPppoeDeviceUp() {
        try {
            return mService.isPppoeDeviceUp();
        } catch (RemoteException e) {
            return false;
        }
    }
}
