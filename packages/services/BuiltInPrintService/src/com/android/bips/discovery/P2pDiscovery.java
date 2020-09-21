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

package com.android.bips.discovery;

import android.net.Uri;
import android.net.wifi.p2p.WifiP2pDevice;
import android.util.Log;

import com.android.bips.BuiltInPrintService;
import com.android.bips.p2p.P2pPeerListener;

/**
 * Discover previously-added P2P devices, if any.
 */
public class P2pDiscovery extends SavedDiscovery implements P2pPeerListener {
    public static final String SCHEME_P2P = "p2p";
    private static final String TAG = P2pDiscovery.class.getSimpleName();
    private static final boolean DEBUG = false;

    private boolean mDiscoveringPeers = false;

    public P2pDiscovery(BuiltInPrintService printService) {
        super(printService);
    }

    /** Convert a peer device to a debug representation */
    public static DiscoveredPrinter toPrinter(WifiP2pDevice device) {
        Uri path = toPath(device);
        String deviceName = device.deviceName;
        if (deviceName.trim().isEmpty()) {
            deviceName = device.deviceAddress;
        }
        return new DiscoveredPrinter(path, deviceName, path, null);
    }

    /** Convert a peer device address to a Uri-based path */
    public static Uri toPath(WifiP2pDevice device) {
        return Uri.parse(SCHEME_P2P + "://" + device.deviceAddress.replace(":", "-"));
    }

    @Override
    void onStart() {
        if (DEBUG) Log.d(TAG, "onStart()");
        startPeerDiscovery();
    }

    @Override
    void onStop() {
        if (DEBUG) Log.d(TAG, "onStop()");
        if (mDiscoveringPeers) {
            mDiscoveringPeers = false;
            getPrintService().getP2pMonitor().stopDiscover(this);
            allPrintersLost();
        }
    }

    private void startPeerDiscovery() {
        // Ignore if already started or no known P2P printers exist
        if (mDiscoveringPeers || getSavedPrinters().isEmpty()) {
            return;
        }
        mDiscoveringPeers = true;
        getPrintService().getP2pMonitor().discover(this);
    }

    @Override
    public void onPeerFound(WifiP2pDevice peer) {
        DiscoveredPrinter printer = toPrinter(peer);
        if (DEBUG) Log.d(TAG, "onPeerFound " + printer);

        // Only find saved printers
        for (DiscoveredPrinter saved : getSavedPrinters()) {
            if (saved.path.equals(printer.path)) {
                printerFound(saved);
            }
        }
    }

    @Override
    public void onPeerLost(WifiP2pDevice peer) {
        DiscoveredPrinter printer = toPrinter(peer);
        if (DEBUG) Log.d(TAG, "onPeerLost " + printer);
        printerLost(printer.getUri());
    }

    /** Adds a known, P2P-discovered, valid printer */
    public void addValidPrinter(DiscoveredPrinter printer) {
        if (addSavedPrinter(printer)) {
            printerFound(printer);
            if (isStarted()) {
                startPeerDiscovery();
            }
        }
    }
}
