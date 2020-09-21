/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic.SubTitleService;

import android.app.Application;
import android.app.Service;
//import android.app.ActivityThread;
import android.content.Intent;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import java.lang.reflect.Method;
import java.lang.reflect.Constructor;


import android.content.Context;

public class SubTitleServer extends Service {
       private static final String TAG = "SubTitleServer";
       private int mServiceStartId = -1;

       public SubTitleServer() {
          Log.i(TAG,"[SubTitleServer]");
       }

       @Override
       public void onCreate() {
           Log.i(TAG,"[onCreate]");
           super.onCreate();
       }

       @Override
       public int onStartCommand(Intent intent, int flags, int startId) {
           Log.i(TAG,"[onStartCommand]");
           mServiceStartId = startId;
           return START_STICKY;
       }

       @Override
       public boolean onUnbind(Intent intent) {
           Log.i(TAG,"[onUnbind]");
           stopSelf(mServiceStartId);
           return true;
       }

       @Override
       public void onDestroy() {
           Log.i(TAG,"[onDestroy]");
           super.onDestroy();
       }

       @Override
       public IBinder onBind(Intent intent) {
             Log.i(TAG,"[onBind]");
             return new SubTitleService(this);
       }
}