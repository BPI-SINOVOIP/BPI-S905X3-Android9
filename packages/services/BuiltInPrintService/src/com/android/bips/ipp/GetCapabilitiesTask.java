/*
 * Copyright (C) 2016 The Android Open Source Project
 * Copyright (C) 2016 Mopria Alliance, Inc.
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

package com.android.bips.ipp;

import android.net.Uri;
import android.os.AsyncTask;
import android.util.Log;

import com.android.bips.jni.BackendConstants;
import com.android.bips.jni.LocalPrinterCapabilities;
import com.android.bips.util.PriorityLock;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

/** A background task that queries a specific URI for its complete capabilities */
public class GetCapabilitiesTask extends AsyncTask<Void, Void, LocalPrinterCapabilities> {
    private static final String TAG = GetCapabilitiesTask.class.getSimpleName();
    private static final boolean DEBUG = false;

    /** Lock to ensure we don't issue multiple simultaneous capability requests */
    private static final PriorityLock sLock = new PriorityLock();

    private final Backend mBackend;
    private final Uri mUri;
    private final long mTimeout;
    private final boolean mPriority;
    private volatile Socket mSocket;

    GetCapabilitiesTask(Backend backend, Uri uri, long timeout, boolean priority) {
        mUri = uri;
        mBackend = backend;
        mTimeout = timeout;
        mPriority = priority;
    }

    private boolean isDeviceOnline(Uri uri) {
        try (Socket socket = new Socket()) {
            mSocket = socket;
            InetSocketAddress a = new InetSocketAddress(uri.getHost(), uri.getPort());
            socket.connect(a, (int) mTimeout);
            return true;
        } catch (IOException e) {
            return false;
        } finally {
            mSocket = null;
        }
    }

    /** Forcibly cancel this task, including stopping any socket that was opened */
    public void forceCancel() {
        cancel(true);
        Socket socket = mSocket;
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException e) {
                // Ignored
            }
        }
    }

    @Override
    protected LocalPrinterCapabilities doInBackground(Void... dummy) {
        long start = System.currentTimeMillis();

        LocalPrinterCapabilities printerCaps = new LocalPrinterCapabilities();
        try {
            printerCaps.inetAddress = InetAddress.getByName(mUri.getHost());
        } catch (UnknownHostException e) {
            return null;
        }

        boolean online = isDeviceOnline(mUri);
        if (DEBUG) {
            Log.d(TAG, "isDeviceOnline uri=" + mUri + " online=" + online
                    + " (" + (System.currentTimeMillis() - start) + "ms)");
        }

        if (!online || isCancelled()) {
            return null;
        }

        // Do not permit more than a single call to this API or crashes may result
        try {
            // Always allow priority capability requests to execute first
            sLock.lock(mPriority ? 1 : 0);
        } catch (InterruptedException e) {
            return null;
        }
        int status = -1;
        start = System.currentTimeMillis();
        try {
            if (isCancelled()) {
                return null;
            }
            status = mBackend.nativeGetCapabilities(Backend.getIp(mUri.getHost()),
                    mUri.getPort(), mUri.getPath(), mUri.getScheme(), mTimeout, printerCaps);
        } finally {
            sLock.unlock();
        }

        if (DEBUG) {
            Log.d(TAG, "callNativeGetCapabilities uri=" + mUri + " status=" + status
                    + " (" + (System.currentTimeMillis() - start) + "ms)");
        }

        return status == BackendConstants.STATUS_OK ? printerCaps : null;
    }
}
