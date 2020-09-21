/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC NtpService
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
import android.net.ConnectivityManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.SystemClock;
import android.util.Log;

public class NtpService extends Service {
    private static final String TAG = "NtpService";
    private static final int NTP_TIMEOUT = 3000;
    private HandlerThread requestTimeThread;
    private Handler mHandler;
    private boolean register = false;
    private static String NtpServers[];

    private BroadcastReceiver mConnectivityReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if ( ConnectivityManager.CONNECTIVITY_ACTION.equals(action) && ( mHandler != null ) ) {
                mHandler.removeCallbacks(mSyncRunnable);
                mHandler.post(mSyncRunnable);
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        NtpServers = getResources().getStringArray(R.array.config_ntpServer_list);
        registerForConnectivityIntents();
    }

    @Override
    public void onDestroy() {
        cancelReceiver();
        super.onDestroy();
    }

    private void cancelReceiver(){
        if ( register ) {
            this.unregisterReceiver(mConnectivityReceiver);
            register = false;
        }
    }
    private void registerForConnectivityIntents() {
        if (!register) {
            IntentFilter intentFilter = new IntentFilter();
            intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
            registerReceiver(mConnectivityReceiver, intentFilter);
            register = true;
        }
    }

    private void init() {
        requestTimeThread = new HandlerThread("ntp_request");
        requestTimeThread.start();
        mHandler = new Handler(requestTimeThread.getLooper());
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if ( requestTimeThread == null ) {
            init();
        }
        return super.onStartCommand(intent, flags, startId);
    }

    private Runnable mSyncRunnable = new Runnable(){
        @Override
        public void run() {
            SyncTimeLock();
        }
    };

    private void SyncTimeLock() {
        try {
            Class<?> sntpClass = Class.forName("android.net.SntpClient");
            Object sntpObject = sntpClass.newInstance();
            Method getnptMethod = sntpClass.getMethod("getNtpTime", null);
            Method reqtimeMethod = sntpClass.getMethod("requestTime", String.class, int.class);
            Method getreference = sntpClass.getMethod("getNtpTimeReference", null);
            for ( int i=0; (NtpServers != null) && i<NtpServers.length; i++ ) {
                boolean ret = (boolean) reqtimeMethod.invoke(sntpObject, NtpServers[i], NTP_TIMEOUT);
                if (ret) {
                    long now = (long) getnptMethod.invoke(sntpObject, null) + SystemClock.elapsedRealtime() - (long) getreference.invoke(sntpObject, null);
                    Log.d(TAG,"TIME Set to"+now);
                    SystemClock.setCurrentTimeMillis(now);
                    cancelReceiver();
                    break;
                }
            }

        } catch(ClassNotFoundException e) {
            e.printStackTrace();
        }catch (InstantiationException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
}
