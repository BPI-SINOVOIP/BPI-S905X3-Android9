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

package android.server.wm;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 *  A service collecting data from other apps used by a test.
 *
 *  Use {@link TestLogClient} to send data to this service.
 */
public class TestLogService extends Service {
    private static final String TAG = "TestLogService";

    private static final Object mLock = new Object();

    static class ClientChannel {
        final String mStopKey;
        final CountDownLatch mLatch = new CountDownLatch(1);
        final Map<String, String> mResults = new HashMap<>();

        ClientChannel(String stopKey) {
            mStopKey = stopKey;
        }
    }

    private static Map<String, ClientChannel> mChannels = new HashMap<>();

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        record(intent.getStringExtra(TestLogClient.EXTRA_LOG_TAG),
                intent.getStringExtra(TestLogClient.EXTRA_KEY),
                intent.getStringExtra(TestLogClient.EXTRA_VALUE));
        return START_NOT_STICKY;
    }

    /**
     * Prepare to receive results from a client with a specified tag.
     *
     * @param logtag Unique tag for the client.
     * @param stopKey The key that signals that the client has completed all required actions.
     */
    public static void registerClient(String logtag, String stopKey) {
        synchronized (mLock) {
            if (mChannels.containsKey(logtag)) {
                throw new IllegalArgumentException(logtag);
            }
            mChannels.put(logtag, new ClientChannel(stopKey));
        }
    }

    /**
     * Wait for the client to complete all required actions and return the results.
     *
     * @param logtag Unique tag for the client.
     * @param timeoutMs Latch timeout in ms.
     * @return The map of results from the client
     */
    public static Map<String, String> getResultsForClient(String logtag, int timeoutMs) {
        Map<String, String> result = new HashMap<>();
        CountDownLatch latch;
        synchronized (mLock) {
            if (!mChannels.containsKey(logtag)) {
                return result;
            }
            latch = mChannels.get(logtag).mLatch;
        }
        try {
            latch.await(timeoutMs, TimeUnit.MILLISECONDS);
        } catch (InterruptedException ignore) {
        }
        synchronized (mLock) {
            for (Map.Entry<String, String> e : mChannels.get(logtag).mResults.entrySet()) {
                result.put(e.getKey(), e.getValue());
            }
        }
        return result;
    }

    private static void record(String logtag, String key, String value) {
        synchronized (mLock) {
            if (!mChannels.containsKey(logtag)) {
                Log.e(TAG, "Unexpected logtag: " + logtag);
                return;
            }
            ClientChannel channel = mChannels.get(logtag);
            channel.mResults.put(key, value);
            if (key.equals(channel.mStopKey)) {
                channel.mLatch.countDown();
            }
        }
    }
}
