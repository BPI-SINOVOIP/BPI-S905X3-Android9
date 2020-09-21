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

import android.content.Context;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pManager;
import android.util.Log;

import com.android.bips.BuiltInPrintService;

/**
 * Globally manage P2P discovery and connectivity
 */
public class P2pMonitor {
    private static final String TAG = P2pMonitor.class.getSimpleName();
    private static final boolean DEBUG = false;

    private final BuiltInPrintService mService;
    private final WifiP2pManager mP2pManager;
    private P2pDiscoveryProcedure mPeerDiscovery;
    private P2pConnectionProcedure mConnection;
    private String mConnectedInterface;

    public P2pMonitor(BuiltInPrintService service) {
        mService = service;
        mP2pManager = (WifiP2pManager) mService.getSystemService(Context.WIFI_P2P_SERVICE);
    }

    /** Return a printable String form of a {@link WifiP2pDevice} */
    public static String toString(WifiP2pDevice device) {
        if (device == null) {
            return "null";
        } else {
            return device.deviceName + " " + device.deviceAddress + ", status="
                    + statusString(device.status);
        }
    }

    private static String statusString(int status) {
        switch (status) {
            case WifiP2pDevice.AVAILABLE:
                return "available";
            case WifiP2pDevice.CONNECTED:
                return "connected";
            case WifiP2pDevice.FAILED:
                return "failed";
            case WifiP2pDevice.INVITED:
                return "invited";
            case WifiP2pDevice.UNAVAILABLE:
                return "unavailable";
            default:
                return "unknown";
        }
    }

    /**
     * Start a discovery of Wi-Fi Direct peers until all requests are closed
     */
    public void discover(P2pPeerListener listener) {
        if (DEBUG) Log.d(TAG, "discover()");

        if (mP2pManager == null) {
            return;
        }
        if (mPeerDiscovery == null) {
            mPeerDiscovery = new P2pDiscoveryProcedure(mService, mP2pManager, listener);
        } else {
            mPeerDiscovery.addListener(listener);
        }
    }

    /**
     * Remove the request to discover having the same listener. When all outstanding requests are
     * removed, discovery itself is stopped.
     */
    public void stopDiscover(P2pPeerListener listener) {
        if (DEBUG) Log.d(TAG, "stopDiscover");
        if (mPeerDiscovery != null) {
            mPeerDiscovery.removeListener(listener);
            if (mPeerDiscovery.getListeners().isEmpty()) {
                mPeerDiscovery.cancel();
                mPeerDiscovery = null;
            }
        }
    }

    /**
     * Request connection to a peer (which may already be connected) at least until stopped. Keeps
     * the current connection open as long as it might be useful.
     */
    public void connect(WifiP2pDevice peer, P2pConnectionListener listener) {
        if (DEBUG) Log.d(TAG, "connect(" + toString(peer) + ")");

        if (mP2pManager == null) {
            // Device has no P2P support so indicate failure
            mService.getMainHandler().post(listener::onConnectionClosed);
            return;
        }

        // Check for competing connection
        if (mConnection != null && !peer.deviceAddress.equals(mConnection.getPeer()
                .deviceAddress)) {
            if (mConnection.getListenerCount() == 1) {
                // The only listener is our internal one, so close this connection to make room
                mConnection.close();
                mConnection = null;
            } else {
                // Cannot open connection
                mService.getMainHandler().post(listener::onConnectionClosed);
                return;
            }
        }

        // Check for existing connection to the same device
        if (mConnection == null) {
            // Create a new connection request with our internal listener
            mConnection = new P2pConnectionProcedure(mService, mP2pManager, peer,
                    new P2pConnectionListener() {
                        @Override
                        public void onConnectionOpen(String networkInterface, WifiP2pInfo info) {
                            mConnectedInterface = networkInterface;
                        }

                        @Override
                        public void onConnectionClosed() {
                            mConnectedInterface = null;
                        }

                        @Override
                        public void onConnectionDelayed(boolean delayed) {
                        }
                    });
        }
        mConnection.addListener(listener);
    }

    /**
     * Give up on the connection request associated with a listener. The connection will stay
     * open as long as other requests exist.
     */
    void stopConnect(P2pConnectionListener listener) {
        if (mConnection == null || !mConnection.hasListener(listener)) {
            return;
        }

        if (DEBUG) Log.d(TAG, "stopConnect " + toString(mConnection.getPeer()));
        mConnection.removeListener(listener);

        // If current connection attempt is incomplete and no longer required, close it.
        if (mConnection.getListenerCount() == 1 && mConnectedInterface == null) {
            if (DEBUG) Log.d(TAG, "Abandoning connection request");
            mConnection.close();
            mConnection = null;
        }
    }

    /** Return the current connection procedure, if any */
    P2pConnectionProcedure getConnection() {
        return mConnection;
    }

    /** Return the current connected interface, if any */
    public String getConnectedInterface() {
        return mConnectedInterface;
    }

    /** Forcibly stops all connections/discoveries in progress, if any */
    public void stopAll() {
        if (mConnection != null) {
            mConnection.close();
            mConnection = null;
            mConnectedInterface = null;
        }
        if (mPeerDiscovery != null) {
            mPeerDiscovery.cancel();
            mPeerDiscovery = null;
        }
    }
}
