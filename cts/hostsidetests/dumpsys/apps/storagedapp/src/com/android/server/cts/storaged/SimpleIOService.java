/*
 * Copyright (C) 2017 The Android Open Source Project
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
 * limitations under the License.
 */

package com.android.server.cts.storaged;

import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

public class SimpleIOService extends Service {
    private Looper mServiceLooper;
    private ServiceHandler mServiceHandler;

    private final class ServiceHandler extends Handler {
        private String getCgroupCpuset() {
            File cgroup = new File("/proc/self/cgroup");
            try (BufferedReader br = new BufferedReader(new FileReader(cgroup))) {
                String line;
                while ((line = br.readLine()) != null) {
                    String[] parts = line.split(":");
                    if (parts.length >= 3 && parts[1].equals("cpuset")) {
                        return parts[2];
                    }
                }
            } catch (IOException e) {
            }
            return null;
        }

        public ServiceHandler(Looper looper) {
            super(looper);

            // Spin for 20 seconds until we are not in the foreground. (Note
            // that UsageStatsService considers "foreground" to be in the
            // background - only top and persistent processes are reported as
            // foreground for uid_io).
            for (int i = 1; i <= 200; i++) {
                String cpuset = getCgroupCpuset();
                if (cpuset == null || !cpuset.equals("/top")) {
                    break;
                }
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                }
            }

            Log.i("SimpleIOService", "cgroup:" + getCgroupCpuset());
        }

        @Override
        public void handleMessage(Message msg) {
            File testFile = new File(getFilesDir(), "StoragedTest_Temp_BG");
            try {
                char data[] = new char[4096];
                FileWriter w = new FileWriter(testFile);
                w.write(data);
                w.flush();
                w.close();
            } catch (IOException e) {
                throw new RuntimeException(e);
            } finally {
                // Stop the service using the startId, so that we don't stop
                // the service in the middle of handling another job
                stopSelf(msg.arg1);
            }
        }
    }

    @Override
    public void onCreate() {
        // Start up the thread running the service.  Note that we create a
        // separate thread because the service normally runs in the process's
        // main thread, which we don't want to block.  We also make it
        // background priority so CPU-intensive work will not disrupt our UI.
        HandlerThread thread = new HandlerThread("ServiceStartArguments",
                Process.THREAD_PRIORITY_BACKGROUND);
        thread.start();

        // Get the HandlerThread's Looper and use it for our Handler
        mServiceLooper = thread.getLooper();
        mServiceHandler = new ServiceHandler(mServiceLooper);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = startId;
        mServiceHandler.sendMessage(msg);

        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
