/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.droidlogic.tv.settings;


import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.droidlogic.app.IDolbyAudioEffectService;
import com.droidlogic.app.OutputModeManager;

public class DolbyAudioEffectManager {
    private static final String TAG = "DolbyAudioEffectManager";
    private int RETRY_MAX = 10;

    private IDolbyAudioEffectService mService = null;
    private static DolbyAudioEffectManager mInstance;
    private Context mContext;

    public static DolbyAudioEffectManager getInstance(Context context) {
        if (null == mInstance) {
            OutputModeManager omm = new OutputModeManager(context);
            if (omm.isDapValid())
                mInstance = new DolbyAudioEffectManager(context);
        }
        return mInstance;
    }

    public DolbyAudioEffectManager(Context context) {
        mContext = context;
        getService();
    }
    private void getService() {
        int retry = RETRY_MAX;
        boolean isBind = false;
        try {
            synchronized (this) {
                while (true) {
                    Intent intent = new Intent();
                    intent.setAction("com.droidlogic.DolbyAudioEffectService");
                    intent.setPackage("com.droidlogic");
                    isBind = mContext.bindService(intent, mServiceConn, mContext.BIND_AUTO_CREATE);
                    Log.d(TAG, "getService: mIsBind: " + isBind + ", retry:" + retry);
                    if (isBind || retry <= 0) {
                        break;
                    }
                    retry --;
                    Thread.sleep(500);
                }
            }
        } catch (InterruptedException e){}
    }
    private ServiceConnection mServiceConn = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.d(TAG, "[onServiceDisconnected] mService: " + mService);
            mService = null;
        }
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mService = IDolbyAudioEffectService.Stub.asInterface(service);
            Log.d(TAG, "[onServiceConnected] mService: " + mService);
        }
    };

    public void unBindService() {
        mContext.unbindService(mServiceConn);
    }

    public void setDapParam(int id, int value) {
        try {
            mService.setDapParam(id, value);
        } catch (RemoteException e) {
            Log.e(TAG, "setDapParam failed:" + e);
        }
    }

    public int getDapParam(int id) {
        try {
            return mService.getDapParam(id);
        } catch (RemoteException e) {
            Log.e(TAG, "getDapParam failed:" + e);
        }
        return 0;
    }

    public void initDapAudioEffect() {
        try {
            mService.initDapAudioEffect();
        } catch (RemoteException e) {
            Log.e(TAG, "initDapAudioEffect failed:" + e);
        }
    }
}

