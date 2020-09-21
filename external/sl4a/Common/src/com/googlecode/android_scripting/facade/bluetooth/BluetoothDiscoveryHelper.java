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

package com.googlecode.android_scripting.facade.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import com.googlecode.android_scripting.Log;

import java.util.Set;

/**
 * This class is used to put the device in discovery mode.
 */
public class BluetoothDiscoveryHelper {

    /**
     * BLuetooth discovery listener Interface.
     */
    public interface BluetoothDiscoveryListener {

        /**
         * Add Bonded device.
         */
        void addBondedDevice(String name, String address);

        /**
         * Add Devices.
         */
        void addDevice(String name, String address);

        /**
         * Complete scanning.
         */
        void scanDone();
    }

    private final Context mContext;
    private final BluetoothDiscoveryListener mListener;
    private final BroadcastReceiver mReceiver;

    public BluetoothDiscoveryHelper(
            Context context, BluetoothDiscoveryListener listener) {
    mContext = context;
    mListener = listener;
    mReceiver = new BluetoothReceiver();
    }

    private class BluetoothReceiver extends BroadcastReceiver {
    @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                // Get the BluetoothDevice object from the Intent.
                BluetoothDevice device = intent.getParcelableExtra(
                        BluetoothDevice.EXTRA_DEVICE);
                Log.d("Found device " + device.getAliasName());
                // If it's already paired, skip it, because it's been listed already.
                if (device.getBondState() != BluetoothDevice.BOND_BONDED) {
                    mListener.addDevice(device.getName(), device.getAddress());
                }
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                mListener.scanDone();
            }
        }
    }
    /**
     * Start Bluetooth Discovery.
     */

    public void startDiscovery() {
        BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        if (bluetoothAdapter.isDiscovering()) {
            bluetoothAdapter.cancelDiscovery();
        }

        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
        for (BluetoothDevice device : pairedDevices) {
            mListener.addBondedDevice(device.getName(), device.getAddress());
        }

        IntentFilter filter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        mContext.registerReceiver(mReceiver, filter);

        if (!bluetoothAdapter.isEnabled()) {
            bluetoothAdapter.enable();
        }

        bluetoothAdapter.startDiscovery();
    }

    /**
     * Cancel.
     */
    public void cancel() {
        mContext.unregisterReceiver(mReceiver);
        mListener.scanDone();
    }
}
