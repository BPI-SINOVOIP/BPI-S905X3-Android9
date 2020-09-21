/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC Optimization
 */

package com.droidlogic;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.Service;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.SystemProperties;
import android.util.Log;

import java.util.ArrayList;
import java.lang.Runnable;
import java.lang.Thread;
import java.util.List;

import com.droidlogic.app.SystemControlManager;

public class Optimization extends Service {
    private static String TAG = "Optimization";
    private Context mContext;

    //static {
    //    System.loadLibrary("optimization");
    //}

    SystemControlManager mSCM;

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;

        mSCM = SystemControlManager.getInstance();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (SystemProperties.getBoolean("ro.vendor.app.optimization", true)) {
            new Thread(runnable).start();
        }
        else {
            Log.i(TAG, "Optimization service not start");
        }

        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private Runnable runnable = new Runnable() {
        public void run() {
            //int retProc = -1;
            //int retPkg = -1;
            //boolean isInProcOpt = false;
            //boolean isInPkgOpt = false;
            ActivityManager am = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);

            while (true) {
                try {
                    /*
                    if (!isInProcOpt) {
                        List< ActivityManager.RunningTaskInfo > task = am.getRunningTasks (1);
                        if (!task.isEmpty()) {
                            ComponentName cn = task.get (0).topActivity;
                            String pkg = cn.getPackageName();
                            String cls = cn.getClassName();

                            retPkg = nativeOptimization(pkg, cls);//bench match
                            if (retPkg == 0) {//0:PKG_BENCH, -4:PKG_SAME
                                isInPkgOpt = true;
                            }
                            else if (retPkg != -4) {
                                isInPkgOpt = false;
                            }
                        }
                    }

                    if (!isInPkgOpt) {
                        List< ActivityManager.RunningAppProcessInfo> apInfo = am.getRunningAppProcesses();
                        int len = apInfo.size();
                        //Log.i(TAG, "apInfo.size():" + len);
                        String [] proc = new String[len];
                        for (int i = 0; i < len; i++) {
                            //Log.i(TAG, "apInfo[" + i + "] processName:" + apInfo.get(i).processName);
                            proc[i] = apInfo.get(i).processName;
                        }
                        retProc = nativeOptimization(proc);
                        if (retProc == 0) {
                            isInProcOpt = true;
                        }
                        else if (retProc != -4) {
                            isInProcOpt = false;
                        }
                    }*/

                    String pkg = "";
                    String cls = "";
                    ArrayList<String> proc = new ArrayList<String>();
                    List< ActivityManager.RunningAppProcessInfo> apInfo = am.getRunningAppProcesses();
                    for (int i = 0; i < apInfo.size(); i++) {
                        proc.add(apInfo.get(i).processName);
                    }

                    List< ActivityManager.RunningTaskInfo > task = am.getRunningTasks (1);
                    if (!task.isEmpty()) {
                        ComponentName cn = task.get (0).topActivity;
                        pkg = cn.getPackageName();
                        cls = cn.getClassName();
                    }

                    mSCM.setAppInfo(pkg, cls, proc);

                    Thread.sleep(50);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    };

    //private native int nativeOptimization(String pkg, String cls);
    //private native int nativeOptimization(String[] proc);
}
