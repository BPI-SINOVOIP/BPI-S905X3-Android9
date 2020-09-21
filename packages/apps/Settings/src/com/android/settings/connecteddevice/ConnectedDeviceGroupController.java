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
package com.android.settings.connecteddevice;

import android.content.pm.PackageManager;
import android.content.Context;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.PreferenceScreen;

import com.android.settings.bluetooth.BluetoothDeviceUpdater;
import com.android.settings.bluetooth.ConnectedBluetoothDeviceUpdater;
import com.android.settings.connecteddevice.dock.DockUpdater;
import com.android.settings.connecteddevice.usb.ConnectedUsbDeviceUpdater;
import com.android.settings.core.BasePreferenceController;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.overlay.DockUpdaterFeatureProvider;
import com.android.settings.overlay.FeatureFactory;
import com.android.settingslib.core.lifecycle.events.OnStart;
import com.android.settingslib.core.lifecycle.events.OnStop;
import com.android.settingslib.core.lifecycle.LifecycleObserver;

/**
 * Controller to maintain the {@link android.support.v7.preference.PreferenceGroup} for all
 * connected devices. It uses {@link DevicePreferenceCallback} to add/remove {@link Preference}
 */
public class ConnectedDeviceGroupController extends BasePreferenceController
        implements PreferenceControllerMixin, LifecycleObserver, OnStart, OnStop,
        DevicePreferenceCallback {

    private static final String KEY = "connected_device_list";

    @VisibleForTesting
    PreferenceGroup mPreferenceGroup;
    private BluetoothDeviceUpdater mBluetoothDeviceUpdater;
    private ConnectedUsbDeviceUpdater mConnectedUsbDeviceUpdater;
    private DockUpdater mConnectedDockUpdater;

    public ConnectedDeviceGroupController(Context context) {
        super(context, KEY);
    }

    @Override
    public void onStart() {
        mBluetoothDeviceUpdater.registerCallback();
        mConnectedUsbDeviceUpdater.registerCallback();
        mConnectedDockUpdater.registerCallback();
    }

    @Override
    public void onStop() {
        mConnectedUsbDeviceUpdater.unregisterCallback();
        mBluetoothDeviceUpdater.unregisterCallback();
        mConnectedDockUpdater.unregisterCallback();
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        if (isAvailable()) {
            mPreferenceGroup = (PreferenceGroup) screen.findPreference(KEY);
            mPreferenceGroup.setVisible(false);

            mBluetoothDeviceUpdater.setPrefContext(screen.getContext());
            mBluetoothDeviceUpdater.forceUpdate();
            mConnectedUsbDeviceUpdater.initUsbPreference(screen.getContext());
            mConnectedDockUpdater.forceUpdate();
        }
    }

    @Override
    public int getAvailabilityStatus() {
        return mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH)
                ? AVAILABLE
                : UNSUPPORTED_ON_DEVICE;
    }

    @Override
    public String getPreferenceKey() {
        return KEY;
    }

    @Override
    public void onDeviceAdded(Preference preference) {
        if (mPreferenceGroup.getPreferenceCount() == 0) {
            mPreferenceGroup.setVisible(true);
        }
        mPreferenceGroup.addPreference(preference);
    }

    @Override
    public void onDeviceRemoved(Preference preference) {
        mPreferenceGroup.removePreference(preference);
        if (mPreferenceGroup.getPreferenceCount() == 0) {
            mPreferenceGroup.setVisible(false);
        }
    }

    @VisibleForTesting
    public void init(BluetoothDeviceUpdater bluetoothDeviceUpdater,
            ConnectedUsbDeviceUpdater connectedUsbDeviceUpdater,
            DockUpdater connectedDockUpdater) {

        mBluetoothDeviceUpdater = bluetoothDeviceUpdater;
        mConnectedUsbDeviceUpdater = connectedUsbDeviceUpdater;
        mConnectedDockUpdater = connectedDockUpdater;
    }

    public void init(DashboardFragment fragment) {
        final Context context = fragment.getContext();
        DockUpdaterFeatureProvider dockUpdaterFeatureProvider =
                FeatureFactory.getFactory(context).getDockUpdaterFeatureProvider();
        final DockUpdater connectedDockUpdater =
                dockUpdaterFeatureProvider.getConnectedDockUpdater(context, this);
        init(new ConnectedBluetoothDeviceUpdater(context, fragment, this),
                new ConnectedUsbDeviceUpdater(context, fragment, this),
                connectedDockUpdater);
    }
}
