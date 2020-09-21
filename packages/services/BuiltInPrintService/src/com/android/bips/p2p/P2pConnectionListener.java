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

import android.net.wifi.p2p.WifiP2pInfo;

/**
 * Indicators for steps in the P2P connection procedure
 */
public interface P2pConnectionListener {
    /**
     * The connection is open.
     *
     * @param networkInterface the name of the network interface to which P2P is bound
     * @param info             information about the connection
     */
    void onConnectionOpen(String networkInterface, WifiP2pInfo info);

    /**
     * The connection has closed.
     */
    void onConnectionClosed();

    /**
     * Indicates whether the connection has remained in a half-open state for a while
     */
    void onConnectionDelayed(boolean delayed);
}
