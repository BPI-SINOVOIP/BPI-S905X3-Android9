/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;

import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.content.res.Resources;
import android.os.ParcelUuid;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

/**
 * An advertiser for the Bluetooth LE based Fast Pair service.
 * FastPairProvider enables easy Bluetooth pairing between a peripheral and a phone participating in
 * the Fast Pair Seeker role.  When the seeker finds a compatible peripheral a notification prompts
 * the user to begin pairing if desired.  A peripheral should call startAdvertising when it is
 * appropriate to pair, and stopAdvertising when pairing is complete or it is no longer appropriate
 * to pair.
 */
class FastPairProvider {

    private static final String TAG = "FastPairProvider";
    private static final boolean DBG = Utils.DBG;

    // Service ID assigned for FastPair.
    private static final ParcelUuid FastPairServiceUuid = ParcelUuid
            .fromString("0000FE2C-0000-1000-8000-00805f9b34fb");

    private AdvertiseSettings mSettings;
    private AdvertiseData mData;
    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    private AdvertiseCallback mAdvertiseCallback;


    FastPairProvider(Context context) {
        Resources res = context.getResources();
        int modelId = res.getInteger(R.integer.fastPairModelId);
        if (modelId == 0) {
            Log.w(TAG, "Model ID undefined, disabling");
            return;
        }

        BluetoothManager bluetoothManager = (BluetoothManager) context.getSystemService(
                Context.BLUETOOTH_SERVICE);
        if (bluetoothManager != null) {
            BluetoothAdapter bluetoothAdapter = bluetoothManager.getAdapter();
            if (bluetoothAdapter != null) {
                mBluetoothLeAdvertiser = bluetoothAdapter.getBluetoothLeAdvertiser();
            }
        }

        AdvertiseSettings.Builder settingsBuilder = new AdvertiseSettings.Builder();
        settingsBuilder.setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY);
        settingsBuilder.setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH);
        settingsBuilder.setConnectable(true);
        settingsBuilder.setTimeout(0);
        mSettings = settingsBuilder.build();

        AdvertiseData.Builder dataBuilder = new AdvertiseData.Builder();
        ByteBuffer modelIdBytes = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(
                modelId);
        byte[] fastPairServiceData = Arrays.copyOfRange(modelIdBytes.array(), 0, 3);
        dataBuilder.addServiceData(FastPairServiceUuid, fastPairServiceData);
        dataBuilder.setIncludeTxPowerLevel(true).build();
        mData = dataBuilder.build();

        mAdvertiseCallback = new FastPairAdvertiseCallback();
    }

    /* register the BLE advertisement using the model id provided during construction */
    boolean startAdvertising() {
        if (mBluetoothLeAdvertiser != null) {
            mBluetoothLeAdvertiser.startAdvertising(mSettings, mData, mAdvertiseCallback);
            return true;
        }
        return false;
    }

    /* unregister the BLE advertisement. */
    boolean stopAdvertising() {
        if (mBluetoothLeAdvertiser != null) {
            mBluetoothLeAdvertiser.stopAdvertising(mAdvertiseCallback);
            return true;
        }
        return false;
    }

    /* Callback to handle status when advertising starts. */
    private class FastPairAdvertiseCallback extends AdvertiseCallback {
        @Override
        public void onStartFailure(int errorCode) {
            super.onStartFailure(errorCode);
            if (DBG) Log.d(TAG, "Advertising failed");
        }

        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            super.onStartSuccess(settingsInEffect);
            if (DBG) Log.d(TAG, "Advertising successfully started");
        }
    }
}
