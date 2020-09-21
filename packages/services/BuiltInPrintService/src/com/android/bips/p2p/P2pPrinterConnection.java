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

package com.android.bips.p2p;

import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pInfo;
import android.util.Log;

import com.android.bips.BuiltInPrintService;
import com.android.bips.DelayedAction;
import com.android.bips.discovery.ConnectionListener;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.discovery.Discovery;
import com.android.bips.discovery.P2pDiscovery;
import com.android.bips.jni.LocalPrinterCapabilities;

import java.net.Inet4Address;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;

/**
 * Manage the process of connecting to a P2P device, discovering a printer on the new network, and
 * verifying its capabilities.
 */
public class P2pPrinterConnection implements Discovery.Listener, P2pConnectionListener,
        P2pPeerListener {
    private static final String TAG = P2pPrinterConnection.class.getSimpleName();
    private static final boolean DEBUG = false;
    private static final int TIMEOUT_DISCOVERY = 15 * 1000;

    private final BuiltInPrintService mService;
    private final Discovery mMdnsDiscovery;
    private ConnectionListener mListener;

    private DiscoveredPrinter mPrinter;
    private WifiP2pDevice mPeer;
    private NetworkInterface mInterface;
    private DelayedAction mMdnsDiscoveryTimeout;

    private P2pPrinterConnection(BuiltInPrintService service, ConnectionListener listener) {
        mService = service;
        mListener = listener;
        mMdnsDiscovery = mService.getMdnsDiscovery();
    }

    /** Create a new connection to a known P2P peer device */
    public P2pPrinterConnection(BuiltInPrintService service, WifiP2pDevice peer,
            ConnectionListener listener) {
        this(service, listener);
        if (DEBUG) Log.d(TAG, "Connecting to " + P2pMonitor.toString(peer));
        connectToPeer(peer);
    }

    /** Re-discover and create a new connection to a previously discovered P2P device */
    public P2pPrinterConnection(BuiltInPrintService service, DiscoveredPrinter printer,
            ConnectionListener listener) {
        this(service, listener);
        if (DEBUG) Log.d(TAG, "Connecting to " + printer);
        if (P2pUtils.isOnConnectedInterface(service, printer)) {
            WifiP2pDevice peer = service.getP2pMonitor().getConnection().getPeer();
            connectToPeer(peer);
            return;
        }

        mPrinter = printer;
        service.getP2pMonitor().discover(this);
    }

    private void connectToPeer(WifiP2pDevice peer) {
        mPeer = peer;
        mService.getP2pMonitor().connect(mPeer, this);
    }

    @Override
    public void onPeerFound(WifiP2pDevice peer) {
        if (mListener == null) {
            return;
        }

        String address = mPrinter.path.getHost().replaceAll("-", ":");

        if (peer.deviceAddress.equals(address)) {
            mService.getP2pMonitor().stopDiscover(this);
            // Stop discovery and start validation
            connectToPeer(peer);
        }
    }

    @Override
    public void onPeerLost(WifiP2pDevice peer) {
        // Ignored
    }

    @Override
    public void onConnectionOpen(String networkInterface, WifiP2pInfo info) {
        if (mListener == null) {
            return;
        }

        try {
            mInterface = NetworkInterface.getByName(networkInterface);
        } catch (SocketException ignored) {
        }

        if (mInterface == null) {
            if (DEBUG) Log.d(TAG, "Failed to get interface from " + networkInterface);
            mListener.onConnectionComplete(null);
            close();
            return;
        }

        if (DEBUG) Log.d(TAG, "Connected on network interface " + mInterface);

        // Timeout after a while if MDNS does not find a printer
        mMdnsDiscoveryTimeout = mService.delay(TIMEOUT_DISCOVERY, () -> {
            mMdnsDiscovery.stop(this);
            if (mListener != null) {
                mListener.onConnectionComplete(null);
            }
            close();
        });

        mMdnsDiscovery.start(this);
    }

    @Override
    public void onConnectionClosed() {
        if (DEBUG) Log.d(TAG, "closed/failed connection to " + P2pMonitor.toString(mPeer));
        if (mListener != null) {
            mListener.onConnectionComplete(null);
        }
        close();
    }

    @Override
    public void onConnectionDelayed(boolean delayed) {
        if (mListener == null) {
            return;
        }
        mListener.onConnectionDelayed(delayed);
    }

    @Override
    public void onPrinterFound(DiscoveredPrinter printer) {
        if (DEBUG) Log.d(TAG, "onPrinterFound(" + printer + ")");
        if (mListener == null) {
            return;
        }

        Inet4Address printerAddress;
        try {
            printerAddress = (Inet4Address) Inet4Address.getByName(printer.path.getHost());
        } catch (UnknownHostException e) {
            return;
        }

        if (mInterface != null && P2pUtils.isOnInterface(mInterface, printerAddress)) {
            // Stop discovery and start capabilities query
            mMdnsDiscovery.stop(this);
            mMdnsDiscoveryTimeout.cancel();
            mService.getCapabilitiesCache().request(printer, true, capabilities ->
                    onCapabilities(printer, capabilities));
        }
    }

    private void onCapabilities(DiscoveredPrinter printer, LocalPrinterCapabilities capabilities) {
        if (mListener == null) {
            return;
        }

        if (DEBUG) Log.d(TAG, "Printer " + printer + " caps=" + capabilities);
        if (capabilities == null) {
            mListener.onConnectionComplete(null);
            close();
        } else {
            // Make a copy of the printer bearing its P2P path
            DiscoveredPrinter p2pPrinter = new DiscoveredPrinter(printer.uuid, printer.name,
                    P2pDiscovery.toPath(mPeer), printer.location);
            mListener.onConnectionComplete(p2pPrinter);
        }
    }

    @Override
    public void onPrinterLost(DiscoveredPrinter printer) {
    }

    /** Close the connection and any intermediate procedures */
    public void close() {
        if (DEBUG) Log.d(TAG, "close()");
        mMdnsDiscovery.stop(this);
        if (mMdnsDiscoveryTimeout != null) {
            mMdnsDiscoveryTimeout.cancel();
        }
        mService.getP2pMonitor().stopDiscover(this);
        mService.getP2pMonitor().stopConnect(this);

        // No further notifications
        mListener = null;
    }
}
