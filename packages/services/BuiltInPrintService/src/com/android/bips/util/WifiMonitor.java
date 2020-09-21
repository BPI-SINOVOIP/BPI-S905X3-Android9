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

package com.android.bips.util;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

import com.android.bips.BuiltInPrintService;

/** Reliably reports on changes to Wi-Fi connectivity state */
public class WifiMonitor {
    private static final String TAG = WifiMonitor.class.getSimpleName();
    private static final boolean DEBUG = false;

    private BroadcastMonitor mBroadcasts;
    private Listener mListener;

    /** Current connectivity state or null if not known yet */
    private Boolean mConnected;

    /**
     * Begin listening for connectivity changes, supplying the connectivity state to the listener
     * until stopped.
     */
    public WifiMonitor(BuiltInPrintService service, Listener listener) {
        if (DEBUG) Log.d(TAG, "WifiMonitor()");
        ConnectivityManager connectivityManager =
                (ConnectivityManager) service.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager == null) {
            return;
        }

        mListener = listener;
        mBroadcasts = service.receiveBroadcasts(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (ConnectivityManager.CONNECTIVITY_ACTION.equals(intent.getAction())) {
                    NetworkInfo info = connectivityManager.getActiveNetworkInfo();
                    boolean isConnected = info != null && info.isConnected();
                    if (mListener != null && (mConnected == null || mConnected != isConnected)) {
                        mConnected = isConnected;
                        mListener.onConnectionStateChanged(mConnected);
                    }
                }
            }
        }, ConnectivityManager.CONNECTIVITY_ACTION);
    }

    /** Cease monitoring Wi-Fi connectivity status */
    public void close() {
        if (DEBUG) Log.d(TAG, "close()");
        if (mBroadcasts != null) {
            mBroadcasts.close();
        }
        mListener = null;
    }

    /** Communicate changes to the Wi-Fi connection state */
    public interface Listener {
        /** Called when the Wi-Fi connection state changes */
        void onConnectionStateChanged(boolean isConnected);
    }
}
