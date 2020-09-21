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
 * limitations under the License
 */

package com.android.service.ims.presence;

import android.app.Application;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.mockito.Mockito.when;

/**
 * Base Class for PresencePolling Unit Tests
 */

public class PresencePollingTestBase {

    protected @Mock Context mTestContext;
    protected @Mock Application mApp;
    private ApplicationInfo mAppInfo = new ApplicationInfo();

    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        when(mTestContext.getApplicationInfo()).thenReturn(mAppInfo);
        mAppInfo.targetSdkVersion = Build.VERSION_CODES.CUR_DEVELOPMENT;
        // Set up the looper if it does not exist on the test thread.
        if (Looper.myLooper() == null) {
            Looper.prepare();
        }
    }

    public void tearDown() throws Exception {
    }

    protected final void waitForHandlerAction(Handler h, long timeoutMillis) {
        waitForHandlerActionDelayed(h, timeoutMillis, 0 /*delayMs*/);
    }

    protected final void waitForHandlerActionDelayed(Handler h, long timeoutMillis, long delayMs) {
        final CountDownLatch lock = new CountDownLatch(1);
        h.postDelayed(lock::countDown, delayMs);
        while (lock.getCount() > 0) {
            try {
                lock.await(timeoutMillis, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                // do nothing
            }
        }
    }
}
