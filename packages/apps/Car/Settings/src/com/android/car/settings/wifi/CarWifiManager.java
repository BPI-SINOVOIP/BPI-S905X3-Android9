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
package com.android.car.settings.wifi;

import android.annotation.Nullable;
import android.content.Context;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.support.annotation.UiThread;

import com.android.settingslib.wifi.AccessPoint;
import com.android.settingslib.wifi.WifiTracker;

import java.util.ArrayList;
import java.util.List;

/**
 * Manages Wifi configuration: e.g. monitors wifi states, change wifi setting etc.
 */
public class CarWifiManager implements WifiTracker.WifiListener {
    private final Context mContext;
    private Listener mListener;
    private boolean mStarted;

    private WifiTracker mWifiTracker;
    private WifiManager mWifiManager;
    public interface Listener {
        /**
         * Something about wifi setting changed.
         */
        void onAccessPointsChanged();

        /**
         * Called when the state of Wifi has changed, the state will be one of
         * the following.
         *
         * <li>{@link WifiManager#WIFI_STATE_DISABLED}</li>
         * <li>{@link WifiManager#WIFI_STATE_ENABLED}</li>
         * <li>{@link WifiManager#WIFI_STATE_DISABLING}</li>
         * <li>{@link WifiManager#WIFI_STATE_ENABLING}</li>
         * <li>{@link WifiManager#WIFI_STATE_UNKNOWN}</li>
         * <p>
         *
         * @param state The new state of wifi.
         */
        void onWifiStateChanged(int state);
    }

    public CarWifiManager(Context context, Listener listener) {
        mContext = context;
        mListener = listener;
        mWifiManager = (WifiManager) mContext.getSystemService(WifiManager.class);
        mWifiTracker = new WifiTracker(context, this, true, true);
    }

    /**
     * Starts {@link CarWifiManager}.
     * This should be called only from main thread.
     */
    @UiThread
    public void start() {
        if (!mStarted) {
            mStarted = true;
            mWifiTracker.onStart();
        }
    }

    /**
     * Stops {@link CarWifiManager}.
     * This should be called only from main thread.
     */
    @UiThread
    public void stop() {
        if (mStarted) {
            mStarted = false;
            mWifiTracker.onStop();
        }
    }

    /**
     * Destroys {@link CarWifiManager}
     * This should only be called from main thread.
     */
    @UiThread
    public void destroy() {
        mWifiTracker.onDestroy();
    }

    /**
     * Returns a list of all reachable access points.
     */
    public List<AccessPoint> getAllAccessPoints() {
        return getAccessPoints(false);
    }

    /**
     * Returns a list of saved access points.
     */
    public List<AccessPoint> getSavedAccessPoints() {
        return getAccessPoints(true);
    }

    private List<AccessPoint> getAccessPoints(boolean saved) {
        List<AccessPoint> accessPoints = new ArrayList<AccessPoint>();
        if (mWifiManager.isWifiEnabled()) {
            for (AccessPoint accessPoint : mWifiTracker.getAccessPoints()) {
                // ignore out of reach access points.
                if (shouldIncludeAp(accessPoint, saved)) {
                    accessPoints.add(accessPoint);
                }
            }
        }
        return accessPoints;
    }

    private boolean shouldIncludeAp(AccessPoint accessPoint, boolean saved) {
        return saved ? accessPoint.isReachable() && accessPoint.isSaved()
                : accessPoint.isReachable();
    }

    @Nullable
    public AccessPoint getConnectedAccessPoint() {
        for (AccessPoint accessPoint : getAllAccessPoints()) {
            if (accessPoint.getDetailedState() == NetworkInfo.DetailedState.CONNECTED) {
                return accessPoint;
            }
        }
        return null;
    }

    public boolean isWifiEnabled() {
        return mWifiManager.isWifiEnabled();
    }

    public int getWifiState() {
        return mWifiManager.getWifiState();
    }

    public boolean setWifiEnabled(boolean enabled) {
        return mWifiManager.setWifiEnabled(enabled);
    }

    public void connectToPublicWifi(AccessPoint accessPoint, WifiManager.ActionListener listener) {
        accessPoint.generateOpenNetworkConfig();
        mWifiManager.connect(accessPoint.getConfig(), listener);
    }

    @Override
    public void onWifiStateChanged(int state) {
        mListener.onWifiStateChanged(state);
    }

    @Override
    public void onConnectedChanged() {
    }

    @Override
    public void onAccessPointsChanged() {
        mListener.onAccessPointsChanged();
    }
}
