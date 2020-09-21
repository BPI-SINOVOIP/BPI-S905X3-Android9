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
import android.net.NetworkInfo;
import android.net.wifi.WpsInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Looper;
import android.util.Log;

import com.android.bips.BuiltInPrintService;
import com.android.bips.DelayedAction;
import com.android.bips.util.BroadcastMonitor;

import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Manage the process of connecting to a previously discovered P2P device
 */
public class P2pConnectionProcedure extends BroadcastReceiver {
    private static final String TAG = P2pConnectionProcedure.class.getSimpleName();
    private static final boolean DEBUG = false;

    private static final int P2P_CONNECT_DELAYED_PERIOD = 3000;

    private final BuiltInPrintService mService;
    private final WifiP2pManager mP2pManager;
    private final WifiP2pDevice mPeer;
    private final BroadcastMonitor mConnectionMonitor;
    private final List<P2pConnectionListener> mListeners = new CopyOnWriteArrayList<>();
    private WifiP2pManager.Channel mChannel;
    private String mNetwork;
    private WifiP2pInfo mInfo;
    private boolean mInvited = false;
    private boolean mDelayed = false;
    private DelayedAction mDetectDelayed;

    P2pConnectionProcedure(BuiltInPrintService service, WifiP2pManager p2pManager,
            WifiP2pDevice peer, P2pConnectionListener listener) {
        mService = service;
        mP2pManager = p2pManager;
        mPeer = peer;
        mConnectionMonitor = service.receiveBroadcasts(this,
                WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION,
                WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);
        if (DEBUG) Log.d(TAG, "Connecting to " + mPeer.deviceAddress);
        mChannel = mP2pManager.initialize(service, Looper.getMainLooper(), null);
        mListeners.add(listener);
        mP2pManager.connect(mChannel, configForPeer(peer), null);
    }

    private WifiP2pConfig configForPeer(WifiP2pDevice peer) {
        WifiP2pConfig config = new WifiP2pConfig();
        config.deviceAddress = peer.deviceAddress;
        if (peer.wpsPbcSupported()) {
            config.wps.setup = WpsInfo.PBC;
        } else if (peer.wpsKeypadSupported()) {
            config.wps.setup = WpsInfo.KEYPAD;
        } else {
            config.wps.setup = WpsInfo.DISPLAY;
        }
        return config;
    }

    /** Return the peer associated with this connection procedure */
    public WifiP2pDevice getPeer() {
        return mPeer;
    }

    /** Return true if the specified listener is currently listening to this object */
    boolean hasListener(P2pConnectionListener listener) {
        return mListeners.contains(listener);
    }

    void addListener(P2pConnectionListener listener) {
        if (mInfo != null) {
            listener.onConnectionOpen(mNetwork, mInfo);
        }
        mListeners.add(listener);
    }

    void removeListener(P2pConnectionListener listener) {
        mListeners.remove(listener);
    }

    int getListenerCount() {
        return mListeners.size();
    }

    /** Close this connection */
    public void close() {
        if (DEBUG) Log.d(TAG, "stop() for " + mPeer.deviceAddress);
        mListeners.clear();
        mConnectionMonitor.close();
        if (mDetectDelayed != null) {
            mDetectDelayed.cancel();
        }
        if (mChannel != null) {
            mP2pManager.cancelConnect(mChannel, null);
            mP2pManager.removeGroup(mChannel, null);
            mChannel.close();
            mChannel = null;
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION.equals(intent.getAction())) {
            NetworkInfo network = intent.getParcelableExtra(WifiP2pManager.EXTRA_NETWORK_INFO);
            WifiP2pGroup group = intent.getParcelableExtra(WifiP2pManager.EXTRA_WIFI_P2P_GROUP);
            WifiP2pInfo info = intent.getParcelableExtra(WifiP2pManager.EXTRA_WIFI_P2P_INFO);

            if (DEBUG) Log.d(TAG, "Connection state=" + network.getState());

            if (network.isConnected()) {
                if (isConnectedToPeer(group)) {
                    if (DEBUG) Log.d(TAG, "Group=" + group.getNetworkName() + ", info=" + info);
                    if (mDelayed) {
                        // We notified a delay in the past, remove this
                        for (P2pConnectionListener listener : mListeners) {
                            listener.onConnectionDelayed(false);
                        }
                    } else {
                        // Cancel any future delayed indications
                        if (mDetectDelayed != null) {
                            mDetectDelayed.cancel();
                        }
                    }

                    mNetwork = group.getInterface();
                    mInfo = info;
                    for (P2pConnectionListener listener : mListeners) {
                        listener.onConnectionOpen(mNetwork, mInfo);
                    }
                }
            } else if (mInvited) {
                // Only signal connection closure if we reached the invitation phase
                for (P2pConnectionListener listener : mListeners) {
                    listener.onConnectionClosed();
                }
                close();
            }
        } else if (WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION.equals(intent.getAction())) {
            WifiP2pDeviceList list = intent.getParcelableExtra(
                    WifiP2pManager.EXTRA_P2P_DEVICE_LIST);
            WifiP2pDevice device = list.get(mPeer.deviceAddress);
            if (DEBUG) Log.d(TAG, "Peers changed, device is " + P2pMonitor.toString(device));

            if (!mInvited && device != null && device.status == WifiP2pDevice.INVITED) {
                // Upon first invite, start timer to detect delayed connection
                mInvited = true;
                mDetectDelayed = mService.delay(P2P_CONNECT_DELAYED_PERIOD, () -> {
                    mDelayed = true;
                    for (P2pConnectionListener listener : mListeners) {
                        listener.onConnectionDelayed(true);
                    }
                });
            }
        }
    }

    /** Return true if group is connected to the peer */
    private boolean isConnectedToPeer(WifiP2pGroup group) {
        WifiP2pDevice owner = group.getOwner();
        if (owner != null && owner.deviceAddress.equals(mPeer.deviceAddress)) {
            return true;
        }
        for (WifiP2pDevice client : group.getClientList()) {
            if (client.deviceAddress.equals(mPeer.deviceAddress)) {
                return true;
            }
        }
        return false;
    }
}
