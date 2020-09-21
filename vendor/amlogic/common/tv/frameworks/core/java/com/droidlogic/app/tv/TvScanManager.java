/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;

import com.droidlogic.tvinput.services.ITvScanService;
import com.droidlogic.tvinput.services.IUpdateUiCallbackListener;
import com.droidlogic.tvinput.services.TvMessage;

/**
 * Created by yu.fang on 2018/7/9.
 */

public class TvScanManager {
    private String TAG = "TvScanManager";
    private boolean mDebug = true;
    private boolean isConnected = false;

    private int RETRY_MAX = 10;

    private ITvScanService mService = null;
    private ScannerMessageListener mMessageListener = null;
    private Context mContext;
    private Intent intent;

    private RemoteCallbackList<IUpdateUiCallbackListener> mListenerList = new RemoteCallbackList<>();

    public TvScanManager(Context context, Intent intent) {
        mContext = context;
        this.intent = intent;
        getService();
    }

    private void LOGI(String msg) {
        if (mDebug) Log.i(TAG, msg);
    }

    private IUpdateUiCallbackListener.Stub mListener = new IUpdateUiCallbackListener.Stub() {
        @Override
        public void onRespond(TvMessage msg) throws RemoteException {
            Log.d(TAG, "=====receive message from TvScanService");
            if (mMessageListener != null)
                mMessageListener.onMessage(msg);
        }
    };

    private void getService() {
        LOGI("=====[getService]");
        int retry = RETRY_MAX;
        boolean mIsBind = false;
        try {
            synchronized (this) {
                while (true) {
                    Intent intent = new Intent();
                    intent.setAction("com.droidlogic.tvinput.services.TvScanService");
                    intent.setPackage("com.droidlogic.tvinput");
                    mIsBind = mContext.bindService(intent, serConn, mContext.BIND_AUTO_CREATE);
                    LOGI("=====[getService] mIsBind: " + mIsBind + ", retry:" + retry);
                    if (mIsBind || retry <= 0) {
                        break;
                    }
                    retry --;
                    Thread.sleep(500);
                }
            }
        } catch (InterruptedException e){}
    }

    private ServiceConnection serConn = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            LOGI("[onServiceDisconnected]mService: " + mService);
            try{
                //unregister callback
                mService.unregisterListener(mListener);
                isConnected = false;
            } catch (RemoteException e){
                e.printStackTrace();
            }
            mService = null;
        }
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mService = ITvScanService.Stub.asInterface(service);
            try{
                //register callback
                mService.registerListener(mListener);
                isConnected = true;
            } catch (RemoteException e){
                e.printStackTrace();
            }
            LOGI("SubTitleClient.onServiceConnected()..mService: " + mService);
        }
    };

    public void unBindService() {
        mContext.unbindService(serConn);
    }

    public boolean isConnected() {
        return isConnected;
    }

    public void init(){
        try {
            if (intent != null) {
                mService.init(intent);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void setAtsccSearchSys(int value){
        try {
            mService.setAtsccSearchSys(value);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void startAutoScan(){
        try {
            mService.startAutoScan();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void startManualScan(){
        try {
            mService.startManualScan();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void setSearchSys (boolean value1, boolean value2){
        try {
            mService.setSearchSys(value1, value2);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void setFrequency (String value1, String value2) {
        try {
            mService.setFrequency(value1, value2);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void release() {
        try {
            mService.unregisterListener(mListener);
            mService.release();
            mService = null;
            mMessageListener = null;
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void registerListener(IUpdateUiCallbackListener listener) throws RemoteException {
        mListenerList.register(listener);
    }

    public void unregisterListener(IUpdateUiCallbackListener listener) throws RemoteException {
        mListenerList.unregister(listener);
    }

    public void setMessageListener(ScannerMessageListener l) {
        mMessageListener = l;
    }

    public interface ScannerMessageListener {
        void onMessage(TvMessage msg);
    }
}
