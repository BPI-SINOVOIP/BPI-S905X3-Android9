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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Base class to help broadcast-based RPC.
 */
public abstract class BroadcastRpcBase<TRequest, TResponse> {
    private static final String TAG = "BroadcastRpc";

    private static final boolean VERBOSE = Log.isLoggable(TAG, Log.VERBOSE);

    static final String ACTION_REQUEST = "ACTION_REQUEST";
    static final String EXTRA_PAYLOAD = "EXTRA_PAYLOAD";
    static final String EXTRA_EXCEPTION = "EXTRA_EXCEPTION";

    static Handler sMainHandler = new Handler(Looper.getMainLooper());

    /** Implement in a subclass */
    protected abstract byte[] requestToBytes(TRequest request);

    /** Implement in a subclass */
    protected abstract TResponse bytesToResponse(byte[] bytes);

    public TResponse invoke(ComponentName targetReceiver, TRequest request) throws Exception {
        // Create a request intent.
        Log.i(TAG, "Sending to: " + targetReceiver + (VERBOSE ? "\nRequest: " + request : ""));

        final Intent requestIntent = new Intent(ACTION_REQUEST)
                .setComponent(targetReceiver)
                .addFlags(Intent.FLAG_RECEIVER_FOREGROUND)
                .putExtra(EXTRA_PAYLOAD, requestToBytes(request));

        // Send it.
        final CountDownLatch latch = new CountDownLatch(1);
        final AtomicReference<Bundle> responseBundle = new AtomicReference<>();

        InstrumentationRegistry.getContext().sendOrderedBroadcast(
                requestIntent, null, new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        responseBundle.set(getResultExtras(false));
                        latch.countDown();
                    }
                }, sMainHandler, 0, null, null);

        // Wait for a reply and check it.
        final boolean responseArrived = latch.await(60, TimeUnit.SECONDS);
        assertTrue("Didn't receive broadcast result.", responseArrived);

        // TODO If responseArrived is false, print if the package / component is installed?

        assertNotNull("Didn't receive result extras", responseBundle.get());

        final String exception = responseBundle.get().getString(EXTRA_EXCEPTION);
        if (exception != null) {
            fail("Target throw exception: receiver=" + targetReceiver
                    + "\nException: " + exception);
        }

        final byte[] resultPayload = responseBundle.get().getByteArray(EXTRA_PAYLOAD);
        assertNotNull("Didn't receive result payload", resultPayload);

        Log.i(TAG, "Response received: " + (VERBOSE ? resultPayload.toString() : ""));

        return bytesToResponse(resultPayload);
    }

    /**
     * Base class for a receiver for a broadcast-based RPC.
     */
    public abstract static class ReceiverBase<TRequest, TResponse> extends BroadcastReceiver {
        @Override
        public final void onReceive(Context context, Intent intent) {
            assertEquals(ACTION_REQUEST, intent.getAction());

            // Parse the request.
            final TRequest request = bytesToRequest(intent.getByteArrayExtra(EXTRA_PAYLOAD));

            Log.i(TAG, "Request received: " + (VERBOSE ? request.toString() : ""));

            Throwable exception = null;

            // Handle it and generate a response.
            TResponse response = null;
            try {
                response = handleRequest(context, request);
                Log.i(TAG, "Response generated: " + (VERBOSE ? response.toString() : ""));
            } catch (Throwable e) {
                exception = e;
                Log.e(TAG, "Exception thrown: " + e.getMessage(), e);
            }

            // Send back.
            final Bundle extras = new Bundle();
            if (response != null) {
                extras.putByteArray(EXTRA_PAYLOAD, responseToBytes(response));
            }
            if (exception != null) {
                extras.putString(EXTRA_EXCEPTION,
                        exception.toString() + "\n" + Log.getStackTraceString(exception));
            }
            setResultExtras(extras);
        }

        /** Implement in a subclass */
        protected abstract TResponse handleRequest(Context context, TRequest request)
                throws Exception;

        /** Implement in a subclass */
        protected abstract byte[] responseToBytes(TResponse response);

        /** Implement in a subclass */
        protected abstract TRequest bytesToRequest(byte[] bytes);
    }
}
