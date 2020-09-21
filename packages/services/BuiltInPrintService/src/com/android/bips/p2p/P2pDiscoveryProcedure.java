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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Looper;
import android.util.Log;

import com.android.bips.BuiltInPrintService;
import com.android.bips.util.BroadcastMonitor;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.regex.Pattern;

/**
 * Manage the process of discovering P2P printer devices
 */
class P2pDiscoveryProcedure {
    private static final String TAG = P2pDiscoveryProcedure.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static final Pattern PRINTER_PATTERN =
            Pattern.compile("^(^3-.+-[145])|(0003.+000[145])$");

    private final WifiP2pManager mP2pManager;
    private final List<P2pPeerListener> mListeners = new CopyOnWriteArrayList<>();
    private final List<WifiP2pDevice> mPeers = new ArrayList<>();
    private BroadcastMonitor mBroadcastMonitor;
    private WifiP2pManager.Channel mChannel;

    P2pDiscoveryProcedure(BuiltInPrintService service, WifiP2pManager p2pManager,
            P2pPeerListener listener) {
        mP2pManager = p2pManager;
        if (DEBUG) Log.d(TAG, "P2pDiscoveryProcedure()");
        mChannel = mP2pManager.initialize(service, Looper.getMainLooper(), null);
        mListeners.add(listener);

        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION.equals(action)) {
                    int state = intent.getIntExtra(WifiP2pManager.EXTRA_WIFI_STATE, -1);
                    boolean isP2pEnabled = state == WifiP2pManager.WIFI_P2P_STATE_ENABLED;
                    if (DEBUG) Log.d(TAG, "WIFI_P2P_STATE_CHANGED_ACTION: enabled=" + isP2pEnabled);
                    if (isP2pEnabled) {
                        mP2pManager.stopPeerDiscovery(mChannel, null);
                        mP2pManager.discoverPeers(mChannel, null);
                    }
                } else if (WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION.equals(action)) {
                    WifiP2pDeviceList list = intent.getParcelableExtra(
                            WifiP2pManager.EXTRA_P2P_DEVICE_LIST);
                    Collection<WifiP2pDevice> newPeers = list.getDeviceList();
                    updatePeers(newPeers);

                    if (newPeers.isEmpty()) {
                        // Remind system we are still interested
                        mP2pManager.stopPeerDiscovery(mChannel, null);
                        mP2pManager.discoverPeers(mChannel, null);
                    }
                }
            }
        };

        mBroadcastMonitor = service.receiveBroadcasts(receiver,
                WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION,
                WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);
        mP2pManager.discoverPeers(mChannel, null);
    }

    void addListener(P2pPeerListener listener) {
        mListeners.add(listener);
        if (!mPeers.isEmpty()) {
            for (WifiP2pDevice peer : mPeers) {
                listener.onPeerFound(peer);
            }
        }
    }

    void removeListener(P2pPeerListener listener) {
        mListeners.remove(listener);
    }

    List<P2pPeerListener> getListeners() {
        return mListeners;
    }

    /**
     * Signal find/loss of each device to listeners as it occurs
     */
    private void updatePeers(Collection<WifiP2pDevice> newPeers) {
        List<WifiP2pDevice> oldPeers = new ArrayList<>(mPeers);

        // Reset peer list and populate with new printer-type devices
        mPeers.clear();
        for (WifiP2pDevice peer : newPeers) {
            if (PRINTER_PATTERN.matcher(peer.primaryDeviceType).find()) {
                mPeers.add(peer);
            }
        }

        // Notify newly found devices
        Set<String> foundAddresses = new HashSet<>();
        for (WifiP2pDevice peer : mPeers) {
            foundAddresses.add(peer.deviceAddress);
            WifiP2pDevice old = getDevice(oldPeers, peer.deviceAddress);
            if (old == null || !old.equals(peer)) {
                for (P2pPeerListener listener : mListeners) {
                    listener.onPeerFound(peer);
                }
            }
        }

        // Notify lost devices
        for (WifiP2pDevice oldPeer : oldPeers) {
            if (!foundAddresses.contains(oldPeer.deviceAddress)) {
                for (P2pPeerListener listener : mListeners) {
                    listener.onPeerLost(oldPeer);
                }
            }
        }
    }

    private WifiP2pDevice getDevice(Collection<WifiP2pDevice> peers, String address) {
        for (WifiP2pDevice found : peers) {
            if (found.deviceAddress.equals(address)) {
                return found;
            }
        }
        return null;
    }

    /** Stop the discovery procedure */
    public void cancel() {
        if (DEBUG) Log.d(TAG, "stop()");
        mBroadcastMonitor.close();
        if (mChannel != null) {
            mP2pManager.stopPeerDiscovery(mChannel, null);
            mChannel.close();
            mChannel = null;
        }
    }
}
