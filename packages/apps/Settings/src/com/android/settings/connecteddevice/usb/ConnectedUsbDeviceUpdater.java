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
package com.android.settings.connecteddevice.usb;

import android.content.Context;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbPort;
import android.support.annotation.VisibleForTesting;
import android.support.v7.preference.Preference;

import com.android.settings.R;
import com.android.settings.connecteddevice.DevicePreferenceCallback;
import com.android.settings.core.SubSettingLauncher;
import com.android.settings.dashboard.DashboardFragment;

/**
 * Controller to maintain connected usb device
 */
public class ConnectedUsbDeviceUpdater {
    private DashboardFragment mFragment;
    private UsbBackend mUsbBackend;
    private DevicePreferenceCallback mDevicePreferenceCallback;
    @VisibleForTesting
    Preference mUsbPreference;
    @VisibleForTesting
    UsbConnectionBroadcastReceiver mUsbReceiver;

    @VisibleForTesting
    UsbConnectionBroadcastReceiver.UsbConnectionListener mUsbConnectionListener =
            (connected, functions, powerRole, dataRole) -> {
                if (connected) {
                    mUsbPreference.setSummary(getSummary(mUsbBackend.getCurrentFunctions(),
                            mUsbBackend.getPowerRole()));
                    mDevicePreferenceCallback.onDeviceAdded(mUsbPreference);
                } else {
                    mDevicePreferenceCallback.onDeviceRemoved(mUsbPreference);
                }
            };

    public ConnectedUsbDeviceUpdater(Context context, DashboardFragment fragment,
            DevicePreferenceCallback devicePreferenceCallback) {
        this(context, fragment, devicePreferenceCallback, new UsbBackend(context));
    }

    @VisibleForTesting
    ConnectedUsbDeviceUpdater(Context context, DashboardFragment fragment,
            DevicePreferenceCallback devicePreferenceCallback, UsbBackend usbBackend) {
        mFragment = fragment;
        mDevicePreferenceCallback = devicePreferenceCallback;
        mUsbBackend = usbBackend;
        mUsbReceiver = new UsbConnectionBroadcastReceiver(context,
                mUsbConnectionListener, mUsbBackend);
    }

    public void registerCallback() {
        // This method could handle multiple register
        mUsbReceiver.register();
    }

    public void unregisterCallback() {
        mUsbReceiver.unregister();
    }

    public void initUsbPreference(Context context) {
        mUsbPreference = new Preference(context, null /* AttributeSet */);
        mUsbPreference.setTitle(R.string.usb_pref);
        mUsbPreference.setIcon(R.drawable.ic_usb);
        mUsbPreference.setOnPreferenceClickListener((Preference p) -> {
            // New version - uses a separate screen.
            new SubSettingLauncher(mFragment.getContext())
                    .setDestination(UsbDetailsFragment.class.getName())
                    .setTitle(R.string.device_details_title)
                    .setSourceMetricsCategory(mFragment.getMetricsCategory())
                    .launch();
            return true;
        });

        forceUpdate();
    }

    private void forceUpdate() {
        // Register so we can get the connection state from sticky intent.
        //TODO(b/70336520): Use an API to get data instead of sticky intent
        mUsbReceiver.register();
    }

    public static int getSummary(long functions, int power) {
        switch (power) {
            case UsbPort.POWER_ROLE_SINK:
                if (functions == UsbManager.FUNCTION_MTP) {
                    return R.string.usb_summary_file_transfers;
                } else if (functions == UsbManager.FUNCTION_RNDIS) {
                    return R.string.usb_summary_tether;
                } else if (functions == UsbManager.FUNCTION_PTP) {
                    return R.string.usb_summary_photo_transfers;
                } else if (functions == UsbManager.FUNCTION_MIDI) {
                    return R.string.usb_summary_MIDI;
                } else {
                    return R.string.usb_summary_charging_only;
                }
            case UsbPort.POWER_ROLE_SOURCE:
                if (functions == UsbManager.FUNCTION_MTP) {
                    return R.string.usb_summary_file_transfers_power;
                } else if (functions == UsbManager.FUNCTION_RNDIS) {
                    return R.string.usb_summary_tether_power;
                } else if (functions == UsbManager.FUNCTION_PTP) {
                    return R.string.usb_summary_photo_transfers_power;
                } else if (functions == UsbManager.FUNCTION_MIDI) {
                    return R.string.usb_summary_MIDI_power;
                } else {
                    return R.string.usb_summary_power_only;
                }
            default:
                return R.string.usb_summary_charging_only;
        }
    }
}
