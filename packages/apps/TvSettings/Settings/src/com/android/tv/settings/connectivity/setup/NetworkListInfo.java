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

package com.android.tv.settings.connectivity.setup;

import android.arch.lifecycle.ViewModel;

import com.android.settingslib.wifi.WifiTracker;

/**
 * Class that stores the network list info.
 */
public class NetworkListInfo extends ViewModel {
    private static final int NETWORK_REFRESH_BUFFER_DURATION = 5000;
    private WifiTracker mWifiTracker;
    private boolean mShowSkipNetwork;
    private long mNextNetworkRefreshTime;

    public boolean isShowSkipNetwork() {
        return mShowSkipNetwork;
    }

    public void setShowSkipNetwork(boolean showSkipNetwork) {
        this.mShowSkipNetwork = showSkipNetwork;
    }

    public long getNextNetworkRefreshTime() {
        return mNextNetworkRefreshTime;
    }

    /**
     * Update the next network refresh time.
     */
    public void updateNextNetworkRefreshTime() {
        mNextNetworkRefreshTime = System.currentTimeMillis() + NETWORK_REFRESH_BUFFER_DURATION;
    }

    /**
     * Initialize the network refresh time to current system time.
     */
    public void initNetworkRefreshTime() {
        mNextNetworkRefreshTime = System.currentTimeMillis();
    }

    public WifiTracker getWifiTracker() {
        return mWifiTracker;
    }

    public void setWifiTracker(WifiTracker wifiTracker) {
        this.mWifiTracker = wifiTracker;
    }
}
