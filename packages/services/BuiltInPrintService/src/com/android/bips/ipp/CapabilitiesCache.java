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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;
import android.util.LruCache;

import com.android.bips.BuiltInPrintService;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.jni.LocalPrinterCapabilities;
import com.android.bips.p2p.P2pUtils;
import com.android.bips.util.BroadcastMonitor;
import com.android.bips.util.WifiMonitor;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Consumer;

/**
 * A cache of printer URIs (see {@link DiscoveredPrinter#path}) to printer capabilities,
 * with the ability to fetch them on cache misses. {@link #close} must be called when use
 * is complete.
 */
public class CapabilitiesCache extends LruCache<Uri, LocalPrinterCapabilities> implements
        AutoCloseable {
    private static final String TAG = CapabilitiesCache.class.getSimpleName();
    private static final boolean DEBUG = false;

    // Maximum number of capability queries to perform at any one time, so as not to overwhelm
    // AsyncTask.THREAD_POOL_EXECUTOR
    public static final int DEFAULT_MAX_CONCURRENT = 3;

    // Maximum number of printers expected on a single network
    private static final int CACHE_SIZE = 100;

    // Maximum time per retry before giving up on first pass
    private static final int FIRST_PASS_TIMEOUT = 500;

    // Maximum time per retry before giving up on second pass. Must differ from FIRST_PASS_TIMEOUT.
    private static final int SECOND_PASS_TIMEOUT = 8000;

    // Outstanding requests based on printer path
    private final Map<Uri, Request> mRequests = new HashMap<>();
    private final Set<Uri> mToEvict = new HashSet<>();
    private final Set<Uri> mToEvictP2p = new HashSet<>();
    private final int mMaxConcurrent;
    private final Backend mBackend;
    private final WifiMonitor mWifiMonitor;
    private final BroadcastMonitor mP2pMonitor;
    private final BuiltInPrintService mService;
    private boolean mIsStopped = false;

    /**
     * @param maxConcurrent Maximum number of capabilities requests to make at any one time
     */
    public CapabilitiesCache(BuiltInPrintService service, Backend backend, int maxConcurrent) {
        super(CACHE_SIZE);
        if (DEBUG) Log.d(TAG, "CapabilitiesCache()");

        mService = service;
        mBackend = backend;
        mMaxConcurrent = maxConcurrent;

        mP2pMonitor = mService.receiveBroadcasts(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                NetworkInfo info = intent.getParcelableExtra(WifiP2pManager.EXTRA_NETWORK_INFO);
                if (!info.isConnected()) {
                    // Evict specified device capabilities when P2P network is lost.
                    if (DEBUG) Log.d(TAG, "Evicting P2P " + mToEvictP2p);
                    for (Uri uri : mToEvictP2p) {
                        remove(uri);
                    }
                    mToEvictP2p.clear();
                }
            }
        }, WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);

        mWifiMonitor = new WifiMonitor(service, connected -> {
            if (!connected) {
                // Evict specified device capabilities when network is lost.
                if (DEBUG) Log.d(TAG, "Evicting Wi-Fi " + mToEvict);
                for (Uri uri : mToEvict) {
                    remove(uri);
                }
                mToEvict.clear();
            }
        });
    }

    @Override
    public void close() {
        if (DEBUG) Log.d(TAG, "stop()");
        mIsStopped = true;
        mWifiMonitor.close();
        mP2pMonitor.close();
    }

    /** Callback for receiving capabilities */
    public interface OnLocalPrinterCapabilities {
        /** Called when capabilities are retrieved */
        void onCapabilities(LocalPrinterCapabilities capabilities);
    }

    /**
     * Query capabilities and return full results to the listener. A full result includes
     * enough backend data and is suitable for printing. If full data is already available
     * it will be returned to the callback immediately.
     *
     * @param highPriority if true, perform this query before others
     * @param onLocalPrinterCapabilities listener to receive capabilities. Receives null
     *                                   if the attempt fails
     */
    public void request(DiscoveredPrinter printer, boolean highPriority,
            OnLocalPrinterCapabilities onLocalPrinterCapabilities) {
        if (DEBUG) Log.d(TAG, "request() printer=" + printer + " high=" + highPriority);

        LocalPrinterCapabilities capabilities = get(printer);
        if (capabilities != null && capabilities.nativeData != null) {
            onLocalPrinterCapabilities.onCapabilities(capabilities);
            return;
        }

        if (P2pUtils.isOnConnectedInterface(mService, printer)) {
            if (DEBUG) Log.d(TAG, "Adding to P2P evict list: " + printer);
            mToEvictP2p.add(printer.path);
        } else {
            if (DEBUG) Log.d(TAG, "Adding to WLAN evict list: " + printer);
            mToEvict.add(printer.path);
        }

        // Create a new request with timeout based on priority
        Request request = mRequests.computeIfAbsent(printer.path, uri ->
                new Request(printer, highPriority ? SECOND_PASS_TIMEOUT : FIRST_PASS_TIMEOUT));

        if (highPriority) {
            request.mHighPriority = true;
        }

        request.mCallbacks.add(onLocalPrinterCapabilities);

        startNextRequest();
    }

    /**
     * Returns capabilities for the specified printer, if known
     */
    public LocalPrinterCapabilities get(DiscoveredPrinter printer) {
        return get(printer.path);
    }

    /**
     * Cancel all outstanding attempts to get capabilities for this callback
     */
    public void cancel(OnLocalPrinterCapabilities onLocalPrinterCapabilities) {
        List<Uri> toDrop = new ArrayList<>();
        for (Map.Entry<Uri, Request> entry : mRequests.entrySet()) {
            Request request = entry.getValue();
            request.mCallbacks.remove(onLocalPrinterCapabilities);
            if (request.mCallbacks.isEmpty()) {
                toDrop.add(entry.getKey());
                request.cancel();
            }
        }
        for (Uri request : toDrop) {
            mRequests.remove(request);
        }
    }

    /** Look for next query and launch it */
    private void startNextRequest() {
        final Request request = getNextRequest();
        if (request == null) {
            return;
        }

        request.start();
    }

    /** Return the next request if it is appropriate to perform one */
    private Request getNextRequest() {
        Request found = null;
        int total = 0;
        for (Request request : mRequests.values()) {
            if (request.mQuery != null) {
                total++;
            } else if (found == null || (!found.mHighPriority && request.mHighPriority)
                    || (found.mHighPriority == request.mHighPriority
                    && request.mTimeout < found.mTimeout)) {
                // First valid or higher priority request
                found = request;
            }
        }

        if (total >= mMaxConcurrent) {
            return null;
        }

        return found;
    }

    /** Holds an outstanding capabilities request */
    public class Request implements Consumer<LocalPrinterCapabilities> {
        final DiscoveredPrinter mPrinter;
        final List<OnLocalPrinterCapabilities> mCallbacks = new ArrayList<>();
        GetCapabilitiesTask mQuery;
        boolean mHighPriority = false;
        long mTimeout;

        Request(DiscoveredPrinter printer, long timeout) {
            mPrinter = printer;
            mTimeout = timeout;
        }

        private void start() {
            mQuery = mBackend.getCapabilities(mPrinter.path, mTimeout, mHighPriority, this);
        }

        private void cancel() {
            if (mQuery != null) {
                mQuery.forceCancel();
                mQuery = null;
            }
        }

        @Override
        public void accept(LocalPrinterCapabilities capabilities) {
            DiscoveredPrinter printer = mPrinter;
            if (DEBUG) Log.d(TAG, "Capabilities for " + printer + " cap=" + capabilities);

            if (mIsStopped) {
                return;
            }
            mRequests.remove(printer.path);

            // Grab uuid from capabilities if possible
            Uri capUuid = null;
            if (capabilities != null) {
                if (!TextUtils.isEmpty(capabilities.uuid)) {
                    capUuid = Uri.parse(capabilities.uuid);
                }
                if (printer.uuid != null && !printer.uuid.equals(capUuid)) {
                    Log.w(TAG, "UUID mismatch for " + printer + "; rejecting capabilities");
                    capabilities = null;
                }
            }

            if (capabilities == null) {
                if (mTimeout == FIRST_PASS_TIMEOUT) {
                    // Printer did not respond quickly, try again in the slow lane
                    mTimeout = SECOND_PASS_TIMEOUT;
                    mQuery = null;
                    mRequests.put(printer.path, this);
                    startNextRequest();
                    return;
                } else {
                    remove(printer.getUri());
                }
            } else {
                put(printer.path, capabilities);
            }

            LocalPrinterCapabilities result = capabilities;
            for (OnLocalPrinterCapabilities callback : mCallbacks) {
                callback.onCapabilities(result);
            }
            startNextRequest();
        }
    }
}
