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
package com.android.compatibility.common.util;

import static junit.framework.Assert.fail;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.net.Uri;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;

/**
 * CallbackAsserter helps wait until a callback is called.
 */
public class CallbackAsserter {
    private static final String TAG = "CallbackAsserter";

    final CountDownLatch mLatch = new CountDownLatch(1);

    CallbackAsserter() {
    }

    /**
     * Call this to assert a callback be called within the given timeout.
     */
    public final void assertCalled(String message, int timeoutSeconds) throws Exception {
        try {
            if (mLatch.await(timeoutSeconds, TimeUnit.SECONDS)) {
                return;
            }
            fail("Didn't receive callback: " + message);
        } finally {
            cleanUp();
        }
    }

    void cleanUp() {
    }

    /**
     * Create an instance for a broadcast.
     */
    public static CallbackAsserter forBroadcast(IntentFilter filter) {
        return forBroadcast(filter, null);
    }

    /**
     * Create an instance for a broadcast.
     */
    public static CallbackAsserter forBroadcast(IntentFilter filter, Predicate<Intent> checker) {
        return new BroadcastAsserter(filter, checker);
    }

    /**
     * Create an instance for a content changed notification.
     */
    public static CallbackAsserter forContentUri(Uri watchUri) {
        return forContentUri(watchUri, null);
    }

    /**
     * Create an instance for a content changed notification.
     */
    public static CallbackAsserter forContentUri(Uri watchUri, Predicate<Uri> checker) {
        return new ContentObserverAsserter(watchUri, checker);
    }

    private static class BroadcastAsserter extends CallbackAsserter {
        private final BroadcastReceiver mReceiver;

        BroadcastAsserter(IntentFilter filter, Predicate<Intent> checker) {
            mReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if (checker != null && !checker.test(intent)) {
                        Log.v(TAG, "Ignoring intent: " + intent);
                        return;
                    }
                    mLatch.countDown();
                }
            };
            InstrumentationRegistry.getContext().registerReceiver(mReceiver, filter);
        }

        @Override
        void cleanUp() {
            InstrumentationRegistry.getContext().unregisterReceiver(mReceiver);
        }
    }

    private static class ContentObserverAsserter extends CallbackAsserter {
        private final ContentObserver mObserver;

        ContentObserverAsserter(Uri watchUri, Predicate<Uri> checker) {
            mObserver = new ContentObserver(null) {
                @Override
                public void onChange(boolean selfChange, Uri uri) {
                    if (checker != null && !checker.test(uri)) {
                        Log.v(TAG, "Ignoring notification on URI: " + uri);
                        return;
                    }
                    mLatch.countDown();
                }
            };
            InstrumentationRegistry.getContext().getContentResolver().registerContentObserver(
                    watchUri, true, mObserver);
        }

        @Override
        void cleanUp() {
            InstrumentationRegistry.getContext().getContentResolver().unregisterContentObserver(
                    mObserver);
        }
    }
}
