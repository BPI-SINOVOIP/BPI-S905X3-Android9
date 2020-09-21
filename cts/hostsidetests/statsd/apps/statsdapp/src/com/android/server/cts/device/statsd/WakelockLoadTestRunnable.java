/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.server.cts.device.statsd;

import android.content.Context;
import android.os.PowerManager;
import android.support.test.InstrumentationRegistry;
import java.util.concurrent.CountDownLatch;


public class WakelockLoadTestRunnable implements Runnable {
    String tag;
    CountDownLatch latch;
    WakelockLoadTestRunnable(String t, CountDownLatch l) {
        tag = t;
        latch = l;
    }
    @Override
    public void run() {
        Context context = InstrumentationRegistry.getContext();
        PowerManager pm = context.getSystemService(PowerManager.class);
        PowerManager.WakeLock wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, tag);
        long sleepTimeNs = 700_000;

        for (int i = 0; i < 1000; i++) {
            wl.acquire();
            long startTime = System.nanoTime();
            while (System.nanoTime() - startTime < sleepTimeNs) {}
            wl.release();
            startTime = System.nanoTime();
            while (System.nanoTime() - startTime < sleepTimeNs) {}
        }
        latch.countDown();
    }

}
