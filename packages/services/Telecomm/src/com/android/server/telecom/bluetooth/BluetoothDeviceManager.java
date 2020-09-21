/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.server.telecom.bluetooth;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.telecom.Log;

import com.android.server.telecom.BluetoothAdapterProxy;
import com.android.server.telecom.BluetoothHeadsetProxy;
import com.android.server.telecom.TelecomSystem;

import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Collectors;

public class BluetoothDeviceManager {
    private final BluetoothProfile.ServiceListener mBluetoothProfileServiceListener =
            new BluetoothProfile.ServiceListener() {
                @Override
                public void onServiceConnected(int profile, BluetoothProfile proxy) {
                    Log.startSession("BMSL.oSC");
                    try {
                        synchronized (mLock) {
                            if (profile == BluetoothProfile.HEADSET) {
                                mBluetoothHeadsetService =
                                        new BluetoothHeadsetProxy((BluetoothHeadset) proxy);
                                Log.i(this, "- Got BluetoothHeadset: " + mBluetoothHeadsetService);
                            } else {
                                Log.w(this, "Connected to non-headset bluetooth service." +
                                        " Not changing bluetooth headset.");
                            }
                        }
                    } finally {
                        Log.endSession();
                    }
                }

                @Override
                public void onServiceDisconnected(int profile) {
                    Log.startSession("BMSL.oSD");
                    try {
                        synchronized (mLock) {
                            mBluetoothHeadsetService = null;
                            Log.i(BluetoothDeviceManager.this, "Lost BluetoothHeadset service. " +
                                    "Removing all tracked devices.");
                            List<BluetoothDevice> devicesToRemove = new LinkedList<>(
                                    mConnectedDevicesByAddress.values());
                            mConnectedDevicesByAddress.clear();
                            for (BluetoothDevice device : devicesToRemove) {
                                mBluetoothRouteManager.onDeviceLost(device.getAddress());
                            }
                        }
                    } finally {
                        Log.endSession();
                    }
                }
           };

    private final LinkedHashMap<String, BluetoothDevice> mConnectedDevicesByAddress =
            new LinkedHashMap<>();
    private final TelecomSystem.SyncRoot mLock;

    private BluetoothRouteManager mBluetoothRouteManager;
    private BluetoothHeadsetProxy mBluetoothHeadsetService;

    public BluetoothDeviceManager(Context context, BluetoothAdapterProxy bluetoothAdapter,
            TelecomSystem.SyncRoot lock) {
        mLock = lock;

        if (bluetoothAdapter != null) {
            bluetoothAdapter.getProfileProxy(context, mBluetoothProfileServiceListener,
                    BluetoothProfile.HEADSET);
        }
    }

    public void setBluetoothRouteManager(BluetoothRouteManager brm) {
        mBluetoothRouteManager = brm;
    }

    public int getNumConnectedDevices() {
        return mConnectedDevicesByAddress.size();
    }

    public Collection<BluetoothDevice> getConnectedDevices() {
        return mConnectedDevicesByAddress.values();
    }

    public String getMostRecentlyConnectedDevice(String excludeAddress) {
        String result = null;
        synchronized (mLock) {
            for (String addr : mConnectedDevicesByAddress.keySet()) {
                if (!Objects.equals(addr, excludeAddress)) {
                    result = addr;
                }
            }
        }
        return result;
    }

    public BluetoothHeadsetProxy getHeadsetService() {
        return mBluetoothHeadsetService;
    }

    public void setHeadsetServiceForTesting(BluetoothHeadsetProxy bluetoothHeadset) {
        mBluetoothHeadsetService = bluetoothHeadset;
    }

    public BluetoothDevice getDeviceFromAddress(String address) {
        return mConnectedDevicesByAddress.get(address);
    }

    void onDeviceConnected(BluetoothDevice device) {
        synchronized (mLock) {
            if (!mConnectedDevicesByAddress.containsKey(device.getAddress())) {
                mConnectedDevicesByAddress.put(device.getAddress(), device);
                mBluetoothRouteManager.onDeviceAdded(device.getAddress());
            }
        }
    }

    void onDeviceDisconnected(BluetoothDevice device) {
        synchronized (mLock) {
            if (mConnectedDevicesByAddress.containsKey(device.getAddress())) {
                mConnectedDevicesByAddress.remove(device.getAddress());
                mBluetoothRouteManager.onDeviceLost(device.getAddress());
            }
        }
    }
}
