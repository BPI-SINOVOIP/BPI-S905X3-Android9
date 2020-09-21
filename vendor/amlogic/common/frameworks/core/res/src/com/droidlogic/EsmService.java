/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC EsmService
 */

package com.droidlogic;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.PowerManager;
import android.util.Log;
import android.os.SystemProperties;

//this service used to kill esm when sleep
public class EsmService extends Service {
    private static final String TAG = "EsmService";
    private HandlerThread mKillThread;
    private Handler mHandler;
    private boolean register = false;

    private PowerManager.WakeLock mWakeLock;

    private BroadcastReceiver mScreenReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_SCREEN_ON.equals(action)) {
                //writeSysfsStr(KILL_ESM_PATH, "0");
                //Log.d(TAG, "when resume set N to Kill_ESM_PATH");
            }
            if ( Intent.ACTION_SCREEN_OFF.equals(action) && ( mHandler != null ) ) {
                //mHandler.removeCallbacks(mKillRunnable);
                //mHandler.post(mKillRunnable);
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();

        Log.i(TAG, "onCreate");

        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
        if (!register) {
            IntentFilter intentFilter = new IntentFilter();
            intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
            intentFilter.addAction(Intent.ACTION_SCREEN_ON);
            registerReceiver(mScreenReceiver, intentFilter);
            register = true;
        }
    }

    @Override
    public void onDestroy() {
        if ( register ) {
            this.unregisterReceiver(mScreenReceiver);
            register = false;
        }

        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if ( mKillThread == null ) {
            mKillThread = new HandlerThread("kill_esm");
            mKillThread.start();
            mHandler = new Handler(mKillThread.getLooper());
        }
        return super.onStartCommand(intent, flags, startId);
    }


    private static final String KILL_ESM_PATH = "/sys/module/tvin_hdmirx/parameters/hdcp22_kill_esm";
    private int writeSysfsStr(String path, String val) {
        if (!new java.io.File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return 1;
        }

        try {
            java.io.BufferedWriter writer = new java.io.BufferedWriter(new java.io.FileWriter(path), 64);
            try {
                writer.write(val);
            } finally {
                writer.close();
            }
            return 0;

        } catch (java.io.IOException e) {
            Log.e(TAG, "IO Exception when write: " + path, e);
            return 1;
        }
    }

    private String readSysfsStr(String path) {
        String val = null;

        if (!new java.io.File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return null;
        }

        try {
            java.io.BufferedReader reader = new java.io.BufferedReader(new java.io.FileReader(path), 64);
            try {
                val = reader.readLine();
            } finally {
                reader.close();
            }
            return val;

        } catch (java.io.IOException e) {
            Log.e(TAG, "IO Exception when write: " + path, e);
            return null;
        }
    }

    public void forceExitEsm() {
        //String killEsm = SystemProperties.get("sys.kill.esm", "true");
        //if(!killEsm.equals("true")) {
        //    return;
        //}

        if (!new java.io.File("/mnt/vendor/param/firmware.le").exists()) {
            Log.w(TAG, "forceExitEsm do not have hdcp rx22 key");
            return;
        }

        String powerState = readSysfsStr("/sys/power/state");

        Log.d(TAG, "forceExitEsm /sys/power/state:" + powerState);
        writeSysfsStr(KILL_ESM_PATH, "1");
        int count =0;
        while (true) {
            try {
                Thread.sleep(500);//500ms
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }

            String esmValue = readSysfsStr(KILL_ESM_PATH);
            String state = SystemProperties.get("init.svc.hdcp_rx22");
            Log.w(TAG, "hdcp22_kill_esm value:" + esmValue +"rx22 is:"+state);
            if (("Y".equals(esmValue)) && ("running".equals(state)) && (count < 10))
                Log.w(TAG, "esm still is running, we need it be killed");
            else break;
        }
        Log.d(TAG, "forceExitEsm esm killed done");
    }

    private Runnable mKillRunnable = new Runnable(){
        @Override
        public void run() {
            mWakeLock.acquire();
            forceExitEsm();
            mWakeLock.release();
        }
    };
}
